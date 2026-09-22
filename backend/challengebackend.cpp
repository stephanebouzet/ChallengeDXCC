#include "backend/challengebackend.h"
#include "backend/backendcommon.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>

namespace {
constexpr const char *HAMALERT_HOST = "hamalert.org";
constexpr quint16 HAMALERT_PORT = 7300;
const QString DEFAULT_DUMP_PATH = QStringLiteral("C:/Logger32/ClubLog Dump File.txt");
}

ChallengeBackend::ChallengeBackend(const Paths &paths, QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_engine(paths.resources)
{
    m_db = new ChallengeDB(paths.runtime.filePath("challenge_dxcc.db"));
    m_resolver = new LocalDXCCResolver(paths.resources.filePath("dxcc_prefixes.txt"),
                                        paths.resources.filePath("dxcc_entities.txt"),
                                        paths.resources.filePath("dxcc_aliases.txt"));
    m_importer = new ADIFImporter(*m_db, *m_resolver);
    m_deletedWatcher = new DeletedQSOWatcher(*m_db);

    m_hamAlert = new HamAlertClient(HAMALERT_HOST, HAMALERT_PORT,
                                     paths.config.filePath("hamalert.ini"), this);
    m_localCluster = new LocalDXClusterServer("0.0.0.0", 7301, this);
    m_pskReporter = new PSKReporterMQTTClient(paths.config.filePath("pskreporter.json"), this);
    m_dxCluster = new GenericDXClusterClient(paths.config.filePath("dxcluster.json"), this);
    m_udpListener = new JtdxUdpListener("127.0.0.1", 2238, this);
    m_logger32 = new Logger32Bridge(this);

    connect(m_hamAlert, &HamAlertClient::alertReceived, this, &ChallengeBackend::onHamAlertAlert);
    connect(m_hamAlert, &HamAlertClient::logLine, this, &ChallengeBackend::logLine);
    connect(m_hamAlert, &HamAlertClient::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("hamalert", active, d);
    });

    connect(m_localCluster, &LocalDXClusterServer::logLine, this, &ChallengeBackend::logLine);
    connect(m_localCluster, &LocalDXClusterServer::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("localcluster", active, d);
    });
    connect(m_localCluster, &LocalDXClusterServer::clusterTx, this, &ChallengeBackend::dxSpotLine);
    connect(m_localCluster, &LocalDXClusterServer::alertBroadcast, this,
            [this](const QJsonObject &alert, const QString &line) {
                emit dxSpotAccepted(alert, line);
            });

    connect(m_pskReporter, &PSKReporterMQTTClient::spotReceived, this, &ChallengeBackend::onPskReport);
    connect(m_pskReporter, &PSKReporterMQTTClient::logLine, this, &ChallengeBackend::logLine);
    connect(m_pskReporter, &PSKReporterMQTTClient::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("pskreporter", active, d);
    });

    connect(m_dxCluster, &GenericDXClusterClient::spotReceived, this, &ChallengeBackend::onDxClusterSpot);
    connect(m_dxCluster, &GenericDXClusterClient::logLine, this, &ChallengeBackend::logLine);
    connect(m_dxCluster, &GenericDXClusterClient::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("dxcluster", active, d);
    });

    connect(m_udpListener, &JtdxUdpListener::adifReceived, this, &ChallengeBackend::onUdpAdif);
    connect(m_udpListener, &JtdxUdpListener::logLine, this, &ChallengeBackend::logLine);
    connect(m_udpListener, &JtdxUdpListener::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("jtdx", active, d);
    });

    connect(m_logger32, &Logger32Bridge::logLine, this, &ChallengeBackend::logLine);
    connect(m_logger32, &Logger32Bridge::stateChanged, this, [this](bool active, const QString &d) {
        emit serviceStateChanged("logger32", active, d);
    });
    connect(m_logger32, &Logger32Bridge::qsoCommitted, this, &ChallengeBackend::onLogger32Qso);
    connect(m_logger32, &Logger32Bridge::deletedDumpPathReceived, this, &ChallengeBackend::onDeletedDumpPath);

    m_logger32Timer = new QTimer(this);
    m_logger32Timer->setInterval(15000);
    connect(m_logger32Timer, &QTimer::timeout, this, &ChallengeBackend::tryConnectLogger32);

    m_deletedWatcherTimer = new QTimer(this);
    m_deletedWatcherTimer->setInterval(5000);
    connect(m_deletedWatcherTimer, &QTimer::timeout, this, &ChallengeBackend::pollDeletedWatcher);
}

