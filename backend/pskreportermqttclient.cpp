#include "backend/pskreportermqttclient.h"
#include "backend/backendcommon.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QCoreApplication>
#include <QRandomGenerator>

PSKReporterMQTTClient::PSKReporterMQTTClient(const QString &configPath, QObject *parent)
    : QObject(parent)
    , m_configPath(configPath)
{
    loadConfig();
    m_clock.start();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &PSKReporterMQTTClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &PSKReporterMQTTClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &PSKReporterMQTTClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &PSKReporterMQTTClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &PSKReporterMQTTClient::reconnectTick);

    m_pingTimer = new QTimer(this);
    m_pingTimer->setInterval(30000); // keepalive 60s côté serveur, on ping 2x plus souvent
    connect(m_pingTimer, &QTimer::timeout, this, &PSKReporterMQTTClient::sendPing);
}

void PSKReporterMQTTClient::loadConfig()
{
    QFile f(m_configPath);
    if (!f.open(QIODevice::ReadOnly))
        return;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logLine("[PSKREPORTER] Configuration invalide : " + err.errorString());
        return;
    }
    const QJsonObject obj = doc.object();
    m_enabled = obj.value("enabled").toBool(true);
    const QString host = bcClean(obj.value("host").toString());
    if (!host.isEmpty())
        m_host = host;
    m_port = static_cast<quint16>(obj.value("port").toInt(1883));

    if (obj.contains("modes") && obj.value("modes").isArray()) {
        m_modes.clear();
        for (const QJsonValue &v : obj.value("modes").toArray()) {
            const QString mode = bcClean(v.toString()).toUpper();
            if (!mode.isEmpty())
                m_modes << mode;
        }
    }
    if (obj.contains("rx_dxcc") && obj.value("rx_dxcc").isArray()) {
        m_rxDxcc.clear();
        for (const QJsonValue &v : obj.value("rx_dxcc").toArray())
            m_rxDxcc << v.toInt();
    }
    m_dedupeSeconds = qMax(0, obj.value("dedupe_seconds").toInt(60));
}

QStringList PSKReporterMQTTClient::topics() const
{
    QStringList result;
    for (const QString &band : CHALLENGE_BANDS) {
        for (const QString &mode : m_modes) {
            for (int rx : m_rxDxcc) {
                result << QString("pskr/filter/v2/%1/%2/+/+/+/+/+/%3").arg(band, mode).arg(rx);
            }
        }
    }
    return result;
}

void PSKReporterMQTTClient::start()
{
    if (!m_enabled) {
        emit logLine("[PSKREPORTER] Désactivé par configuration.");
        return;
    }
    m_wantConnected = true;
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        m_socket->connectToHost(m_host, m_port);
    m_reconnectTimer->start();
    emit logLine(QString("[PSKREPORTER] Connexion MQTT demandée %1:%2").arg(m_host).arg(m_port));
}

void PSKReporterMQTTClient::close()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    m_pingTimer->stop();
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->waitForDisconnected(500);
    m_connected = false;
    m_connAckReceived = false;
    m_rx.clear();
}

void PSKReporterMQTTClient::reconnectTick()
{
    if (!m_wantConnected || !m_enabled)
        return;
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        m_socket->connectToHost(m_host, m_port);
}

void PSKReporterMQTTClient::onConnected()
{
    m_connAckReceived = false;
    m_rx.clear();
    sendConnectPacket();
}

void PSKReporterMQTTClient::onDisconnected()
{
    const bool wasConnected = m_connected;
    m_connected = false;
    m_connAckReceived = false;
    m_pingTimer->stop();
    if (wasConnected) {
        emit logLine("[PSKREPORTER] Déconnecté");
        emit stateChanged(false, m_host + ":" + QString::number(m_port));
    }
}

void PSKReporterMQTTClient::onErrorOccurred()
{
    emit logLine("[PSKREPORTER] Connexion impossible : " + m_socket->errorString());
}

QByteArray PSKReporterMQTTClient::encodeRemainingLength(int length)
{
    QByteArray out;
    do {
        quint8 byte = length % 128;
        length /= 128;
        if (length > 0)
            byte |= 0x80;
        out.append(char(byte));
    } while (length > 0);
    return out;
}

QByteArray PSKReporterMQTTClient::encodeMqttString(const QString &s)
{
    const QByteArray utf8 = s.toUtf8();
    QByteArray out;
    out.append(char((utf8.size() >> 8) & 0xFF));
    out.append(char(utf8.size() & 0xFF));
    out.append(utf8);
    return out;
}

void PSKReporterMQTTClient::writePacket(quint8 header, const QByteArray &variableAndPayload)
{
    QByteArray packet;
    packet.append(char(header));
    packet.append(encodeRemainingLength(variableAndPayload.size()));
    packet.append(variableAndPayload);
    m_socket->write(packet);
}

void PSKReporterMQTTClient::sendConnectPacket()
{
    QByteArray body;
    body.append(encodeMqttString("MQTT"));
    body.append(char(4));    // niveau de protocole MQTT 3.1.1
    body.append(char(0x02)); // connect flags : clean session
    body.append(char(0));    // keepalive MSB (60s)
    body.append(char(60));   // keepalive LSB

    const QString clientId = QString("ChallengeDXCC-%1-%2")
                                  .arg(QCoreApplication::applicationPid())
                                  .arg(QRandomGenerator::global()->bounded(10000));
    body.append(encodeMqttString(clientId));

    writePacket(0x10, body); // CONNECT
}

