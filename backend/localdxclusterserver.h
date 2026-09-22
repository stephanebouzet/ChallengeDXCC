#pragma once

#include <QTimer>
// Portage C++ de LocalDXClusterServer (services/v10_backend.py).
// Serveur DX Cluster local pour Logger32, piloté par QTcpServer/QTcpSocket
// (plus de polling manuel).

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QMap>
#include <QElapsedTimer>

class LocalDXClusterServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalDXClusterServer(const QString &host, quint16 port, QObject *parent = nullptr);

    void start();
    void close();

    static QString formatSpot(const QJsonObject &alert);
    bool broadcastAlert(const QJsonObject &alert);

signals:
    void logLine(const QString &line);
    void stateChanged(bool active, const QString &detail);
    void clusterTx(const QString &formattedLine); // journal / compatibilité
    void alertBroadcast(const QJsonObject &alert, const QString &formattedLine);

private slots:
    void onNewConnection();

private:
    struct ClientState {
        QTcpSocket *socket = nullptr;
        QString addr;
        quint16 port = 0;
        bool loggedIn = false;
        QString callsign;
        QByteArray rx;
        QElapsedTimer connectedAt;
        bool loginPromptSent = false;
        bool bannerSent = false;
        bool announced = false;
        QTimer *promptTimer = nullptr;
        QTimer *compatTimer = nullptr;
    };

    void configureClient(QTcpSocket *client);
    static QByteArray banner();
    void sendLoginPrompt(ClientState &state);
    void sendBanner(ClientState &state);
    void activateClientSession(ClientState &state, const QString &callsign, bool sendBannerFlag);
    void removeClient(QTcpSocket *client);
    static QString extractCallsignFromBuffer(const QByteArray &buffer);
    static double frequencyKhz(const QJsonObject &alert);

    QString m_host;
    quint16 m_port;
    QTcpServer *m_server = nullptr;
    QMap<QTcpSocket *, ClientState> m_clients;

    static constexpr int LOGIN_PROMPT_DELAY_MS = 150;
    static constexpr int LOGIN_COMPAT_TIMEOUT_MS = 900;
    static constexpr int TRANSIENT_CONNECTION_MS = 1000;
};