ChallengeBackend::~ChallengeBackend()
{
    stop();
    delete m_db;
    delete m_resolver;
    delete m_importer;
    delete m_deletedWatcher;
}

void ChallengeBackend::start(const ChallengeProfile &profile)
{
    if (m_started)
        return;
    m_started = true;

    m_engine.loadProfile(profile);

    emit logLine(QString("=").repeated(64));
    emit logLine("   LOGGER32 / JTDX / HAMALERT - CHALLENGE DXCC - PORTAGE C++");
    emit logLine(QString("=").repeated(64));

    const auto period = backendChallengePeriod();
    emit logLine(QString("Base SQLite : %1").arg(m_paths.runtime.filePath("challenge_dxcc.db")));
    emit logLine(QString("Période     : %1 -> %2")
                      .arg(period.first.toString("dd/MM/yyyy"), period.second.toString("dd/MM/yyyy")));

    if (m_resolver->loaded())
        emit logLine(QString("DXCC local   : %1 préfixes chargés").arg(m_resolver->prefixCount()));
    else
        emit logLine("DXCC local   : ERREUR - " + m_resolver->lastError());

    // Import ADIF avant le dump de suppressions, pour ne jamais ressusciter
    // un QSO supprimé dans Logger32.
    if (profile.hasAdif()) {
        const AdifImportStats stats = m_importer->importFile(profile.adifPath);
        emit logLine(QString("Import ADIF   : %1: %2 QSO lus, %3 dans la période, %4 DXCC résolus")
                          .arg(stats.file).arg(stats.read).arg(stats.challenge).arg(stats.resolved));
    } else {
        m_importer->importDirectory(m_paths.adifImport.absolutePath());
    }

    if (QFile::exists(DEFAULT_DUMP_PATH)) {
        m_deletedWatcher->setPath(DEFAULT_DUMP_PATH);
        m_deletedWatcher->poll();
    }

    refreshFilterCache();
    writeReports();

    m_localCluster->start();
    m_pskReporter->start();
    m_dxCluster->start();
    m_udpListener->start();
    m_hamAlert->start();

    if (m_hamAlert->configured())
        emit logLine("HamAlert      : compte " + m_hamAlert->username() + " configuré");
    else
        emit logLine("HamAlert      : configuration absente/incomplète (hamalert.ini)");

    tryConnectLogger32();
    m_logger32Timer->start();
    m_deletedWatcherTimer->start();

    emit logLine("Surveillance active.");
}

void ChallengeBackend::stop()
{
    if (!m_started)
        return;
    m_started = false;

    m_logger32Timer->stop();
    m_deletedWatcherTimer->stop();

    m_logger32->disconnectAll();
    m_hamAlert->close();
    m_pskReporter->close();
    m_dxCluster->close();
    m_udpListener->close();
    m_localCluster->close();
}

void ChallengeBackend::tryConnectLogger32()
{
    if (m_logger32->isConnected())
        return;
    if (!Logger32Bridge::platformSupported()) {
        m_logger32Timer->stop(); // inutile de réessayer, plateforme non-Windows
        return;
    }
    m_logger32->connectToLogger32();
}

void ChallengeBackend::pollDeletedWatcher()
{
    m_deletedWatcher->poll();
    if (m_deletedWatcher->changed) {
        m_deletedWatcher->changed = false;
        refreshFilterCache();
        writeReports();
    }
}

void ChallengeBackend::onDeletedDumpPath(const QString &path)
{
    m_deletedWatcher->setPath(path);
    m_deletedWatcher->poll();
}

void ChallengeBackend::refreshFilterCache()
{
    m_activeDxccCodes.clear();
    for (auto it = m_resolver->entitiesByCode().constBegin(); it != m_resolver->entitiesByCode().constEnd(); ++it)
        m_activeDxccCodes.insert(it.key());

    m_challengeBandSet.clear();
    for (const QString &b : CHALLENGE_BANDS)
        m_challengeBandSet.insert(b);

    const auto period = backendChallengePeriod();
    const auto worked = m_db->workedDxccByBand(period.first, period.second);

    m_workedByBand.clear();
    for (const QString &b : CHALLENGE_BANDS) {
        const QString key = b.toLower();
        QSet<int> codes;
        const auto bandMap = worked.value(key);
        for (auto it = bandMap.constBegin(); it != bandMap.constEnd(); ++it)
            if (m_activeDxccCodes.contains(it.key()))
                codes.insert(it.key());
        m_workedByBand[key] = codes;
    }
}

