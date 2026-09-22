#include "gui/tciclient.h"

#include <QRegularExpression>

TciClient::TciClient(const QString &host, quint16 port, int receiver, int vfo, QObject *parent)
    : QObject(parent)
    , m_host(host)
    , m_port(port)
    , m_receiver(receiver)
    , m_vfo(vfo)
{
    m_socket = new QWebSocket();
    m_socket->setParent(this);
    connect(m_socket, &QWebSocket::connected, this, &TciClient::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &TciClient::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &TciClient::onTextMessageReceived);
    connect(m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        // Silencieux : reconnexion automatique périodique, comme la version Python.
    });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(750);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TciClient::reconnectTick);
}

void TciClient::start()
{
    m_wantConnected = true;
    m_reconnectTimer->start();
    reconnectTick();
}

void TciClient::stop()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    m_socket->close();
    m_ready = false;
    m_locked = -1;
}

void TciClient::reconnectTick()
{
    if (!m_wantConnected)
        return;
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        m_socket->open(QUrl(QString("ws://%1:%2").arg(m_host).arg(m_port)));
}

void TciClient::onConnected()
{
    m_ready = false;
    m_locked = -1;
}

void TciClient::onDisconnected()
{
    const bool wasReady = m_ready;
    m_ready = false;
    m_locked = -1;
    if (wasReady)
        emit readyChanged(false);
}

void TciClient::send(const QString &command)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->sendTextMessage(command);
}

void TciClient::handleCommand(const QString &commandIn)
{
    const QString command = commandIn.trimmed();
    const QString low = command.toLower();

    static const QRegularExpression protoRe("protocol:([^;]+);", QRegularExpression::CaseInsensitiveOption);
    const auto protoMatch = protoRe.match(command);
    if (protoMatch.hasMatch())
        m_protocol = protoMatch.captured(1).trimmed();

    const QRegularExpression lockRe(
        QString("\\block:%1,(true|false)\\s*;").arg(m_receiver),
        QRegularExpression::CaseInsensitiveOption);
    const auto lockMatch = lockRe.match(command);
    if (lockMatch.hasMatch())
        m_locked = (lockMatch.captured(1).toLower() == "true") ? 1 : 0;

    if (low == "ready;") {
        m_ready = true;
        emit readyChanged(true);
        send(QString("LOCK:%1;").arg(m_receiver));
    }
}

void TciClient::onTextMessageReceived(const QString &frame)
{
    const QStringList parts = frame.split(';');
    for (QString part : parts) {
        part = part.trimmed();
        if (!part.isEmpty())
            handleCommand(part + ";");
    }
}

QPair<bool, QString> TciClient::setFrequency(qint64 frequencyHz)
{
    if (!m_ready || m_socket->state() != QAbstractSocket::ConnectedState)
        return {false, "TCI non prêt"};
    if (m_locked == 1)
        return {false, "VFO verrouillé"};
    if (m_locked == -1) {
        send(QString("LOCK:%1;").arg(m_receiver));
        return {false, "état LOCK non confirmé"};
    }

    const QString command = QString("VFO:%1,%2,%3;").arg(m_receiver).arg(m_vfo).arg(frequencyHz);
    send(command);
    return {true, "OK"};
}