void PSKReporterMQTTClient::sendSubscribePackets()
{
    const QStringList allTopics = topics();
    // On regroupe les abonnements par paquets de 20 pour rester sous les
    // limites usuelles de longueur de paquet MQTT.
    const int chunkSize = 20;
    for (int i = 0; i < allTopics.size(); i += chunkSize) {
        QByteArray body;
        body.append(char((m_nextPacketId >> 8) & 0xFF));
        body.append(char(m_nextPacketId & 0xFF));
        m_nextPacketId++;
        if (m_nextPacketId == 0)
            m_nextPacketId = 1;

        const int end = qMin(i + chunkSize, allTopics.size());
        for (int j = i; j < end; ++j) {
            body.append(encodeMqttString(allTopics.at(j)));
            body.append(char(0)); // QoS 0
        }
        writePacket(0x82, body); // SUBSCRIBE, QoS1 obligatoire sur le paquet lui-même
    }
    emit logLine(QString("[PSKREPORTER] Connecté à %1:%2 - %3 abonnements")
                      .arg(m_host).arg(m_port).arg(allTopics.size()));
}

void PSKReporterMQTTClient::sendPing()
{
    if (m_connected)
        writePacket(0xC0, QByteArray()); // PINGREQ
}

void PSKReporterMQTTClient::onReadyRead()
{
    m_rx.append(m_socket->readAll());
    processIncoming();
}

void PSKReporterMQTTClient::processIncoming()
{
    while (true) {
        if (m_rx.size() < 2)
            return;

        const quint8 header = static_cast<quint8>(m_rx.at(0));
        const quint8 type = header >> 4;

        // Décodage de la longueur restante (variable byte integer).
        int multiplier = 1, remaining = 0, index = 1;
        quint8 encodedByte;
        bool complete = false;
        while (index <= 4 && index < m_rx.size()) {
            encodedByte = static_cast<quint8>(m_rx.at(index));
            remaining += (encodedByte & 0x7F) * multiplier;
            multiplier *= 128;
            index++;
            if (!(encodedByte & 0x80)) { complete = true; break; }
        }
        if (!complete)
            return; // pas assez de données reçues pour connaître la taille du paquet

        const int totalSize = index + remaining;
        if (m_rx.size() < totalSize)
            return; // paquet incomplet, on attend la suite

        const QByteArray payload = m_rx.mid(index, remaining);
        m_rx.remove(0, totalSize);

        switch (type) {
        case 2: // CONNACK
            if (payload.size() >= 2 && static_cast<quint8>(payload.at(1)) == 0) {
                m_connAckReceived = true;
                m_connected = true;
                sendSubscribePackets();
                m_pingTimer->start();
                emit stateChanged(true, m_host + ":" + QString::number(m_port));
            } else {
                emit logLine("[PSKREPORTER] Connexion refusée par le broker");
            }
            break;
        case 3: // PUBLISH
            handlePublish(payload, (header & 0x06) == 0);
            break;
        case 9:  // SUBACK
        case 13: // PINGRESP
        default:
            break;
        }
    }
}

void PSKReporterMQTTClient::handlePublish(const QByteArray &variableHeaderAndPayload, bool qos0)
{
    if (variableHeaderAndPayload.size() < 2)
        return;
    const int topicLen = (static_cast<quint8>(variableHeaderAndPayload.at(0)) << 8) |
                          static_cast<quint8>(variableHeaderAndPayload.at(1));
    if (variableHeaderAndPayload.size() < 2 + topicLen)
        return;
    const QString topic = QString::fromUtf8(variableHeaderAndPayload.mid(2, topicLen));

    int payloadStart = 2 + topicLen;
    if (!qos0)
        payloadStart += 2; // identifiant de paquet (QoS 1/2), ignoré : le broker n'envoie que du QoS0 ici

    const QByteArray jsonPayload = variableHeaderAndPayload.mid(payloadStart);

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonPayload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    QJsonObject obj = doc.object();
    obj["_topic"] = topic;
    emit spotReceived(obj);
}

bool PSKReporterMQTTClient::allowSpot(const QString &call, const QString &band)
{
    if (m_dedupeSeconds <= 0)
        return true;

    const auto key = qMakePair(bcClean(call).toUpper(), bcClean(band).toLower());
    const qint64 now = m_clock.elapsed();

    auto it = m_lastForwarded.constFind(key);
    if (it != m_lastForwarded.constEnd() && (now - it.value()) < qint64(m_dedupeSeconds) * 1000)
        return false;

    m_lastForwarded[key] = now;

    if (m_lastForwarded.size() > 5000) {
        const qint64 cutoff = now - qMax(m_dedupeSeconds * 2, 120) * qint64(1000);
        for (auto purgeIt = m_lastForwarded.begin(); purgeIt != m_lastForwarded.end();) {
            if (purgeIt.value() < cutoff)
                purgeIt = m_lastForwarded.erase(purgeIt);
            else
                ++purgeIt;
        }
    }
    return true;
}
