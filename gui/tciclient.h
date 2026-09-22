#pragma once
// Portage C++ de PersistentTCIClient (gui/main_window.py).
// Connexion WebSocket TCI permanente pour des QSY instantanés.

#include <QObject>
#include <QWebSocket>
#include <QTimer>

class TciClient : public QObject
{
    Q_OBJECT
public:
    explicit TciClient(const QString &host, quint16 port, int receiver = 0, int vfo = 0,
                        QObject *parent = nullptr);

    void start();
    void stop();

    // Retourne (ok, raison). Aucun QSY si LOCK n'est pas connu/libre.
    QPair<bool, QString> setFrequency(qint64 frequencyHz);

    bool isReady() const { return m_ready; }

signals:
    void logLine(const QString &line);
    void readyChanged(bool ready);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);
    void reconnectTick();

private:
    void handleCommand(const QString &command);
    void send(const QString &command);

    QString m_host;
    quint16 m_port;
    int m_receiver;
    int m_vfo;

    QWebSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    bool m_wantConnected = false;
    bool m_ready = false;
    int m_locked = -1; // -1 inconnu, 0 faux, 1 vrai
    QString m_protocol;
};
