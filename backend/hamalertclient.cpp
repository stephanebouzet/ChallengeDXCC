#include "backend/hamalertclient.h"
#include "backend/backendcommon.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonParseError>

HamAlertClient::HamAlertClient(const QString &host, quint16 port, const QString &configPath,
                                QObject *parent)
    : QObject(parent)
    , m_host(host)
    , m_port(port)
    , m_configPath(configPath)
{
    loadConfig();

    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &HamAlertClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &HamAlertClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &HamAlertClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &HamAlertClient::onErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(10000); // HAMALERT_RECONNECT_SECONDS
    connect(m_reconnectTimer, &QTimer::timeout, this, &HamAlertClient::tryReconnect);
}

void HamAlertClient::loadConfig()
{
    m_username.clear();
    m_password.clear();

    QFile f(m_configPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || !line.contains('='))
            continue;
        const int eq = line.indexOf('=');
        const QString key = line.left(eq).trimmed().toLower();
        const QString value = line.mid(eq + 1).trimmed();
        if (key == "username" || key == "user" || key == "callsign")
            m_username = value;
        else if (key == "password" || key == "pass")
            m_password = value;
    }
}

void HamAlertClient::start()
{
    m_wantConnected = true;
    if (!configured()) {
        emit logLine(QString("HamAlert      : configuration absente/incomplète"));
        return;
    }
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        m_socket->connectToHost(m_host, m_port);
    m_reconnectTimer->start();
}

void HamAlertClient::close()
{
    m_wantConnected = false;
    m_reconnectTimer->stop();
    m_socket->disconnectFromHost();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->waitForDisconnected(500);
    m_loggedIn = false;
    m_jsonRequested = false;
    m_rx.clear();
}

void HamAlertClient::tryReconnect()
{
    if (!m_wantConnected || !configured())
        return;
    if (m_socket->state() == QAbstractSocket::UnconnectedState)
        m_socket->connectToHost(m_host, m_port);
}

void HamAlertClient::onConnected()
{
    m_loggedIn = false;
    m_jsonRequested = false;
    m_rx.clear();
    emit logLine(QString("[HAMALERT] Connecté à %1:%2").arg(m_host).arg(m_port));
    emit stateChanged(true, "hamalert.org:7300 - connecté");
}

void HamAlertClient::onDisconnected()
{
    m_loggedIn = false;
    m_jsonRequested = false;
    m_rx.clear();
    emit logLine("[HAMALERT] Déconnecté.");
    emit stateChanged(false, "hamalert.org:7300");
}

void HamAlertClient::onErrorOccurred()
{
    emit logLine("[HAMALERT] Connexion impossible : " + m_socket->errorString());
}

void HamAlertClient::sendLine(const QString &text)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState)
        return;
    m_socket->write((text + "\r\n").toUtf8());
}

QByteArray HamAlertClient::stripTelnet(const QByteArray &data)
{
    QByteArray out;
    int i = 0;
    while (i < data.size()) {
        const quint8 b = static_cast<quint8>(data[i]);
        if (b != IAC) { out.append(char(b)); i++; continue; }
        if (i + 1 >= data.size()) break;
        const quint8 cmd = static_cast<quint8>(data[i + 1]);
        if (cmd == IAC) { out.append(char(IAC)); i += 2; continue; }
        if (cmd == DO || cmd == DONT || cmd == WILL || cmd == WONT) {
            if (i + 2 >= data.size()) break;
            const quint8 option = static_cast<quint8>(data[i + 2]);
            char answer[3];
            answer[0] = char(IAC);
            answer[1] = (cmd == DO || cmd == DONT) ? char(WONT) : char(DONT);
            answer[2] = char(option);
            m_socket->write(answer, 3);
            i += 3;
            continue;
        }
        i += 2;
    }
    return out;
}

void HamAlertClient::onReadyRead()
{
    const QByteArray data = m_socket->readAll();
    m_rx.append(stripTelnet(data));

    const QString promptText = QString::fromUtf8(m_rx).toLower();

    if (!m_loggedIn) {
        if (promptText.contains("login:") || promptText.contains("username:") ||
            promptText.contains("callsign:") || promptText.contains("call:")) {
            sendLine(m_username);
            m_rx.clear();
            return;
        }
        if (promptText.contains("password:")) {
            sendLine(m_password);
            m_rx.clear();
            m_loggedIn = true;
            return;
        }
    }

    if (m_loggedIn && !m_jsonRequested) {
        sendLine("set/json");
        m_jsonRequested = true;
        emit logLine("[HAMALERT] Mode JSON demandé.");
    }

    int nl;
    while ((nl = m_rx.indexOf('\n')) >= 0) {
        QByteArray rawLine = m_rx.left(nl);
        m_rx.remove(0, nl + 1);
        QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.isEmpty())
            continue;

        const QString lower = line.toLower();
        if (lower.contains("password:") && !m_loggedIn) {
            sendLine(m_password);
            m_loggedIn = true;
            continue;
        }

        if (line.startsWith('{') && line.endsWith('}')) {
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError && doc.isObject())
                emit alertReceived(doc.object());
            else
                emit logLine(QString("[HAMALERT JSON] Erreur de décodage : %1").arg(err.errorString()));
        }
    }
}
