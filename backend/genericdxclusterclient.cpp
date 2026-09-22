#include "backend/genericdxclusterclient.h"
#include "backend/backendcommon.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegularExpression>

const QStringList GenericDXClusterClient::EUROPE_PREFIXES = {
    "1A","3A","4O","4U1V","9A","C3","CT","CU","D","DA","DB","DC","DD","DF","DG","DH","DJ","DK","DL","DM","DN","DO","DP","DQ","DR",
    "E7","EA","EB","EC","ED","EE","EF","EG","EH","EI","EJ","ER","ES","EU","EV","EW",
    "F","G","GM","GW","GI","GD","GU","GJ","HA","HB","HB0","HV","I","IS","IT","IQ","IR","II",
    "LA","LB","LC","LD","LE","LF","LG","LI","LJ","LN","LX","LY","LZ",
    "OE","OH","OJ0","OK","OL","OM","ON","OO","OP","OQ","OR","OS","OT","OY","OZ",
    "PA","PB","PC","PD","PE","PF","PG","PH","PI","S5","SM","SN","SO","SP","SQ","SR","SV","SX","SY","SZ",
    "T7","TA1","TF","TK","UA1","UA2","UA3","UA4","UA6","UR","US","UT","UX","UY","UZ",
    "YL","YO","YP","YQ","YR","YU","YT","Z3","ZA","ZB2"
};

GenericDXClusterClient::GenericDXClusterClient(const QString &configPath, QObject *parent)
    : QObject(parent)
    , m_configPath(configPath)
{
    m_servers = {
        {"dxc.hamserve.uk", 7300, ""},
        {"dxcluster.iu1bow.it", 7300, ""},
        {"dxc.pi4cc.nl", 8000, ""},
    };
    loadConfig();
    m_clock.start();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &GenericDXClusterClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &GenericDXClusterClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &GenericDXClusterClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &GenericDXClusterClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(1000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &GenericDXClusterClient::reconnectTick);
}

void GenericDXClusterClient::loadConfig()
{
    QFile f(m_configPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logLine("[DXCLUSTER] Configuration invalide : " + err.errorString());
        return;
    }
    const QJsonObject obj = doc.object();
    m_enabled = obj.value("enabled").toBool(true);
    const QString call = bcClean(obj.value("callsign").toString()).toUpper();
    if (!call.isEmpty())
        m_callsign = call;

    if (obj.contains("servers") && obj.value("servers").isArray()) {
        QVector<DxClusterServerCfg> parsed;
        for (const QJsonValue &v : obj.value("servers").toArray()) {
            if (!v.isObject())
                continue;
            const QJsonObject item = v.toObject();
            const QString host = bcClean(item.value("host").toString());
            const int port = item.value("port").toInt(-1);
            if (!host.isEmpty() && port >= 1 && port <= 65535)
                parsed.append({host, static_cast<quint16>(port), item.value("label").toString()});
        }
        if (!parsed.isEmpty())
            m_servers = parsed;
    }

    m_preferEuropeanSpotters = obj.value("prefer_european_spotters").toBool(true);
    m_euDedupeSeconds = qMax(0, obj.value("eu_dedupe_seconds").toInt(45));
    m_otherDedupeSeconds = qMax(0, obj.value("other_dedupe_seconds").toInt(180));
    m_euPreferenceWindowSeconds = qMax(0, obj.value("eu_preference_window_seconds").toInt(600));
    m_reconnectSeconds = qMax(3, obj.value("reconnect_seconds").toInt(10));
}

void GenericDXClusterClient::start()
{
    m_wantConnected = true;
    m_reconnectTimer->start();
    connectNext();
}

void GenericDXClusterClient::close()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->write("bye\r\n");
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->waitForDisconnected(500);
    m_rx.clear();
}

void GenericDXClusterClient::reconnectTick()
{
    if (m_wantConnected && m_socket->state() == QAbstractSocket::UnconnectedState)
        connectNext();
}

void GenericDXClusterClient::connectNext()
{
    if (!m_enabled || m_servers.isEmpty())
        return;
    const qint64 now = m_clock.elapsed();
    if (now - m_lastConnectAttemptMs < qint64(m_reconnectSeconds) * 1000)
        return;
    m_lastConnectAttemptMs = now;

    const DxClusterServerCfg &item = m_servers.at(m_serverIndex % m_servers.size());
    m_serverIndex = (m_serverIndex + 1) % m_servers.size();

    m_rx.clear();
    m_socket->connectToHost(item.host, item.port);
}

void GenericDXClusterClient::onConnected()
{
    m_socket->write((m_callsign + "\r\n").toLatin1());
    emit logLine(QString("[DXCLUSTER] Connecté %1:%2 - call %3")
                      .arg(m_socket->peerName()).arg(m_socket->peerPort()).arg(m_callsign));
    emit stateChanged(true, m_socket->peerName());
}

void GenericDXClusterClient::onDisconnected()
{
    m_rx.clear();
    emit logLine("[DXCLUSTER] Déconnecté");
    emit stateChanged(false, "");
}