bool ChallengeBackend::isMissingSlot(int dxcc, const QString &bandIn) const
{
    const QString band = bandIn.toLower();
    return m_challengeBandSet.contains(band) &&
           m_activeDxccCodes.contains(dxcc) &&
           !m_workedByBand.value(band).contains(dxcc);
}

void ChallengeBackend::writeReports()
{
    const auto period = backendChallengePeriod();

    QFile txt(m_paths.runtime.filePath("dxcc_manquants_par_bande.txt"));
    if (txt.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&txt);
        out.setEncoding(QStringConverter::Utf8);
        out << "CHALLENGE DXCC - DXCC MANQUANTS PAR BANDE\n";
        out << QString("Période : %1 -> %2\n")
                   .arg(period.first.toString("dd/MM/yyyy"), period.second.toString("dd/MM/yyyy"));
        out << QString("Entités DXCC actives : %1\n\n").arg(m_activeDxccCodes.size());

        for (const QString &band : CHALLENGE_BANDS) {
            const QString key = band.toLower();
            const QSet<int> worked = m_workedByBand.value(key);
            QVector<int> missing;
            for (int code : m_activeDxccCodes)
                if (!worked.contains(code))
                    missing.append(code);
            std::sort(missing.begin(), missing.end());

            out << QString("===== %1 : %2 travaillés / %3 manquants =====\n")
                       .arg(band).arg(worked.size()).arg(missing.size());
            for (int code : missing)
                out << QString("%1  %2\n").arg(code, 3).arg(m_resolver->entitiesByCode().value(code, "?"));
            out << "\n";
        }
    }

    QFile csv(m_paths.runtime.filePath("dxcc_travailles_manquants.csv"));
    if (csv.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&csv);
        out.setEncoding(QStringConverter::Utf8);
        out << "DXCC;ENTITY";
        for (const QString &band : CHALLENGE_BANDS)
            out << ";" << band;
        out << "\n";

        for (auto it = m_resolver->entitiesByCode().constBegin(); it != m_resolver->entitiesByCode().constEnd(); ++it) {
            out << it.key() << ";" << it.value();
            for (const QString &band : CHALLENGE_BANDS) {
                const bool worked = m_workedByBand.value(band.toLower()).contains(it.key());
                out << ";" << (worked ? "WORKED" : "MISSING");
            }
            out << "\n";
        }
    }

    emit reportsUpdated();
}

void ChallengeBackend::ingestFields(AdifFields fields, const QString &source, bool announce)
{
    const QString call = bcClean(fields.value("CALL")).toUpper();
    if (!backendPlausibleCallsign(call))
        return;

    const QString resolution = m_resolver->enrichFields(fields);

    if (m_db->probableDuplicate(fields, 30)) {
        emit logLine(QString("[DUP] %1 %2 %3 (déjà reçu par une autre voie)")
                          .arg(call, bcClean(fields.value("BAND")), bcClean(fields.value("MODE"))));
        return;
    }

    if (!m_db->upsertQso(fields, false, source))
        return;

    refreshFilterCache();
    writeReports();

    const QString isoDate = backendAdifDateToIso(fields.value("QSO_DATE"));
    const auto period = backendChallengePeriod();
    const bool inChallenge = !isoDate.isEmpty() &&
                              isoDate >= period.first.toString(Qt::ISODate) &&
                              isoDate <= period.second.toString(Qt::ISODate);

    if (announce) {
        emit logLine(QString("[QSO %1] %2 DXCC=%3 %4 %5 (%6)")
                          .arg(source, call, bcClean(fields.value("DXCC")),
                               bcClean(fields.value("BAND")), bcClean(fields.value("MODE")), resolution));
    }

    QJsonObject obj;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        obj[it.key()] = it.value();
    obj["_SOURCE"] = source;
    emit qsoAdded(obj, inChallenge);
}

void ChallengeBackend::onLogger32Qso(const AdifFields &fields)
{
    ingestFields(fields, "logger32_manual", true);
}

