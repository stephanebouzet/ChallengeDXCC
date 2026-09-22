#pragma once
// Portage C++ de HamAlertClient (services/v10_backend.py).
// Contrairement à la version Python (polling non bloquant), la version Qt
// est entièrement pilotée par les signaux de QTcpSocket.

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonObject>
#include <QByteArray>

class HamAlertClient : public QObject
{
    Q_OBJECT
public:
    explicit HamAlertClient(const QString &host, quint16 port, const QString &configPath,
                             QObject *parent = nullptr);

    bool configured() const { return !m_username.isEmpty() && !m_password.isEmpty(); }
    QString username() const { return m_username; }

    void start();
    void close();

signals:
    void alertReceived(const QJsonObject &alert);
    void stateChanged(bool connected, const QString &detail);
    void logLine(const QString &line);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred();
    void tryReconnect();

private:
    void loadConfig();
    void sendLine(const QString &text);
    QByteArray stripTelnet(const QByteArray &data);

    static constexpr quint8 IAC = 255, DONT = 254, DO = 253, WONT = 252, WILL = 251;

    QString m_host;
    quint16 m_port;
    QString m_configPath;
    QString m_username, m_password;

    QTcpSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    bool m_loggedIn = false;
    bool m_jsonRequested = false;
    QByteArray m_rx;
    bool m_wantConnected = false;
};