void GenericDXClusterClient::onErrorOccurred()
{
    emit logLine("[DXCLUSTER] Connexion impossible : " + m_socket->errorString());
}

QByteArray GenericDXClusterClient::stripTelnet(const QByteArray &data)
{
    QByteArray out;
    int i = 0;
    while (i < data.size()) {
        const quint8 b = static_cast<quint8>(data[i]);
        if (b != 255) { out.append(char(b)); i++; continue; }
        if (i + 1 >= data.size()) break;
        const quint8 cmd = static_cast<quint8>(data[i + 1]);
        if (cmd == 251 || cmd == 252 || cmd == 253 || cmd == 254) {
            i += 3;
        } else if (cmd == 250) {
            i += 2;
            while (i + 1 < data.size() && !(static_cast<quint8>(data[i]) == 255 &&
                                             static_cast<quint8>(data[i + 1]) == 240))
                i++;
            i += 2;
        } else {
            i += 2;
        }
    }
    return out;
}

bool GenericDXClusterClient::isEuropeanSpotter(const QString &callIn)
{
    const QString c = bcClean(callIn).toUpper();
    QStringList candidates = c.split('/', Qt::SkipEmptyParts);
    candidates << c;
    for (const QString &x : candidates) {
        for (const QString &p : EUROPE_PREFIXES)
            if (x.startsWith(p))
                return true;
    }
    return false;
}

QString GenericDXClusterClient::bandFromKhz(double freq)
{
    static const struct { const char *band; double lo, hi; } bands[] = {
        {"160m", 1800, 2000}, {"80m", 3500, 4000}, {"40m", 7000, 7300},
        {"30m", 10100, 10150}, {"20m", 14000, 14350}, {"17m", 18068, 18168},
        {"15m", 21000, 21450}, {"12m", 24890, 24990}, {"10m", 28000, 29700},
    };
    for (const auto &b : bands)
        if (freq >= b.lo && freq <= b.hi)
            return b.band;
    return QString();
}

void GenericDXClusterClient::handleLine(const QString &lineIn)
{
    const QString line = lineIn.trimmed();
    if (line.isEmpty())
        return;

    const QString low = line.toLower();
    if (low.contains("login:") || low.endsWith("call:") || low.contains("callsign:")) {
        m_socket->write((m_callsign + "\r\n").toLatin1());
        return;
    }

    static const QRegularExpression dxRe(
        R"(^DX\s+de\s+([^:]+):\s*([0-9]+(?:\.[0-9]+)?)\s+(\S+)\s+(.*?)\s+([0-2][0-9][0-5][0-9])Z?\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    const auto m = dxRe.match(line);
    if (!m.hasMatch())
        return;

    const QString spotter = bcClean(m.captured(1)).toUpper();
    const double freq = m.captured(2).toDouble();
    const QString call = bcClean(m.captured(3)).toUpper();
    const QString comment = bcClean(m.captured(4));
    const QString hhmm = m.captured(5);

    const QString band = bandFromKhz(freq);
    if (band.isEmpty())
        return;

    QJsonObject spot;
    spot["raw"] = line;
    spot["spotter"] = spotter;
    spot["frequency_khz"] = freq;
    spot["call"] = call;
    spot["comment"] = comment;
    spot["time"] = hhmm;
    spot["band"] = band.toLower();
    spot["european_spotter"] = isEuropeanSpotter(spotter);
    emit spotReceived(spot);
}

void GenericDXClusterClient::onReadyRead()
{
    m_rx.append(stripTelnet(m_socket->readAll()));

    QByteArray data = m_rx;
    data.replace('\r', '\n');
    const QList<QByteArray> parts = data.split('\n');
    m_rx = parts.last();

    for (int i = 0; i < parts.size() - 1; ++i)
        handleLine(QString::fromUtf8(parts.at(i)));

    if (m_rx.size() > 32768)
        m_rx = m_rx.right(4096);
}

bool GenericDXClusterClient::allowByOrigin(const QString &call, const QString &band, bool europeanSpotter)
{
    const auto key = qMakePair(call, band);
    const qint64 now = m_clock.elapsed();

    if (!m_preferEuropeanSpotters) {
        const qint64 last = m_lastAny.value(key, -1000000000);
        if (now - last < qint64(m_otherDedupeSeconds) * 1000)
            return false;
        m_lastAny[key] = now;
        return true;
    }

    if (europeanSpotter) {
        const qint64 last = m_lastEu.value(key, -1000000000);
        if (now - last < qint64(m_euDedupeSeconds) * 1000)
            return false;
        m_lastEu[key] = now;
        m_lastAny[key] = now;
        return true;
    }

    const qint64 lastEu = m_lastEu.value(key, 0);
    if (lastEu > 0 && now - lastEu < qint64(m_euPreferenceWindowSeconds) * 1000)
        return false;

    const qint64 lastAny = m_lastAny.value(key, -1000000000);
    if (now - lastAny < qint64(m_otherDedupeSeconds) * 1000)
        return false;

    m_lastAny[key] = now;
    return true;
}
