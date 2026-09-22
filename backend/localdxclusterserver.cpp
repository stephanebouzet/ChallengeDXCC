#include "backend/localdxclusterserver.h"
#include "backend/backendcommon.h"

#include <QTimer>
#include <QDateTime>
#include <QRegularExpression>

LocalDXClusterServer::LocalDXClusterServer(const QString &host, quint16 port, QObject *parent)
    : QObject(parent)
    , m_host(host)
    , m_port(port)
{
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &LocalDXClusterServer::onNewConnection);
}

void LocalDXClusterServer::start()
{
    if (!m_server->listen(QHostAddress(m_host), m_port)) {
        emit logLine(QString("[WARN] Cluster local impossible %1:%2 : %3")
                          .arg(m_host).arg(m_port).arg(m_server->errorString()));
        emit stateChanged(false, m_host + ":" + QString::number(m_port));
        return;
    }
    emit logLine(QString("Cluster local : %1:%2 (écoute active)").arg(m_host).arg(m_port));
    emit stateChanged(true, QString("%1:%2 - serveur actif").arg(m_host).arg(m_port));
}

void LocalDXClusterServer::close()
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.value().socket->close();
        it.value().socket->deleteLater();
    }
    m_clients.clear();
    if (m_server->isListening())
        m_server->close();
}

void LocalDXClusterServer::configureClient(QTcpSocket *client)
{
    client->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);
}

QByteArray LocalDXClusterServer::banner()
{
    return QByteArray(
        "ChallengeDXCC Cpp Cluster V10\r\n"
        "Spots HamAlert / PSK Reporter : DXCC manquants seulement\r\n"
        "F4HAB de ChallengeDXCC >\r\n");
}

void LocalDXClusterServer::sendLoginPrompt(ClientState &state)
{
    if (state.loginPromptSent)
        return;
    state.socket->write("login: ");
    state.loginPromptSent = true;
}

void LocalDXClusterServer::sendBanner(ClientState &state)
{
    if (state.bannerSent)
        return;
    state.socket->write(banner());
    state.bannerSent = true;
    if (!state.announced) {
        emit logLine(QString("[CLUSTER] Connexion active depuis %1:%2 - session V10")
                          .arg(state.addr).arg(state.port));
        state.announced = true;
    }
}

QString LocalDXClusterServer::extractCallsignFromBuffer(const QByteArray &bufferBytes)
{
    if (bufferBytes.isEmpty())
        return QString();

    QString text = QString::fromLatin1(bufferBytes).toUpper();
    for (const QString &word : {"LOGIN", "CALLSIGN", "CALL", "USER", "PASSWORD"})
        text.replace(word, " ");

    static const QRegularExpression tokenRe("[A-Z0-9]+(?:/[A-Z0-9]+)*");
    auto it = tokenRe.globalMatch(text);
    while (it.hasNext()) {
        const QString token = it.next().captured(0);
        if (token.length() < 3 || token.length() > 16)
            continue;
        bool hasDigit = false, hasAlpha = false;
        for (const QChar &ch : token) {
            if (ch.isDigit()) hasDigit = true;
            if (ch.isLetter()) hasAlpha = true;
        }
        if (hasDigit && hasAlpha)
            return token;
    }
    return QString();
}

void LocalDXClusterServer::activateClientSession(ClientState &state, const QString &callsign, bool sendBannerFlag)
{
    if (state.loggedIn)
        return;
    state.callsign = (callsign.isEmpty() ? "F4HAB" : callsign).toUpper();
    state.loggedIn = true;

    if (sendBannerFlag) {
        sendBanner(state);
    } else {
        state.bannerSent = true;
        if (!state.announced) {
            emit logLine(QString("[CLUSTER] Connexion active depuis %1:%2 - session V10 compat")
                              .arg(state.addr).arg(state.port));
            state.announced = true;
        }
    }
    emit logLine("[CLUSTER] Login accepté : " + state.callsign);
}

void LocalDXClusterServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        configureClient(client);

        ClientState state;
        state.socket = client;
        state.addr = client->peerAddress().toString();
        state.port = client->peerPort();
        state.connectedAt.start();

        state.promptTimer = new QTimer(this);
        state.promptTimer->setSingleShot(true);
        state.compatTimer = new QTimer(this);
        state.compatTimer->setSingleShot(true);

        m_clients.insert(client, state);

        connect(state.promptTimer, &QTimer::timeout, this, [this, client]() {
            auto it = m_clients.find(client);
            if (it == m_clients.end()) return;
            sendLoginPrompt(it.value());
        });
        state.promptTimer->start(LOGIN_PROMPT_DELAY_MS);

        connect(state.compatTimer, &QTimer::timeout, this, [this, client]() {
            auto it = m_clients.find(client);
            if (it == m_clients.end()) return;
            if (!it.value().loggedIn)
                activateClientSession(it.value(), "F4HAB", false);
        });
        state.compatTimer->start(LOGIN_COMPAT_TIMEOUT_MS);

        connect(client, &QTcpSocket::readyRead, this, [this, client]() {
            auto it = m_clients.find(client);
            if (it == m_clients.end()) return;
            ClientState &st = it.value();

            const QByteArray data = client->readAll();
            st.rx.append(data);

            if (!st.loggedIn) {
                const QString callsign = extractCallsignFromBuffer(st.rx);
                if (!callsign.isEmpty()) {
                    activateClientSession(st, callsign, true);
                    st.rx.clear();
                }
            }

            if (st.loggedIn && !st.rx.isEmpty()) {
                QByteArray raw = st.rx;
                raw.replace('\r', '\n');
                const QList<QByteArray> parts = raw.split('\n');
                st.rx = parts.last();
                for (int i = 0; i < parts.size() - 1; ++i) {
                    const QString line = QString::fromLatin1(parts.at(i)).trimmed();
                    if (line.isEmpty())
                        continue;
                    client->write((st.callsign.isEmpty() ? "F4HAB" : st.callsign).toLatin1() +
                                  " de ChallengeDXCC >\r\n");
                }
            }
        });

        connect(client, &QTcpSocket::disconnected, this, [this, client]() {
            removeClient(client);
        });
    }
}

