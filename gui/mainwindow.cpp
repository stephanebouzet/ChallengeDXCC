#include "gui/mainwindow.h"
#include "ui_main_window.h"
#include "backend/backendcommon.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QColor>
#include <QDateTime>
#include <QApplication>
#include <QFileInfo>
#include <QTextStream>
#include <QCheckBox>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
constexpr int MAX_LOG_ROWS = 500;
constexpr int DX_SPOT_LIFETIME_SECONDS = 60 * 60;
constexpr int DX_SPOT_AGGREGATE_SECONDS = 10 * 60;
constexpr double DIGI_STANDARD_WINDOW_KHZ = 4.0;
constexpr const char *APP_VERSION = "V0.1 C++";

const QMap<QString, double> FT8_STANDARD_KHZ = {
    {"80m", 3573.0}, {"40m", 7074.0}, {"30m", 10136.0}, {"20m", 14074.0},
    {"17m", 18100.0}, {"15m", 21074.0}, {"12m", 24915.0}, {"10m", 28074.0},
};
const QMap<QString, double> FT4_STANDARD_KHZ = {
    {"80m", 3575.0}, {"40m", 7047.5}, {"30m", 10140.0}, {"20m", 14080.0},
    {"17m", 18104.0}, {"15m", 21140.0}, {"12m", 24919.0}, {"10m", 28180.0},
};
}