void ChallengeBackend::onUdpAdif(const QString &adifText, const QString &sourceAddr, const QString &origin)
{
    Q_UNUSED(sourceAddr);
    const AdifFields fields = backendParseAdif(adifText);
    if (!backendPlausibleCallsign(fields.value("CALL")))
        return;
    ingestFields(fields, origin, true);
}

void ChallengeBackend::onDxClusterSpot(const QJsonObject &spot)
{
    const QString call = bcClean(spot.value("call").toString()).toUpper();
    const QString band = bcClean(spot.value("band").toString()).toLower();
    QString spotter = bcClean(spot.value("spotter").toString()).toUpper();
    if (spotter.isEmpty())
        spotter = "DXC";
    const QString comment = bcClean(spot.value("comment").toString());
    const double freqKhz = spot.value("frequency_khz").toDouble(-1);

    if (call.isEmpty() || band.isEmpty() || freqKhz < 0)
        return;

    AdifFields fields;
    fields["CALL"] = call;
    m_resolver->enrichFields(fields);
    bool ok = false;
    const int dxcc = bcClean(fields.value("DXCC")).toInt(&ok);

    const QString origin = spot.value("european_spotter").toBool() ? "EU" : "MONDE";
    emit logLine(QString("[DXCLUSTER RX] %1 %2 DXCC %3 via %4 [%5]")
                      .arg(call, band, ok ? QString::number(dxcc) : "?", spotter, origin));

    if (!ok) {
        emit logLine(QString("[DXC FILTRE] %1 / %2 = DXCC NON RÉSOLU -> REJET").arg(call, band));
        return;
    }
    if (!isMissingSlot(dxcc, band)) {
        emit logLine(QString("[DXC FILTRE] DXCC %1 / %2 = HORS CHALLENGE OU DÉJÀ TRAVAILLÉ -> REJET")
                          .arg(dxcc).arg(band));
        return;
    }
    if (!m_dxCluster->allowByOrigin(call, band, spot.value("european_spotter").toBool())) {
        emit logLine(QString("[DXC FILTRE] DXCC %1 / %2 = MISSING mais DOUBLON/PRIORITÉ EUROPE -> REJET")
                          .arg(dxcc).arg(band));
        return;
    }

    static const QStringList modeCandidates = {"FT8", "FT4", "CW", "SSB", "RTTY", "PSK"};
    QString mode;
    const QString upperComment = comment.toUpper();
    for (const QString &candidate : modeCandidates) {
        if (QRegularExpression("\\b" + candidate + "\\b").match(upperComment).hasMatch()) {
            mode = candidate;
            break;
        }
    }

    QJsonObject alert;
    alert["fullCallsign"] = call;
    alert["callsign"] = call;
    alert["frequency"] = QString::number(freqKhz / 1000.0, 'f', 6);
    alert["band"] = band;
    alert["mode"] = mode.toLower();
    alert["modeDetail"] = mode.toLower();
    alert["time"] = bcClean(spot.value("time").toString());
    alert["spotter"] = spotter;
    alert["comment"] = comment.isEmpty() ? ("DX Cluster " + origin) : comment;
    alert["source"] = "dxcluster";
    alert["dxcc"] = dxcc;
    alert["entity"] = bcClean(fields.value("COUNTRY"));
    alert["rawText"] = bcClean(spot.value("raw").toString());

    emit logLine(QString("[DXC FILTRE] DXCC %1 / %2 = MISSING [%3] -> ENVOI").arg(dxcc).arg(band, origin));
    m_localCluster->broadcastAlert(alert);
}

