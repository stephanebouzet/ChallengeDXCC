#include "backend/jtdxudplistener.h"

#include <QNetworkDatagram>
#include <QStringDecoder>
#include <QDataStream>
#include <algorithm>

JtdxUdpListener::JtdxUdpListener(const QString &bindAddress, quint16 port, QObject *parent)
    : QObject(parent)
    , m_bindAddress(bindAddress)
    , m_port(port)
{
}

void JtdxUdpListener::start()
{
    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(QHostAddress(m_bindAddress), m_port, QUdpSocket::ShareAddress)) {
        emit logLine(QString("[WARN] Impossible d'ouvrir UDP %1:%2 : %3")
                          .arg(m_bindAddress).arg(m_port).arg(m_socket->errorString()));
        m_enabled = false;
        emit stateChanged(false, QString("%1:%2 - erreur bind").arg(m_bindAddress).arg(m_port));
        return;
    }
    connect(m_socket, &QUdpSocket::readyRead, this, &JtdxUdpListener::onReadyRead);
    m_enabled = true;
    emit logLine(QString("UDP JTDX      : %1:%2 (écoute active)").arg(m_bindAddress).arg(m_port));
    emit stateChanged(true, QString("%1:%2 - écoute active").arg(m_bindAddress).arg(m_port));
}

void JtdxUdpListener::close()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_enabled = false;
    emit stateChanged(false, QString("%1:%2").arg(m_bindAddress).arg(m_port));
}

QString JtdxUdpListener::decodeFlexible(const QByteArray &data)
{
    {
        QStringDecoder decoder(QStringConverter::Utf8);
        QString text = decoder.decode(data);
        if (!decoder.hasError())
            return text;
    }
    {
        QStringDecoder decoder("Windows-1252");
        QString text = decoder.decode(data);
        if (!decoder.hasError())
            return text;
    }
    QStringDecoder latin1(QStringConverter::Latin1);
    return latin1.decode(data);
}

QString JtdxUdpListener::extractLoggedAdif(const QByteArray &data)
{
    const QByteArray upper = data.toUpper();
    QVector<int> candidates;
    for (const QByteArray &marker : {QByteArray("<ADIF_VER:"), QByteArray("<PROGRAMID:"), QByteArray("<CALL:")}) {
        const int pos = upper.indexOf(marker);
        if (pos >= 0)
            candidates.append(pos);
    }
    if (candidates.isEmpty())
        return QString();

    const int start = *std::min_element(candidates.begin(), candidates.end());
    QByteArray raw = data.mid(start);

    const int eor = raw.toUpper().indexOf("<EOR>");
    if (eor >= 0)
        raw = raw.left(eor + 5);

    return decodeFlexible(raw);
}

void JtdxUdpListener::handleDatagram(const QByteArray &data, const QString &addr)
{
    // En-tête réseau JTDX/WSJT-X : quint32 magic, quint32 schema, quint32 type (big-endian).
    if (data.size() >= 12) {
        QDataStream ds(data);
        ds.setByteOrder(QDataStream::BigEndian);
        quint32 magic = 0, schema = 0, msgType = 0;
        ds >> magic >> schema >> msgType;

        if (magic == JTDX_MAGIC) {
            m_packetCount++;
            m_typeCounts[msgType] = m_typeCounts.value(msgType) + 1;

            if (!m_seenTypes.contains(msgType)) {
                m_seenTypes.insert(msgType);
                emit logLine(QString("[UDP JTDX] trafic détecté : type=%1, schema=%2, source=%3")
                                  .arg(msgType).arg(schema).arg(addr));
            }

            if (msgType != JTDX_LOGGED_ADIF)
                return;

            const QString adifText = extractLoggedAdif(data);
            if (adifText.isEmpty()) {
                emit logLine(QString("[UDP JTDX] Type 12 reçu, mais ADIF introuvable (schema=%1)").arg(schema));
                return;
            }
            emit adifReceived(adifText, addr, "jtdx_udp");
            return;
        }
    }

    // Pas d'en-tête binaire reconnu : JTDX peut envoyer directement un
    // enregistrement ADIF texte sur le serveur UDP secondaire.
    const QString text = decodeFlexible(data).trimmed();
    if (text.startsWith('<') && text.toUpper().contains("<EOR>")) {
        emit adifReceived(text, addr, "jtdx_udp_adif");
        return;
    }

    emit logLine(QString("[UDP] Datagramme inconnu reçu de %1 (%2 octets)").arg(addr).arg(data.size()));
}

void JtdxUdpListener::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket->receiveDatagram();
        const QString addr = QString("%1:%2").arg(datagram.senderAddress().toString()).arg(datagram.senderPort());
        handleDatagram(datagram.data(), addr);
    }
}
