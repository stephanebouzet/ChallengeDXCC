#include "db/challengedb.h"
#include "backend/backendcommon.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>

ChallengeDB::ChallengeDB(const QString &path, const QString &connectionName)
    : m_path(path)
    , m_connectionName(connectionName)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "[DB] Ouverture impossible" << path << db.lastError().text();
        return;
    }
    createSchema();
}

ChallengeDB::~ChallengeDB()
{
    close();
}

bool ChallengeDB::isOpen() const
{
    return QSqlDatabase::database(m_connectionName, false).isOpen();
}

void ChallengeDB::createSchema()
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.exec("PRAGMA journal_mode=WAL");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS qso (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            qso_key TEXT NOT NULL UNIQUE,
            logger32_number TEXT,
            call TEXT NOT NULL,
            dxcc INTEGER,
            country TEXT,
            band TEXT,
            mode TEXT,
            submode TEXT,
            freq_mhz REAL,
            qso_date TEXT,
            time_on TEXT,
            deleted INTEGER NOT NULL DEFAULT 0,
            source TEXT,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    )");
    q.exec("CREATE INDEX IF NOT EXISTS idx_qso_challenge ON qso(qso_date, dxcc, band, deleted)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_qso_logger_number ON qso(logger32_number)");
    q.exec(R"(
        CREATE TABLE IF NOT EXISTS state (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )");
}

QString ChallengeDB::makeQsoKey(const AdifFields &fields)
{
    const QString raw = bcClean(fields.value("CALL")).toUpper() + "|" +
                         bcClean(fields.value("QSO_DATE")) + "|" +
                         bcClean(fields.value("TIME_ON")) + "|" +
                         bcClean(fields.value("BAND")).toLower() + "|" +
                         bcClean(fields.value("FREQ"));
    return QString(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha1).toHex());
}