void ChallengeBackend::onPskReport(const QJsonObject &report)
{
    const QString call = bcClean(report.value("sc").toString()).toUpper();
    const QString band = bcClean(report.value("b").toString()).toLower();
    const QString mode = bcClean(report.value("md").toString()).toUpper();

    bool dxccOk = false, freqOk = false;
    const int dxcc = report.value("sa").toVariant().toInt(&dxccOk);
    const qint64 freqHz = report.value("f").toVariant().toLongLong(&freqOk);

    if (!dxccOk || !freqOk || call.isEmpty() || band.isEmpty())
        return;

    QString receiver = bcClean(report.value("rc").toString()).toUpper();
    if (receiver.isEmpty())
        receiver = "PSKR";
    const QString receiverGrid = bcClean(report.value("rl").toString());
    const QJsonValue snr = report.value("rp");

    emit logLine(QString("[PSKREPORTER RX] %1 %2 %3 DXCC %4 via %5").arg(call, band, mode).arg(dxcc).arg(receiver));

    if (!isMissingSlot(dxcc, band)) {
        emit logLine(QString("[PSK FILTRE] DXCC %1 / %2 = HORS CHALLENGE OU DÉJÀ TRAVAILLÉ -> REJET")
                          .arg(dxcc).arg(band));
        return;
    }
    if (!m_pskReporter->allowSpot(call, band)) {
        emit logLine(QString("[PSK FILTRE] DXCC %1 / %2 = MISSING mais DOUBLON -> REJET").arg(dxcc).arg(band));
        return;
    }

    qint64 epoch = report.contains("t_tx") ? report.value("t_tx").toVariant().toLongLong()
                                            : report.value("t").toVariant().toLongLong();
    if (epoch <= 0)
        epoch = QDateTime::currentSecsSinceEpoch();
    const QString hhmm = QDateTime::fromSecsSinceEpoch(epoch).toUTC().toString("HH:mm");

    QString snrText;
    if (snr.isDouble())
        snrText = QString("%1%2 dB").arg(snr.toInt() >= 0 ? "+" : "").arg(snr.toInt());

    QStringList commentParts;
    commentParts << mode;
    if (!snrText.isEmpty())
        commentParts << snrText;
    if (!receiverGrid.isEmpty())
        commentParts << ("RX " + receiverGrid.left(6));

    QJsonObject alert;
    alert["fullCallsign"] = call;
    alert["callsign"] = call;
    alert["frequency"] = QString::number(freqHz / 1000000.0, 'f', 6);
    alert["band"] = band;
    alert["mode"] = mode.toLower();
    alert["modeDetail"] = mode.toLower();
    alert["time"] = hhmm;
    alert["spotter"] = receiver;
    alert["comment"] = commentParts.join(' ');
    alert["source"] = "pskreporter";
    alert["dxcc"] = dxcc;
    alert["entity"] = m_resolver->entitiesByCode().value(dxcc, "");
    if (snr.isDouble())
        alert["snr"] = snr.toInt();

    emit logLine(QString("[PSK FILTRE] DXCC %1 / %2 = MISSING -> ENVOI").arg(dxcc).arg(band));
    if (!m_localCluster->broadcastAlert(alert))
        emit logLine(QString("[PSK FILTRE] DXCC %1 / %2 = MISSING mais LOGGER32 déconnecté").arg(dxcc).arg(band));
}

void ChallengeBackend::onHamAlertAlert(const QJsonObject &alert)
{
    const QString band = bcClean(alert.value("band").toString()).toLower();
    QString call = bcClean(alert.value("fullCallsign").toString());
    if (call.isEmpty())
        call = bcClean(alert.value("callsign").toString());
    if (call.isEmpty())
        call = bcClean(alert.value("call").toString());
    if (call.isEmpty())
        call = "?";

    bool ok = false;
    const int dxcc = alert.value("dxcc").toVariant().toInt(&ok);

    emit logLine(QString("[HAMALERT RX] %1 DXCC %2 / %3").arg(call, ok ? QString::number(dxcc) : "?", band.isEmpty() ? "?" : band));

    if (!ok) {
        emit logLine("[FILTRE] DXCC ? / ? = INVALIDE -> REJET");
        return;
    }
    if (!isMissingSlot(dxcc, band)) {
        emit logLine(QString("[FILTRE] DXCC %1 / %2 = HORS CHALLENGE OU DÉJÀ TRAVAILLÉ -> REJET").arg(dxcc).arg(band));
        return;
    }

    emit logLine(QString("[FILTRE] DXCC %1 / %2 = MISSING -> ENVOI").arg(dxcc).arg(band));

    // Normalisation minimale pour que le tableau DX SPOTS reçoive les mêmes
    // champs quelle que soit la source. L'objet d'origine reste inchangé.
    QJsonObject normalized = alert;
    normalized["source"] = "hamalert";
    normalized["dxcc"] = dxcc;
    if (bcClean(normalized.value("entity").toString()).isEmpty())
        normalized["entity"] = m_resolver->entitiesByCode().value(dxcc, "");
    if (bcClean(normalized.value("band").toString()).isEmpty())
        normalized["band"] = band;

    if (!m_localCluster->broadcastAlert(normalized))
        emit logLine(QString("[FILTRE] DXCC %1 / %2 = MISSING mais LOGGER32 déconnecté").arg(dxcc).arg(band));
}