MainWindow::MainWindow(const QDir &appRoot, QWidget *parent)
    : QMainWindow(parent)
    , m_root(appRoot)
    , m_ui(new Ui::ChallengeDXCCWidget())
    , m_settings("ChallengeDXCC", "QtCpp")
{
    QWidget *central = new QWidget(this);
    m_ui->setupUi(central);
    setCentralWidget(central);

    setWindowTitle(QString("Challenge DXCC - Logger32 / JTDX / HamAlert / PSK Reporter / DX Cluster "
                           "- %1 par F4HAB").arg(APP_VERSION));
    resize(1500, 900);

    // Feuille de style globale.
    QFile qss(m_root.filePath("gui/styles.qss"));
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text))
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));

    buildServiceCards();
    configureTables();

    connect(m_ui->chooseAdifButton, &QPushButton::clicked, this, &MainWindow::chooseAdif);
    connect(m_ui->applyProfileButton, &QPushButton::clicked, this, &MainWindow::applyProfile);
    connect(m_ui->restartServicesButton, &QPushButton::clicked, this, &MainWindow::restartServices);
    connect(m_ui->stopServicesButton, &QPushButton::clicked, this, &MainWindow::stopServices);
    connect(m_ui->tuneAutoButton, &QPushButton::clicked, this, &MainWindow::startAutoTune);
    connect(m_ui->clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
    connect(m_ui->saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);
    connect(m_ui->band, &QTableWidget::itemSelectionChanged, this, &MainWindow::bandSelected);
    connect(m_ui->spots, &QTableWidget::itemDoubleClicked, this, &MainWindow::tuneSelectedSpot);

    ChallengeBackend::Paths paths;
    paths.resources = QDir(m_root.filePath("resources"));
    paths.config = QDir(m_root.filePath("config"));
    paths.runtime = QDir(m_root.filePath("runtime"));
    paths.adifImport = QDir(m_root.filePath("runtime/import_adif"));
    paths.adifImport.mkpath(".");

    m_backend = new ChallengeBackend(paths, this);
    connect(m_backend, &ChallengeBackend::logLine, this, &MainWindow::onBackendLog);
    connect(m_backend, &ChallengeBackend::serviceStateChanged, this, &MainWindow::onServiceStateChanged);
    connect(m_backend, &ChallengeBackend::qsoAdded, this, &MainWindow::onQsoAdded);
    connect(m_backend, &ChallengeBackend::dxSpotLine, this, &MainWindow::onDxSpotLine);
    connect(m_backend, &ChallengeBackend::dxSpotAccepted, this, &MainWindow::onDxSpotAccepted);
    connect(m_backend, &ChallengeBackend::reportsUpdated, this, &MainWindow::onReportsUpdated);

    m_tci = new TciClient("127.0.0.1", 50001, 0, 0, this);
    m_tci->start();

    m_spotPurgeTimer = new QTimer(this);
    m_spotPurgeTimer->setInterval(30 * 1000);
    connect(m_spotPurgeTimer, &QTimer::timeout, this, &MainWindow::purgeDxSpots);
    m_spotPurgeTimer->start();

    // ------------------ Profil : chargé depuis QSettings / exemple ------------------
    QFile exampleFile(m_root.filePath("profiles/example_challenge.json"));
    QJsonObject example;
    if (exampleFile.open(QIODevice::ReadOnly))
        example = QJsonDocument::fromJson(exampleFile.readAll()).object();

    const QString name = m_settings.value("profile/name", example.value("name").toString("Challenge DXCC")).toString();
    const QString adif = m_settings.value("profile/adif", example.value("adif_path").toString()).toString();
    const QString start = m_settings.value("profile/start", example.value("start_date").toString("2025-09-01")).toString();
    const QString end = m_settings.value("profile/end", example.value("end_date").toString("2026-08-31")).toString();

    m_ui->nameEdit->setText(name);
    m_ui->adifEdit->setText(adif);
    m_ui->startEdit->setDate(QDate::fromString(start, "yyyy-MM-dd"));
    m_ui->endEdit->setDate(QDate::fromString(end, "yyyy-MM-dd"));

    applyProfileFromUi();
    restoreUiState();
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

void MainWindow::buildServiceCards()
{
    auto *layout = m_ui->servicesCardsHost->layout();
    if (!layout) {
        layout = new QVBoxLayout(m_ui->servicesCardsHost);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
    }

    const QVector<QPair<QString, QString>> defs = {
        {"HamAlert", "hamalert.org:7300"},
        {"PSK Reporter", "MQTT direct · FT8/FT4"},
        {"DX Cluster", "Europe prioritaire · Telnet"},
        {"Cluster Local", "0.0.0.0:7301"},
        {"JTDX UDP", "récepteur 127.0.0.1:2238"},
        {"Logger32", "interface externe Windows"},
    };
    for (const auto &d : defs) {
        auto *card = new ServiceCard(d.first, d.second, m_ui->servicesCardsHost);
        m_cards[d.first] = card;
        layout->addWidget(card);
    }
    static_cast<QVBoxLayout *>(layout)->addStretch(1);
}

void MainWindow::configureTables()
{
    m_ui->band->verticalHeader()->setVisible(false);
    m_ui->band->verticalHeader()->setDefaultSectionSize(25);
    m_ui->band->setColumnWidth(0, 54);
    m_ui->band->setColumnWidth(1, 70);
    m_ui->band->setColumnWidth(2, 74);
    m_ui->band->setColumnWidth(3, 150);
    m_ui->band->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->band->horizontalHeader()->setStretchLastSection(false);
    m_ui->band->setStyleSheet("QTableWidget::item:selected {background-color:#DCEEFF;color:#202020;}");

    m_ui->missing->verticalHeader()->setVisible(false);
    m_ui->missing->verticalHeader()->setDefaultSectionSize(23);
    m_ui->missing->setSortingEnabled(true);
    m_ui->missing->horizontalHeader()->setSortIndicatorShown(true);
    m_ui->missing->horizontalHeader()->setSectionsClickable(true);
    m_ui->missing->setColumnWidth(0, 48);
    m_ui->missing->setColumnWidth(1, 52);
    m_ui->missing->setColumnWidth(2, 180);
    m_ui->missing->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->missing->horizontalHeader()->setStretchLastSection(false);

    m_ui->spots->verticalHeader()->setVisible(false);
    m_ui->spots->verticalHeader()->setDefaultSectionSize(22);
    m_ui->spots->setColumnWidth(0, 42);
    m_ui->spots->setColumnWidth(1, 75);
    m_ui->spots->setColumnWidth(2, 110);
    m_ui->spots->setColumnWidth(3, 52);
    m_ui->spots->setColumnWidth(4, 48);
    m_ui->spots->setColumnWidth(5, 48);
    m_ui->spots->setColumnWidth(6, 70);
    m_ui->spots->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->spots->horizontalHeader()->setStretchLastSection(false);
    m_ui->spots->horizontalHeader()->setSectionsClickable(true);
    m_ui->spots->horizontalHeader()->setSortIndicatorShown(false);
    m_ui->spots->setSortingEnabled(false);
    connect(m_ui->spots->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &MainWindow::sortDxSpotsByColumn);
    m_ui->spots->setStyleSheet("QTableWidget::item:selected {background-color:#DCEEFF;color:#202020;}");

    m_ui->log->verticalHeader()->setVisible(false);
    m_ui->log->verticalHeader()->setDefaultSectionSize(23);
    m_ui->log->setColumnWidth(0, 76);
    m_ui->log->setColumnWidth(1, 105);
    m_ui->log->setColumnWidth(2, 72);
    m_ui->log->setColumnWidth(3, 420);
    m_ui->log->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_ui->log->horizontalHeader()->setStretchLastSection(false);

    const QMap<QString, QRadioButton *> radios = {
        {"all", m_ui->spotBandAll}, {"160m", m_ui->spotBand160}, {"80m", m_ui->spotBand80},
        {"40m", m_ui->spotBand40}, {"30m", m_ui->spotBand30}, {"20m", m_ui->spotBand20},
        {"17m", m_ui->spotBand17}, {"15m", m_ui->spotBand15}, {"12m", m_ui->spotBand12},
        {"10m", m_ui->spotBand10},
    };
    m_spotBandFilter = m_settings.value("ui/dxSpotBandFilter", "all").toString();
    if (!radios.contains(m_spotBandFilter))
        m_spotBandFilter = "all";
    radios.value(m_spotBandFilter)->setChecked(true);

    for (auto it = radios.constBegin(); it != radios.constEnd(); ++it) {
        const QString key = it.key();
        QRadioButton *radio = it.value();
        connect(radio, &QRadioButton::toggled, this, [this, key](bool checked) {
            if (!checked)
                return;
            m_spotBandFilter = key;
            m_settings.setValue("ui/dxSpotBandFilter", key);
            applyDxSpotBandFilter();
        });
    }
}

void MainWindow::restoreUiState()
{
    if (m_settings.contains("ui/geometry"))
        restoreGeometry(m_settings.value("ui/geometry").toByteArray());
    if (m_settings.contains("ui/windowState"))
        restoreState(m_settings.value("ui/windowState").toByteArray());
    if (m_settings.contains("ui/topSplitter"))
        m_ui->topSplitter->restoreState(m_settings.value("ui/topSplitter").toByteArray());
    if (m_settings.contains("ui/mainSplitter"))
        m_ui->mainSplitter->restoreState(m_settings.value("ui/mainSplitter").toByteArray());
    if (m_settings.contains("ui/journalColumnsSplitter"))
        m_ui->journalSplitter->restoreState(m_settings.value("ui/journalColumnsSplitter").toByteArray());

    const struct { const char *key; QTableWidget *table; } headers[] = {
        {"ui/bandHeader", m_ui->band}, {"ui/missingHeader", m_ui->missing},
        {"ui/spotsHeaderV2", m_ui->spots}, {"ui/logHeader", m_ui->log},
    };
    for (const auto &h : headers)
        if (m_settings.contains(h.key))
            h.table->horizontalHeader()->restoreState(m_settings.value(h.key).toByteArray());

    const QString selectedBand = m_settings.value("ui/selectedBand").toString();
    if (!selectedBand.isEmpty()) {
        for (int row = 0; row < m_ui->band->rowCount(); ++row) {
            if (m_ui->band->item(row, 0) && m_ui->band->item(row, 0)->text() == selectedBand) {
                m_ui->band->selectRow(row);
                break;
            }
        }
    }
}

void MainWindow::saveUiState()
{
    m_settings.setValue("ui/geometry", saveGeometry());
    m_settings.setValue("ui/windowState", saveState());
    m_settings.setValue("ui/topSplitter", m_ui->topSplitter->saveState());
    m_settings.setValue("ui/mainSplitter", m_ui->mainSplitter->saveState());
    m_settings.setValue("ui/journalColumnsSplitter", m_ui->journalSplitter->saveState());
    m_settings.setValue("ui/bandHeader", m_ui->band->horizontalHeader()->saveState());
    m_settings.setValue("ui/missingHeader", m_ui->missing->horizontalHeader()->saveState());
    m_settings.setValue("ui/spotsHeaderV2", m_ui->spots->horizontalHeader()->saveState());
    m_settings.setValue("ui/logHeader", m_ui->log->horizontalHeader()->saveState());

    const int row = m_ui->band->currentRow();
    if (row >= 0 && m_ui->band->item(row, 0))
        m_settings.setValue("ui/selectedBand", m_ui->band->item(row, 0)->text());

    m_settings.sync();
}

void MainWindow::chooseAdif()
{
    QString startDir = m_root.absolutePath();
    const QString current = m_ui->adifEdit->text().trimmed();
    if (!current.isEmpty()) {
        QFileInfo fi(current);
        if (fi.dir().exists())
            startDir = fi.dir().absolutePath();
    }
    const QString filename = QFileDialog::getOpenFileName(
        this, "Choisir l'ADIF de référence", startDir, "ADIF (*.adi *.adif);;Tous les fichiers (*)");
    if (!filename.isEmpty()) {
        m_ui->adifEdit->setText(filename);
        applyProfile();
    }
}

void MainWindow::applyProfileFromUi()
{
    m_profile.name = m_ui->nameEdit->text().trimmed().isEmpty() ? "Challenge DXCC" : m_ui->nameEdit->text().trimmed();
    m_profile.adifPath = m_ui->adifEdit->text().trimmed();
    m_profile.startDate = m_ui->startEdit->date().toString("yyyy-MM-dd");
    m_profile.endDate = m_ui->endEdit->date().toString("yyyy-MM-dd");
    m_profile.bands = defaultBands();

    m_backend->engine().loadProfile(m_profile);

    m_settings.setValue("profile/name", m_profile.name);
    m_settings.setValue("profile/adif", m_profile.adifPath);
    m_settings.setValue("profile/start", m_profile.startDate);
    m_settings.setValue("profile/end", m_profile.endDate);
    m_settings.sync();

    refreshBandTable();
    addLog("Challenge", "INFO",
           QString("%1 QSO chargés - %2 non résolus")
               .arg(m_backend->engine().qsoCount()).arg(m_backend->engine().unresolved()),
           "info");
}

void MainWindow::applyProfile()
{
    applyProfileFromUi();
    m_backend->start(m_profile); // idempotent : ne redémarre pas si déjà lancé
}

void MainWindow::restartServices()
{
    m_backend->stop();
    m_backend->start(m_profile);
}

void MainWindow::stopServices()
{
    m_backend->stop();
    for (auto *card : m_cards)
        card->setConnected(false);
}

void MainWindow::startAutoTune()
{
    // V0.1 : Tune Auto reste volontairement à porter. Dans la version
    // Python il pilote ExpertSDR directement via TCI et télémétrie PWR/ROS ;
    // il ne dépend pas de JTDX.
    QMessageBox::information(this, "Tune Auto",
                              "Tune Auto TCI n'est pas encore porté dans la V0.1 C++.\n"
                              "Le QSY par double-clic via TCI est disponible.");
}

void MainWindow::clearLog()
{
    m_ui->log->setRowCount(0);
}

void MainWindow::saveLog()
{
    const QString filename = QFileDialog::getSaveFileName(
        this, "Enregistrer le journal", m_root.filePath("journal.txt"), "Texte (*.txt)");
    if (filename.isEmpty())
        return;
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    for (int row = 0; row < m_ui->log->rowCount(); ++row) {
        QStringList cells;
        for (int col = 0; col < m_ui->log->columnCount(); ++col) {
            auto *item = m_ui->log->item(row, col);
            cells << (item ? item->text() : QString());
        }
        out << cells.join("  |  ") << "\n";
    }
}

static double progressColorHue(double percent)
{
    const double p = qBound(0.0, percent, 100.0) / 100.0;
    return 120.0 * p;
}

void MainWindow::refreshBandTable()
{
    const auto states = m_backend->engine().bandStates();
    m_ui->band->setRowCount(states.size());
    for (int row = 0; row < states.size(); ++row) {
        const BandState &s = states.at(row);
        auto *bandItem = new QTableWidgetItem(s.band);
        auto *workedItem = new NumericTableWidgetItem(QString::number(s.worked));
        workedItem->setData(Qt::UserRole, s.worked);
        auto *missingItem = new NumericTableWidgetItem(QString::number(s.missing));
        missingItem->setData(Qt::UserRole, s.missing);

        auto *progressItem = new QTableWidgetItem(QString("%1%").arg(s.percent(), 0, 'f', 1));
        const QColor color = QColor::fromHsv(int(progressColorHue(s.percent())), 205, 190);
        progressItem->setBackground(color);

        m_ui->band->setItem(row, 0, bandItem);
        m_ui->band->setItem(row, 1, workedItem);
        m_ui->band->setItem(row, 2, missingItem);
        m_ui->band->setItem(row, 3, progressItem);
    }
}

void MainWindow::bandSelected()
{
    const int row = m_ui->band->currentRow();
    if (row < 0 || !m_ui->band->item(row, 0)) {
        m_ui->missing->setRowCount(0);
        return;
    }
    refreshMissingTable(m_ui->band->item(row, 0)->text());
}

void MainWindow::refreshMissingTable(const QString &band)
{
    const auto missing = m_backend->engine().missingEntities(band);
    m_ui->missing->setSortingEnabled(false);
    m_ui->missing->setRowCount(missing.size());
    for (int row = 0; row < missing.size(); ++row) {
        const MissingEntity &e = missing.at(row);
        auto *dxccItem = new NumericTableWidgetItem(QString::number(e.dxcc));
        dxccItem->setData(Qt::UserRole, e.dxcc);
        m_ui->missing->setItem(row, 0, dxccItem);
        m_ui->missing->setItem(row, 1, new QTableWidgetItem(e.prefix));
        m_ui->missing->setItem(row, 2, new QTableWidgetItem(e.name));
    }
    m_ui->missing->setSortingEnabled(true);
    m_ui->missingTitle->setText(QString("DXCC manquants - %1 (%2)").arg(band).arg(missing.size()));
}

bool MainWindow::logCategoryVisible(const QString &category) const
{
    static const QMap<QString, QCheckBox *> map = {
        {"hamalert_rx", m_ui->filterHamAlert}, {"psk_rx", m_ui->filterPSK},
        {"dxcluster_rx", m_ui->filterDXCluster}, {"cluster_tx", m_ui->filterClusterTX},
        {"cluster_client", m_ui->filterClusterConnection}, {"accept", m_ui->filterAccept},
        {"reject", m_ui->filterReject}, {"qso", m_ui->filterQSO},
        {"database", m_ui->filterDatabase}, {"info", m_ui->filterSystem},
    };
    if (!map.contains(category))
        return true;
    return map.value(category)->isChecked();
}

void MainWindow::addLog(const QString &source, const QString &kind, const QString &message,
                         const QString &category)
{
    if (!category.isEmpty() && !logCategoryVisible(category))
        return;

    const int row = 0;
    m_ui->log->insertRow(row);
    m_ui->log->setItem(row, 0, new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));
    m_ui->log->setItem(row, 1, new QTableWidgetItem(source));
    m_ui->log->setItem(row, 2, new QTableWidgetItem(kind));
    m_ui->log->setItem(row, 3, new QTableWidgetItem(message));

    while (m_ui->log->rowCount() > MAX_LOG_ROWS)
        m_ui->log->removeRow(m_ui->log->rowCount() - 1);
}