void LocalDXClusterServer::removeClient(QTcpSocket *client)
{
    auto it = m_clients.find(client);
    if (it == m_clients.end())
        return;
    ClientState state = it.value();
    m_clients.erase(it);

    if (state.promptTimer) state.promptTimer->deleteLater();
    if (state.compatTimer) state.compatTimer->deleteLater();

    const qint64 ageMs = state.connectedAt.elapsed();
    if (ageMs < TRANSIENT_CONNECTION_MS && !state.announced) {
        emit logLine("[CLUSTER PROBE] connexion locale brève ignorée");
    } else {
        emit logLine(QString("[CLUSTER] Client déconnecté : %1:%2").arg(state.addr).arg(state.port));
    }
    client->deleteLater();
}

double LocalDXClusterServer::frequencyKhz(const QJsonObject &alert)
{
    bool ok = false;
    double mhz = -1.0;
    const QJsonValue value = alert.value("frequency");
    if (value.isDouble()) {
        mhz = value.toDouble(-1.0);
        ok = mhz >= 0.0;
    } else {
        QString text = bcClean(value.toString());
        text.replace(',', '.');
        mhz = text.toDouble(&ok);
    }
    return ok ? mhz * 1000.0 : -1.0;
}

QString LocalDXClusterServer::formatSpot(const QJsonObject &alert)
{
    const QString rawText = bcClean(alert.value("rawText").toString());
    if (rawText.toUpper().startsWith("DX DE ") && rawText.contains(':'))
        return rawText;

    QString spotter = bcClean(alert.value("spotter").toString());
    if (spotter.isEmpty())
        spotter = bcClean(alert.value("spotterCallsign").toString());
    if (spotter.isEmpty())
        spotter = "HAMALERT";
    spotter = spotter.toUpper();

    QString call = bcClean(alert.value("fullCallsign").toString());
    if (call.isEmpty())
        call = bcClean(alert.value("callsign").toString());
    if (call.isEmpty())
        call = bcClean(alert.value("call").toString());
    call = call.toUpper();

    QString comment = bcClean(alert.value("comment").toString());
    QString mode = bcClean(alert.value("modeDetail").toString());
    if (mode.isEmpty())
        mode = bcClean(alert.value("mode").toString());
    mode = mode.toUpper();

    QString hhmm = bcClean(alert.value("time").toString());
    hhmm.remove(':');
    hhmm = hhmm.left(4);

    const double freqKhz = frequencyKhz(alert);

    if (call.isEmpty() || freqKhz < 0)
        return QString();
    if (comment.isEmpty())
        comment = mode.isEmpty() ? "HamAlert" : mode;
    if (hhmm.length() != 4)
        hhmm = QDateTime::currentDateTimeUtc().toString("hhmm");

    const QString prefix = QString("DX de %1:").arg(spotter);
    const QString left = prefix.leftJustified(16);
    QString line = QString("%1%2  %3 %4 %5Z")
                        .arg(left)
                        .arg(freqKhz, 8, 'f', 1)
                        .arg(call.leftJustified(12).left(12))
                        .arg(comment.left(28).leftJustified(28))
                        .arg(hhmm);
    return line;
}

bool LocalDXClusterServer::broadcastAlert(const QJsonObject &alert)
{
    const QString line = formatSpot(alert);
    if (line.isEmpty())
        return false;

    const QByteArray payload = (line + "\r\n").toLatin1();
    int sent = 0;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        ClientState &state = it.value();
        if (!state.loggedIn || !state.bannerSent)
            continue;
        state.socket->write(payload);
        sent++;
    }

    emit logLine("[CLUSTER TX] " + line);
    emit clusterTx(line);
    // Le tableau DX SPOTS doit être alimenté même si Logger32 n'est pas
    // connecté : l'alerte a déjà franchi les filtres Challenge.
    emit alertBroadcast(alert, line);
    if (sent == 0)
        emit logLine("[CLUSTER] Spot accepté mais aucun client Logger32 actif");
    return sent > 0;
}
