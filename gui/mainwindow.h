#pragma once
// Portage C++ de gui/main_window.py (MainWindow).
//
// Le fichier main_window.ui (Qt Designer) est identique à celui utilisé
// côté Python : il est chargé nativement par uic (AUTOUIC) plutôt que par
// QUiLoader, ce qui donne accès direct aux widgets via Ui::ChallengeDXCCWidget
// sans recherche par nom.

#include "backend/challengebackend.h"
#include "gui/tciclient.h"
#include "gui/servicecard.h"
#include "core/models.h"

#include <QMainWindow>
#include <QSettings>
#include <QMap>
#include <QTableWidget>
#include <QRadioButton>
#include <QJsonObject>
#include <QDateTime>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class ChallengeDXCCWidget; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const QDir &appRoot, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void chooseAdif();
    void applyProfile();
    void restartServices();
    void stopServices();
    void startAutoTune();
    void clearLog();
    void saveLog();
    void bandSelected();
    void tuneSelectedSpot(QTableWidgetItem *item);

    void onBackendLog(const QString &line);
    void onServiceStateChanged(const QString &serviceId, bool active, const QString &detail);
    void onQsoAdded(const QJsonObject &fields, bool challengePeriod);
    void onDxSpotLine(const QString &formattedLine);
    void onDxSpotAccepted(const QJsonObject &alert, const QString &formattedLine);
    void purgeDxSpots();
    void onReportsUpdated();

private:
    void buildServiceCards();
    void configureTables();
    void restoreUiState();
    void saveUiState();

    void refreshBandTable();
    void refreshMissingTable(const QString &band);

    void addLog(const QString &source, const QString &kind, const QString &message,
                const QString &category = QString());
    void addDxSpot(const QJsonObject &alert);
    int findActiveDxRow(const QString &call, const QString &band) const;
    void applyDxSpotBandFilter();
    void sortDxSpotsByColumn(int column);
    void reapplyDxSpotSort();
    static int snrFromAlert(const QJsonObject &alert, bool *ok = nullptr);
    static qint64 frequencyHzFromAlert(const QJsonObject &alert, bool *ok = nullptr);
    bool logCategoryVisible(const QString &category) const;
    void applyProfileFromUi();

    QDir m_root;
    Ui::ChallengeDXCCWidget *m_ui = nullptr;
    QSettings m_settings;

    ChallengeBackend *m_backend = nullptr;
    TciClient *m_tci = nullptr;
    ChallengeProfile m_profile;

    QMap<QString, ServiceCard *> m_cards;
    QString m_spotBandFilter = "all";
    QTimer *m_spotPurgeTimer = nullptr;
    int m_spotSortColumn = -1;
    Qt::SortOrder m_spotSortOrder = Qt::AscendingOrder;
};