qint64 MainWindow::frequencyHzFromAlert(const QJsonObject &alert, bool *okOut)
{
    bool ok = false;
    qint64 hz = 0;

    // PSK Reporter fournit déjà des Hz ; les alertes normalisées fournissent
    // généralement "frequency" en MHz, comme la version Python.
    if (alert.contains("frequency_hz")) {
        hz = alert.value("frequency_hz").toVariant().toLongLong(&ok);
    }
    if (!ok && alert.contains("frequency")) {
        double mhz = -1.0;
        if (alert.value("frequency").isDouble()) {
            mhz = alert.value("frequency").toDouble(-1.0);
            ok = mhz >= 0.0;
        } else {
            QString text = bcClean(alert.value("frequency").toString());
            text.replace(',', '.');
            mhz = text.toDouble(&ok);
        }
        if (ok)
            hz = qRound64(mhz * 1000000.0);
    }
    if (!ok && alert.contains("frequency_khz")) {
        const double khz = alert.value("frequency_khz").toDouble(-1.0);
        ok = khz >= 0.0;
        if (ok)
            hz = qRound64(khz * 1000.0);
    }

    if (okOut)
        *okOut = ok;
    return ok ? hz : 0;
}

int MainWindow::snrFromAlert(const QJsonObject &alert, bool *okOut)
{
    bool ok = false;
    int snr = 0;

    if (alert.contains("snr")) {
        snr = alert.value("snr").toVariant().toInt(&ok);
    }
    if (!ok) {
        const QString comment = bcClean(alert.value("comment").toString());
        static const QRegularExpression re(QStringLiteral("([+-]?\\d+)\\s*dB\\b"),
                                            QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(comment);
        if (m.hasMatch()) {
            snr = m.captured(1).toInt(&ok);
        }
    }

    if (okOut)
        *okOut = ok;
    return snr;
}

int MainWindow::findActiveDxRow(const QString &call, const QString &band) const
{
    for (int row = 0; row < m_ui->spots->rowCount(); ++row) {
        const QTableWidgetItem *callItem = m_ui->spots->item(row, 1);
        const QTableWidgetItem *bandItem = m_ui->spots->item(row, 3);
        if (!callItem || !bandItem)
            continue;
        if (callItem->text().trimmed().compare(call, Qt::CaseInsensitive) == 0 &&
            bandItem->text().trimmed().compare(band, Qt::CaseInsensitive) == 0)
            return row;
    }
    return -1;
}

void MainWindow::applyDxSpotBandFilter()
{
    for (int row = 0; row < m_ui->spots->rowCount(); ++row) {
        const QString band = m_ui->spots->item(row, 3)
                                 ? m_ui->spots->item(row, 3)->text().trimmed().toLower()
                                 : QString();
        m_ui->spots->setRowHidden(row, m_spotBandFilter != "all" && band != m_spotBandFilter);
    }
}

void MainWindow::sortDxSpotsByColumn(int column)
{
    if (column < 0 || column >= m_ui->spots->columnCount())
        return;

    if (m_spotSortColumn == column) {
        m_spotSortOrder = (m_spotSortOrder == Qt::AscendingOrder)
                              ? Qt::DescendingOrder
                              : Qt::AscendingOrder;
    } else {
        m_spotSortColumn = column;
        // Comme la version Python : Vu et SNR forts en premier par défaut.
        m_spotSortOrder = (column == 0 || column == 5)
                              ? Qt::DescendingOrder
                              : Qt::AscendingOrder;
    }

    m_ui->spots->horizontalHeader()->setSortIndicatorShown(true);
    m_ui->spots->horizontalHeader()->setSortIndicator(m_spotSortColumn, m_spotSortOrder);
    m_ui->spots->sortItems(m_spotSortColumn, m_spotSortOrder);
    applyDxSpotBandFilter();
}

void MainWindow::reapplyDxSpotSort()
{
    if (m_spotSortColumn >= 0)
        m_ui->spots->sortItems(m_spotSortColumn, m_spotSortOrder);
}

void MainWindow::purgeDxSpots()
{
    const qint64 cutoff = QDateTime::currentSecsSinceEpoch() - DX_SPOT_LIFETIME_SECONDS;
    for (int row = m_ui->spots->rowCount() - 1; row >= 0; --row) {
        const QTableWidgetItem *callItem = m_ui->spots->item(row, 1);
        bool ok = false;
        const qint64 ts = callItem ? callItem->data(Qt::UserRole).toLongLong(&ok) : 0;
        if (!ok || ts < cutoff)
            m_ui->spots->removeRow(row);
    }
}

void MainWindow::addDxSpot(const QJsonObject &alert)
{
    QString call = bcClean(alert.value("fullCallsign").toString()).toUpper();
    if (call.isEmpty())
        call = bcClean(alert.value("callsign").toString()).toUpper();
    if (call.isEmpty())
        call = bcClean(alert.value("call").toString()).toUpper();

    const QString band = bcClean(alert.value("band").toString()).toLower();
    QString mode = bcClean(alert.value("modeDetail").toString()).toUpper();
    if (mode.isEmpty())
        mode = bcClean(alert.value("mode").toString()).toUpper();
    if (mode.isEmpty()) {
        const QString comment = bcClean(alert.value("comment").toString()).toUpper();
        if (comment.contains("FT8")) mode = "FT8";
        else if (comment.contains("FT4")) mode = "FT4";
        else mode = "DIGI";
    }

    const QString country = bcClean(alert.value("entity").toString());
    bool freqOk = false;
    const qint64 frequencyHz = frequencyHzFromAlert(alert, &freqOk);
    if (call.isEmpty() || band.isEmpty() || !freqOk)
        return;

    bool dxccOk = false;
    const int dxcc = alert.value("dxcc").toVariant().toInt(&dxccOk);
    bool snrOk = false;
    const int snr = snrFromAlert(alert, &snrOk);
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    int count = 1;
    int bestSnr = snr;
    bool bestSnrOk = snrOk;

    const int oldRow = findActiveDxRow(call, band);
    if (oldRow >= 0) {
        QTableWidgetItem *oldCall = m_ui->spots->item(oldRow, 1);
        bool tsOk = false;
        const qint64 oldTs = oldCall ? oldCall->data(Qt::UserRole).toLongLong(&tsOk) : 0;
        const bool aggregate = tsOk && (now - oldTs) <= DX_SPOT_AGGREGATE_SECONDS;

        if (aggregate && oldCall) {
            bool countOk = false;
            const int oldCount = oldCall->data(Qt::UserRole + 2).toInt(&countOk);
            count = countOk ? oldCount + 1 : 2;

            bool oldSnrOk = false;
            const int oldBest = oldCall->data(Qt::UserRole + 3).toInt(&oldSnrOk);
            if (oldSnrOk) {
                bestSnr = snrOk ? qMax(oldBest, snr) : oldBest;
                bestSnrOk = true;
            }
        }
        // Une seule ligne par indicatif/bande : au-delà de 10 min, un nouveau
        // cycle de comptage démarre au lieu de créer un doublon.
        m_ui->spots->removeRow(oldRow);
    }

    m_ui->spots->insertRow(0);

    auto *seenItem = new NumericTableWidgetItem(QString::number(count));
    seenItem->setData(Qt::UserRole, count);
    seenItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ui->spots->setItem(0, 0, seenItem);

    auto *callItem = new QTableWidgetItem(call);
    callItem->setData(Qt::UserRole, now);                    // dernière observation
    callItem->setData(Qt::UserRole + 1, frequencyHz);        // QSY
    callItem->setData(Qt::UserRole + 2, count);              // compteur Vu
    if (bestSnrOk)
        callItem->setData(Qt::UserRole + 3, bestSnr);        // meilleur SNR
    if (dxccOk)
        callItem->setData(Qt::UserRole + 4, dxcc);           // suppression après QSO
    callItem->setData(Qt::UserRole + 5, mode);               // mode du spot
    m_ui->spots->setItem(0, 1, callItem);

    m_ui->spots->setItem(0, 2, new QTableWidgetItem(country));

    auto *bandItem = new QTableWidgetItem(band);
    bandItem->setTextAlignment(Qt::AlignCenter);
    m_ui->spots->setItem(0, 3, bandItem);

    auto *modeItem = new QTableWidgetItem(mode);
    modeItem->setTextAlignment(Qt::AlignCenter);
    m_ui->spots->setItem(0, 4, modeItem);

    auto *snrItem = new NumericTableWidgetItem(bestSnrOk ? QString("%1%2").arg(bestSnr >= 0 ? "+" : "").arg(bestSnr)
                                                           : QString());
    snrItem->setData(Qt::UserRole, bestSnrOk ? bestSnr : -999);
    snrItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ui->spots->setItem(0, 5, snrItem);

    auto *freqItem = new NumericTableWidgetItem(QString::number(frequencyHz / 1000.0, 'f', 1));
    freqItem->setData(Qt::UserRole, frequencyHz / 100); // dixième de kHz, tri entier
    freqItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_ui->spots->setItem(0, 6, freqItem);

    purgeDxSpots();
    reapplyDxSpotSort();
    applyDxSpotBandFilter();
}

void MainWindow::tuneSelectedSpot(QTableWidgetItem *item)
{
    if (!item)
        return;
    const int row = item->row();
    auto *callItem = m_ui->spots->item(row, 1);
    auto *modeItem = m_ui->spots->item(row, 4);
    if (!callItem)
        return;

    const QString call = callItem->text().trimmed().toUpper();
    const QString mode = modeItem ? modeItem->text().trimmed().toUpper()
                                  : callItem->data(Qt::UserRole + 5).toString().toUpper();
    if (mode != "FT8" && mode != "FT4") {
        addLog("TCI", "IGNORÉ",
               QString("%1 : double-clic réservé aux spots FT8/FT4").arg(call), "info");
        return;
    }

    const qint64 spotHz = callItem->data(Qt::UserRole + 1).toLongLong();
    const double spotKhz = spotHz / 1000.0;
    const QString band = m_ui->spots->item(row, 3)
                             ? m_ui->spots->item(row, 3)->text().trimmed().toLower()
                             : QString();

    const QMap<QString, double> &standardMap = (mode == "FT8") ? FT8_STANDARD_KHZ
                                                                 : FT4_STANDARD_KHZ;
    if (!standardMap.contains(band)) {
        addLog("TCI", "ERREUR", QString("%1 : aucune fréquence standard %2 configurée pour %3")
                                      .arg(call, mode, band), "info");
        return;
    }

    const double standardKhz = standardMap.value(band);
    const bool nearStandard = qAbs(spotKhz - standardKhz) <= DIGI_STANDARD_WINDOW_KHZ;
    const double targetKhz = nearStandard ? standardKhz : spotKhz;
    const qint64 targetHz = qRound64(targetKhz * 1000.0);

    const auto result = m_tci->setFrequency(targetHz);
    if (result.first) {
        addLog("TCI", "QSY",
               QString("%1 %2 - spot %3 kHz -> %4 kHz (%5)")
                   .arg(call, mode)
                   .arg(spotKhz, 0, 'f', 1)
                   .arg(targetKhz, 0, 'f', 1)
                   .arg(nearStandard ? "fréquence standard" : "fréquence spéciale / DXpedition"),
               "info");
    } else {
        addLog("TCI", "ERREUR", QString("%1 : QSY impossible : %2").arg(call, result.second), "info");
    }
}

void MainWindow::onBackendLog(const QString &line)
{
    addLog("Backend", "INFO", line, "info");
}

void MainWindow::onServiceStateChanged(const QString &serviceId, bool active, const QString &detail)
{
    static const QMap<QString, QString> idToName = {
        {"hamalert", "HamAlert"}, {"pskreporter", "PSK Reporter"}, {"dxcluster", "DX Cluster"},
        {"localcluster", "Cluster Local"}, {"jtdx", "JTDX UDP"}, {"logger32", "Logger32"},
    };
    const QString name = idToName.value(serviceId);
    if (name.isEmpty() || !m_cards.contains(name))
        return;
    m_cards.value(name)->setConnected(active, detail);
}

void MainWindow::onQsoAdded(const QJsonObject &fields, bool challengePeriod)
{
    bool ok = false;
    int dxcc = fields.value("DXCC").toVariant().toInt(&ok);
    if (!ok)
        dxcc = fields.value("DXCC").toString().toInt(&ok);
    const QString band = fields.value("BAND").toString().trimmed().toLower();

    bool changed = false;
    if (ok && challengePeriod)
        changed = m_backend->engine().markWorkedLive(dxcc, band);

    int removed = 0;
    if (ok && challengePeriod) {
        for (int row = m_ui->spots->rowCount() - 1; row >= 0; --row) {
            const QTableWidgetItem *callItem = m_ui->spots->item(row, 1);
            const QTableWidgetItem *bandItem = m_ui->spots->item(row, 3);
            if (!callItem || !bandItem ||
                bandItem->text().trimmed().compare(band, Qt::CaseInsensitive) != 0)
                continue;
            bool spotDxccOk = false;
            const int spotDxcc = callItem->data(Qt::UserRole + 4).toInt(&spotDxccOk);
            if (spotDxccOk && spotDxcc == dxcc) {
                m_ui->spots->removeRow(row);
                ++removed;
            }
        }
    }

    refreshBandTable();
    if (m_ui->band->currentRow() >= 0 && m_ui->band->item(m_ui->band->currentRow(), 0) &&
        m_ui->band->item(m_ui->band->currentRow(), 0)->text().compare(band, Qt::CaseInsensitive) == 0) {
        refreshMissingTable(band);
    }

    addLog(bcClean(fields.value("_SOURCE").toString()), "QSO",
           QString("%1 DXCC=%2 %3 - slot %4, %5 spot(s) retiré(s)")
               .arg(bcClean(fields.value("CALL").toString()))
               .arg(ok ? QString::number(dxcc) : "?")
               .arg(band)
               .arg(changed ? "ajouté" : "déjà travaillé")
               .arg(removed),
           "qso");
}

void MainWindow::onDxSpotLine(const QString &formattedLine)
{
    // Ligne effectivement envoyée au serveur DX Cluster local.
    addLog("Cluster", "TX", formattedLine, "cluster_tx");
}

void MainWindow::onDxSpotAccepted(const QJsonObject &alert, const QString &formattedLine)
{
    Q_UNUSED(formattedLine);
    addDxSpot(alert);
}

void MainWindow::onReportsUpdated()
{
    refreshBandTable();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveUiState();
    m_backend->stop();
    m_tci->stop();
    QMainWindow::closeEvent(event);
}
