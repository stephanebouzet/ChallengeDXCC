#pragma once
// Orchestrateur C++ : remplace à la fois CommunicationsHub (lanceur de
// sous-processus Python) et Logger32ChallengeV9 (services/v10_backend.py).
// Tous les services réseau tournent nativement dans le processus Qt et
// communiquent par signaux, sans plus passer par un sous-processus ni un
// parsing de stdout.

#include "core/challengeengine.h"
#include "db/challengedb.h"
#include "backend/localdxccresolver.h"
#include "backend/adifimporter.h"
#include "backend/deletedqsowatcher.h"
#include "backend/hamalertclient.h"
#include "backend/localdxclusterserver.h"
#include "backend/pskreportermqttclient.h"
#include "backend/genericdxclusterclient.h"
#include "backend/jtdxudplistener.h"
#include "backend/logger32bridge.h"

#include <QObject>
#include <QDir>
#include <QSet>
#include <QMap>
#include <QTimer>
#include <QJsonObject>

class ChallengeBackend : public QObject
{
    Q_OBJECT
public:
    struct Paths {
        QDir resources; // dxcc_prefixes.txt, dxcc_entities.txt, dxcc_aliases.txt
        QDir config;    // hamalert.ini, dxcluster.json, pskreporter.json
        QDir runtime;   // challenge.sqlite3, rapports, dump suppressions par défaut
        QDir adifImport;
    };

    explicit ChallengeBackend(const Paths &paths, QObject *parent = nullptr);
    ~ChallengeBackend() override;

    ChallengeEngine &engine() { return m_engine; }

    void start(const ChallengeProfile &profile);
    void stop();

    bool logger32Connected() const { return m_logger32->isConnected(); }

signals:
    void logLine(const QString &line);
    void serviceStateChanged(const QString &serviceId, bool active, const QString &detail);
    void qsoAdded(const QJsonObject &fields, bool challengePeriod);
    void dxSpotLine(const QString &formattedLine);
    void dxSpotAccepted(const QJsonObject &alert, const QString &formattedLine);
    void reportsUpdated();

private slots:
    void tryConnectLogger32();
    void pollDeletedWatcher();

private:
    void refreshFilterCache();
    void writeReports();
    bool isMissingSlot(int dxcc, const QString &band) const;

    void ingestFields(AdifFields fields, const QString &source, bool announce);

    void onHamAlertAlert(const QJsonObject &alert);
    void onPskReport(const QJsonObject &report);
    void onDxClusterSpot(const QJsonObject &spot);
    void onLogger32Qso(const AdifFields &fields);
    void onUdpAdif(const QString &adifText, const QString &sourceAddr, const QString &origin);
    void onDeletedDumpPath(const QString &path);

    Paths m_paths;
    ChallengeEngine m_engine;
    ChallengeDB *m_db = nullptr;
    LocalDXCCResolver *m_resolver = nullptr;
    ADIFImporter *m_importer = nullptr;
    DeletedQSOWatcher *m_deletedWatcher = nullptr;

    HamAlertClient *m_hamAlert = nullptr;
    LocalDXClusterServer *m_localCluster = nullptr;
    PSKReporterMQTTClient *m_pskReporter = nullptr;
    GenericDXClusterClient *m_dxCluster = nullptr;
    JtdxUdpListener *m_udpListener = nullptr;
    Logger32Bridge *m_logger32 = nullptr;

    QTimer *m_logger32Timer = nullptr;
    QTimer *m_deletedWatcherTimer = nullptr;

    QSet<int> m_activeDxccCodes;
    QMap<QString, QSet<int>> m_workedByBand; // bande minuscule -> codes DXCC
    QSet<QString> m_challengeBandSet;

    bool m_started = false;
};