bool ChallengeDB::upsertQso(const AdifFields &fields, bool deleted, const QString &source)
{
    const QString call = bcClean(fields.value("CALL"));
    if (call.isEmpty())
        return false;

    const QString qsoKey = makeQsoKey(fields);
    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    const QString dxccValue = bcClean(fields.value("DXCC"));
    bool dxccOk = false;
    const int dxcc = dxccValue.toInt(&dxccOk);

    const QString freqValue = bcClean(fields.value("FREQ"));
    bool freqOk = false;
    const double freq = QString(freqValue).replace(',', '.').toDouble(&freqOk);

    QString country = bcClean(fields.value("COUNTRY"));
    if (country.isEmpty())
        country = bcClean(fields.value("APP_COUNTRY"));

    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(R"(
        INSERT INTO qso (
            qso_key, logger32_number, call, dxcc, country, band, mode, submode,
            freq_mhz, qso_date, time_on, deleted, source, created_at, updated_at
        ) VALUES (
            :qso_key, :logger32_number, :call, :dxcc, :country, :band, :mode, :submode,
            :freq_mhz, :qso_date, :time_on, :deleted, :source, :created_at, :updated_at
        )
        ON CONFLICT(qso_key) DO UPDATE SET
            logger32_number = excluded.logger32_number,
            call = excluded.call,
            dxcc = excluded.dxcc,
            country = excluded.country,
            band = excluded.band,
            mode = excluded.mode,
            submode = excluded.submode,
            freq_mhz = excluded.freq_mhz,
            qso_date = excluded.qso_date,
            time_on = excluded.time_on,
            deleted = excluded.deleted,
            source = excluded.source,
            updated_at = excluded.updated_at
    )");

    q.bindValue(":qso_key", qsoKey);
    const QString loggerNumber = bcClean(fields.value("APP_LOGGER32_QSO_NUMBER"));
    q.bindValue(":logger32_number", loggerNumber.isEmpty() ? QVariant() : loggerNumber);
    q.bindValue(":call", call.toUpper());
    q.bindValue(":dxcc", dxccOk ? QVariant(dxcc) : QVariant());
    q.bindValue(":country", country.isEmpty() ? QVariant() : country);
    const QString band = bcClean(fields.value("BAND")).toLower();
    q.bindValue(":band", band.isEmpty() ? QVariant() : band);
    const QString mode = bcClean(fields.value("MODE")).toUpper();
    q.bindValue(":mode", mode.isEmpty() ? QVariant() : mode);
    const QString submode = bcClean(fields.value("SUBMODE")).toUpper();
    q.bindValue(":submode", submode.isEmpty() ? QVariant() : submode);
    q.bindValue(":freq_mhz", freqOk ? QVariant(freq) : QVariant());
    const QString qsoDate = backendAdifDateToIso(fields.value("QSO_DATE"));
    q.bindValue(":qso_date", qsoDate.isEmpty() ? QVariant() : qsoDate);
    const QString timeOn = bcClean(fields.value("TIME_ON"));
    q.bindValue(":time_on", timeOn.isEmpty() ? QVariant() : timeOn);
    q.bindValue(":deleted", deleted ? 1 : 0);
    q.bindValue(":source", source);
    q.bindValue(":created_at", now);
    q.bindValue(":updated_at", now);

    if (!q.exec()) {
        qWarning() << "[DB] upsertQso" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChallengeDB::markDeleted(const AdifFields &fields)
{
    const QString qsoKey = makeQsoKey(fields);

    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT deleted FROM qso WHERE qso_key = ?");
    q.addBindValue(qsoKey);
    q.exec();
    bool alreadyDeleted = false;
    if (q.next())
        alreadyDeleted = q.value(0).toInt() != 0;

    upsertQso(fields, true, "clublog_dump");
    return !alreadyDeleted;
}

bool ChallengeDB::probableDuplicate(const AdifFields &fields, int seconds)
{
    const QString call = bcClean(fields.value("CALL")).toUpper();
    const QString band = bcClean(fields.value("BAND")).toLower();
    const QString mode = bcClean(fields.value("MODE")).toUpper();
    const QString qsoDate = backendAdifDateToIso(fields.value("QSO_DATE"));
    const QString timeOn = bcClean(fields.value("TIME_ON"));

    if (call.isEmpty() || band.isEmpty() || mode.isEmpty() || qsoDate.isEmpty() || timeOn.isEmpty())
        return false;
    if (timeOn.length() < 6)
        return false;
    for (int i = 0; i < 6; ++i)
        if (!timeOn.at(i).isDigit())
            return false;

    const int target = timeOn.mid(0, 2).toInt() * 3600 + timeOn.mid(2, 2).toInt() * 60 + timeOn.mid(4, 2).toInt();

    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(R"(
        SELECT time_on FROM qso
        WHERE deleted = 0 AND call = ? AND band = ? AND mode = ? AND qso_date = ?
    )");
    q.addBindValue(call);
    q.addBindValue(band);
    q.addBindValue(mode);
    q.addBindValue(qsoDate);
    q.exec();

    while (q.next()) {
        const QString value = q.value(0).toString();
        if (value.length() < 6)
            continue;
        bool ok = true;
        for (int i = 0; i < 6; ++i)
            if (!value.at(i).isDigit()) { ok = false; break; }
        if (!ok)
            continue;
        const int current = value.mid(0, 2).toInt() * 3600 + value.mid(2, 2).toInt() * 60 + value.mid(4, 2).toInt();
        if (qAbs(current - target) <= seconds)
            return true;
    }
    return false;
}

int ChallengeDB::countActiveQso()
{
    QSqlQuery q("SELECT COUNT(*) FROM qso WHERE deleted = 0", QSqlDatabase::database(m_connectionName));
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

int ChallengeDB::countChallengeQso(const QDate &start, const QDate &end)
{
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare("SELECT COUNT(*) FROM qso WHERE deleted = 0 AND qso_date >= ? AND qso_date <= ?");
    q.addBindValue(start.toString(Qt::ISODate));
    q.addBindValue(end.toString(Qt::ISODate));
    q.exec();
    return q.next() ? q.value(0).toInt() : 0;
}

QMap<QString, QMap<int, int>> ChallengeDB::workedDxccByBand(const QDate &start, const QDate &end)
{
    QMap<QString, QMap<int, int>> result;
    QSqlQuery q(QSqlDatabase::database(m_connectionName));
    q.prepare(R"(
        SELECT band, dxcc, COUNT(*) AS qso_count
        FROM qso
        WHERE deleted = 0 AND dxcc IS NOT NULL AND band IS NOT NULL
          AND qso_date >= ? AND qso_date <= ?
        GROUP BY band, dxcc
    )");
    q.addBindValue(start.toString(Qt::ISODate));
    q.addBindValue(end.toString(Qt::ISODate));
    q.exec();
    while (q.next()) {
        const QString band = q.value(0).toString().toLower();
        const int dxcc = q.value(1).toInt();
        const int count = q.value(2).toInt();
        result[band][dxcc] = count;
    }
    return result;
}

void ChallengeDB::close()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid() && db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}
