#pragma once
// Portage C++ de PSKReporterMQTTClient (services/v10_backend.py).
//
// Qt n'embarque pas de client MQTT (Qt Mqtt est un module séparé, non
// garanti disponible) : ce fichier implémente un client MQTT 3.1.1 minimal
// (CONNECT/CONNACK, SUBSCRIBE, PUBLISH en réception, PINGREQ/PINGRESP) sur
// QTcpSocket, suffisant pour s'abonner en QoS 0 au flux mqtt.pskreporter.info.

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QMap>
#include <QPair>
#include <QSet>
#include <QElapsedTimer>

class PSKReporterMQTTClient : public QObject
{
    Q_OBJECT
public:
    explicit PSKReporterMQTTClient(const QString &configPath, QObject *parent = nullptr);

    void start();
    void close();

    bool enabled() const { return m_enabled; }
    bool isConnected() const { return m_connected; }

    // Anti-rafale : même CALL/bande transmis au plus une fois par délai.
    bool allowSpot(const QString &call, const QString &band);

signals:
    void spotReceived(const QJsonObject &data); // contient "_topic"
    void logLine(const QString &line);
    void stateChanged(bool connected, const QString &detail);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred();
    void reconnectTick();
    void sendPing();

private:
    void loadConfig();
    QStringList topics() const;

    void sendConnectPacket();
    void sendSubscribePackets();
    void writePacket(quint8 header, const QByteArray &variableAndPayload);
    static QByteArray encodeRemainingLength(int length);
    static QByteArray encodeMqttString(const QString &s);
    void processIncoming();
    void handlePublish(const QByteArray &variableHeaderAndPayload, bool qos0);

    QString m_configPath;
    bool m_enabled = true;
    QString m_host = "mqtt.pskreporter.info";
    quint16 m_port = 1883;
    QStringList m_modes = {"FT8", "FT4"};
    QVector<int> m_rxDxcc = {227};
    int m_dedupeSeconds = 60;

    QTcpSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_pingTimer = nullptr;
    bool m_wantConnected = false;
    bool m_connected = false;
    bool m_connAckReceived = false;
    QByteArray m_rx;
    quint16 m_nextPacketId = 1;

    QMap<QPair<QString, QString>, qint64> m_lastForwarded; // (call, band) -> ms monotonic
    QElapsedTimer m_clock;
};
