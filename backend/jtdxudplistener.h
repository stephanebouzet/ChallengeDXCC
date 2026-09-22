#pragma once
// Portage C++ de la réception UDP JTDX (services/v10_backend.py, méthodes
// setup_udp/poll_udp/parse_jtdx_header/extract_logged_adif/process_plain_adif_udp).
//
// Émet un texte ADIF brut (un seul enregistrement) à chaque QSO détecté ;
// c'est ChallengeBackend qui se charge ensuite de la résolution DXCC, de
// la déduplication et de l'écriture en base (logique partagée avec les
// autres sources).

#include <QObject>
#include <QUdpSocket>
#include <QSet>
#include <QPair>
#include <QMap>

class JtdxUdpListener : public QObject
{
    Q_OBJECT
public:
    explicit JtdxUdpListener(const QString &bindAddress, quint16 port, QObject *parent = nullptr);

    void start();
    void close();
    bool enabled() const { return m_enabled; }

signals:
    void adifReceived(const QString &adifText, const QString &sourceAddr, const QString &origin);
    void logLine(const QString &line);
    void stateChanged(bool active, const QString &detail);

private slots:
    void onReadyRead();

private:
    static QString extractLoggedAdif(const QByteArray &data);
    static QString decodeFlexible(const QByteArray &data);
    void handleDatagram(const QByteArray &data, const QString &addr);

    static constexpr quint32 JTDX_MAGIC = 0xADBCCBDA;
    static constexpr quint32 JTDX_LOGGED_ADIF = 12;

    QString m_bindAddress;
    quint16 m_port;
    QUdpSocket *m_socket = nullptr;
    bool m_enabled = false;

    quint64 m_packetCount = 0;
    QMap<quint32, quint64> m_typeCounts;
    QSet<quint32> m_seenTypes;
    QSet<QPair<int, QByteArray>> m_unknownSignatures;
};
