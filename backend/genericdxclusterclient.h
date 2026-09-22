#pragma once
// Portage C++ de GenericDXClusterClient (services/v10_backend.py).
// Client DX Cluster TCP/Telnet générique, préférence des spotters européens.

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>
#include <QPair>
#include <QElapsedTimer>
#include <QJsonObject>

struct DxClusterServerCfg { QString host; quint16 port; QString label; };

class GenericDXClusterClient : public QObject
{
    Q_OBJECT
public:
    explicit GenericDXClusterClient(const QString &configPath, QObject *parent = nullptr);

    void start();
    void close();

    static bool isEuropeanSpotter(const QString &call);
    static QString bandFromKhz(double freqKhz);

    // Anti-doublon avec préférence Europe.
    bool allowByOrigin(const QString &call, const QString &band, bool europeanSpotter);

signals:
    void spotReceived(const QJsonObject &spot); // raw, spotter, frequency_khz, call, comment, time, band, european_spotter
    void logLine(const QString &line);
    void stateChanged(bool connected, const QString &detail);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred();
    void reconnectTick();

private:
    void loadConfig();
    void connectNext();
    QByteArray stripTelnet(const QByteArray &data);
    void handleLine(const QString &line);

    static const QStringList EUROPE_PREFIXES;

    QString m_configPath;
    bool m_enabled = true;
    QString m_callsign = "F4HAB";
    QVector<DxClusterServerCfg> m_servers;
    bool m_preferEuropeanSpotters = true;
    int m_euDedupeSeconds = 45;
    int m_otherDedupeSeconds = 180;
    int m_euPreferenceWindowSeconds = 600;
    int m_reconnectSeconds = 10;

    int m_serverIndex = 0;
    QTcpSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    bool m_wantConnected = false;
    QByteArray m_rx;
    QElapsedTimer m_clock;
    qint64 m_lastConnectAttemptMs = -1000000;

    QMap<QPair<QString, QString>, qint64> m_lastEu;
    QMap<QPair<QString, QString>, qint64> m_lastAny;
};
