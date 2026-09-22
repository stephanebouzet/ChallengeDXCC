/********************************************************************************
** Form generated from reading UI file 'main_window.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ChallengeDXCCWidget
{
public:
    QVBoxLayout *rootLayout;
    QSplitter *mainSplitter;
    QSplitter *topSplitter;
    QWidget *challengePanel;
    QVBoxLayout *challengePanelLayout;
    QGroupBox *challengeGroup;
    QGridLayout *challengeFormLayout;
    QLabel *nameLabel;
    QLineEdit *nameEdit;
    QLabel *adifLabel;
    QLineEdit *adifEdit;
    QPushButton *chooseAdifButton;
    QLabel *startLabel;
    QDateEdit *startEdit;
    QLabel *endLabel;
    QDateEdit *endEdit;
    QPushButton *applyProfileButton;
    QTableWidget *band;
    QWidget *missingPanel;
    QVBoxLayout *missingPanelLayout;
    QLabel *missingTitle;
    QTableWidget *missing;
    QWidget *servicesPanel;
    QVBoxLayout *servicesPanelLayout;
    QGroupBox *servicesGroup;
    QVBoxLayout *servicesGroupLayout;
    QWidget *servicesCardsHost;
    QVBoxLayout *servicesCardsLayout;
    QHBoxLayout *servicesButtonsLayout;
    QPushButton *restartServicesButton;
    QPushButton *stopServicesButton;
    QPushButton *tuneAutoButton;
    QSpacerItem *servicesSpacer;
    QWidget *journalArea;
    QHBoxLayout *journalAreaLayout;
    QSplitter *journalSplitter;
    QGroupBox *spotsGroup;
    QVBoxLayout *spotsLayout;
    QHBoxLayout *spotBandFilterLayout;
    QRadioButton *spotBandAll;
    QRadioButton *spotBand160;
    QRadioButton *spotBand80;
    QRadioButton *spotBand40;
    QRadioButton *spotBand30;
    QRadioButton *spotBand20;
    QRadioButton *spotBand17;
    QRadioButton *spotBand15;
    QRadioButton *spotBand12;
    QRadioButton *spotBand10;
    QSpacerItem *spotBandFilterSpacer;
    QTableWidget *spots;
    QGroupBox *logGroup;
    QVBoxLayout *logLayout;
    QTableWidget *log;
    QWidget *filterPanel;
    QVBoxLayout *filterPanelLayout;
    QGroupBox *displayGroup;
    QVBoxLayout *displayLayout;
    QCheckBox *filterHamAlert;
    QCheckBox *filterPSK;
    QCheckBox *filterDXCluster;
    QCheckBox *filterClusterTX;
    QCheckBox *filterClusterConnection;
    QCheckBox *filterAccept;
    QCheckBox *filterReject;
    QCheckBox *filterQSO;
    QCheckBox *filterDatabase;
    QCheckBox *filterSystem;
    QGroupBox *clearGroup;
    QVBoxLayout *clearLayout;
    QPushButton *clearLogButton;
    QGroupBox *saveGroup;
    QVBoxLayout *saveLayout;
    QPushButton *saveLogButton;
    QSpacerItem *filterSpacer;

    void setupUi(QWidget *ChallengeDXCCWidget)
    {
        if (ChallengeDXCCWidget->objectName().isEmpty())
            ChallengeDXCCWidget->setObjectName("ChallengeDXCCWidget");
        ChallengeDXCCWidget->resize(1500, 900);
        rootLayout = new QVBoxLayout(ChallengeDXCCWidget);
        rootLayout->setSpacing(6);
        rootLayout->setObjectName("rootLayout");
        rootLayout->setContentsMargins(8, 8, 8, 8);
        mainSplitter = new QSplitter(ChallengeDXCCWidget);
        mainSplitter->setObjectName("mainSplitter");
        mainSplitter->setOrientation(Qt::Vertical);
        mainSplitter->setChildrenCollapsible(false);
        topSplitter = new QSplitter(mainSplitter);
        topSplitter->setObjectName("topSplitter");
        topSplitter->setOrientation(Qt::Horizontal);
        topSplitter->setChildrenCollapsible(false);
        challengePanel = new QWidget(topSplitter);
        challengePanel->setObjectName("challengePanel");
        challengePanelLayout = new QVBoxLayout(challengePanel);
        challengePanelLayout->setSpacing(5);
        challengePanelLayout->setObjectName("challengePanelLayout");
        challengePanelLayout->setContentsMargins(0, 0, 0, 0);
        challengeGroup = new QGroupBox(challengePanel);
        challengeGroup->setObjectName("challengeGroup");
        challengeFormLayout = new QGridLayout(challengeGroup);
        challengeFormLayout->setObjectName("challengeFormLayout");
        challengeFormLayout->setHorizontalSpacing(6);
        challengeFormLayout->setVerticalSpacing(4);
        challengeFormLayout->setContentsMargins(8, 6, 8, 6);
        nameLabel = new QLabel(challengeGroup);
        nameLabel->setObjectName("nameLabel");

        challengeFormLayout->addWidget(nameLabel, 0, 0, 1, 1);

        nameEdit = new QLineEdit(challengeGroup);
        nameEdit->setObjectName("nameEdit");

        challengeFormLayout->addWidget(nameEdit, 0, 1, 1, 3);

        adifLabel = new QLabel(challengeGroup);
        adifLabel->setObjectName("adifLabel");

        challengeFormLayout->addWidget(adifLabel, 1, 0, 1, 1);

        adifEdit = new QLineEdit(challengeGroup);
        adifEdit->setObjectName("adifEdit");
        adifEdit->setReadOnly(true);

        challengeFormLayout->addWidget(adifEdit, 1, 1, 1, 2);

        chooseAdifButton = new QPushButton(challengeGroup);
        chooseAdifButton->setObjectName("chooseAdifButton");

        challengeFormLayout->addWidget(chooseAdifButton, 1, 3, 1, 1);

        startLabel = new QLabel(challengeGroup);
        startLabel->setObjectName("startLabel");

        challengeFormLayout->addWidget(startLabel, 2, 0, 1, 1);

        startEdit = new QDateEdit(challengeGroup);
        startEdit->setObjectName("startEdit");
        startEdit->setCalendarPopup(true);

        challengeFormLayout->addWidget(startEdit, 2, 1, 1, 1);

        endLabel = new QLabel(challengeGroup);
        endLabel->setObjectName("endLabel");

        challengeFormLayout->addWidget(endLabel, 2, 2, 1, 1);

        endEdit = new QDateEdit(challengeGroup);
        endEdit->setObjectName("endEdit");
        endEdit->setCalendarPopup(true);

        challengeFormLayout->addWidget(endEdit, 2, 3, 1, 1);

        applyProfileButton = new QPushButton(challengeGroup);
        applyProfileButton->setObjectName("applyProfileButton");

        challengeFormLayout->addWidget(applyProfileButton, 3, 2, 1, 2);


        challengePanelLayout->addWidget(challengeGroup);

        band = new QTableWidget(challengePanel);
        if (band->columnCount() < 4)
            band->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        band->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        band->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        band->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        band->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        band->setObjectName("band");
        band->setEditTriggers(QAbstractItemView::NoEditTriggers);
        band->setSelectionBehavior(QAbstractItemView::SelectRows);
        band->setSelectionMode(QAbstractItemView::SingleSelection);
        band->setAlternatingRowColors(true);

        challengePanelLayout->addWidget(band);

        topSplitter->addWidget(challengePanel);
        missingPanel = new QWidget(topSplitter);
        missingPanel->setObjectName("missingPanel");
        missingPanelLayout = new QVBoxLayout(missingPanel);
        missingPanelLayout->setSpacing(4);
        missingPanelLayout->setObjectName("missingPanelLayout");
        missingPanelLayout->setContentsMargins(0, 0, 0, 0);
        missingTitle = new QLabel(missingPanel);
        missingTitle->setObjectName("missingTitle");
        missingTitle->setStyleSheet(QString::fromUtf8("font-size:15px;font-weight:bold;"));

        missingPanelLayout->addWidget(missingTitle);

        missing = new QTableWidget(missingPanel);
        if (missing->columnCount() < 3)
            missing->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        missing->setHorizontalHeaderItem(0, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        missing->setHorizontalHeaderItem(1, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        missing->setHorizontalHeaderItem(2, __qtablewidgetitem6);
        missing->setObjectName("missing");
        missing->setEditTriggers(QAbstractItemView::NoEditTriggers);
        missing->setSelectionBehavior(QAbstractItemView::SelectRows);
        missing->setAlternatingRowColors(true);

        missingPanelLayout->addWidget(missing);

        topSplitter->addWidget(missingPanel);
        servicesPanel = new QWidget(topSplitter);
        servicesPanel->setObjectName("servicesPanel");
        servicesPanelLayout = new QVBoxLayout(servicesPanel);
        servicesPanelLayout->setObjectName("servicesPanelLayout");
        servicesPanelLayout->setContentsMargins(0, 0, 0, 0);
        servicesGroup = new QGroupBox(servicesPanel);
        servicesGroup->setObjectName("servicesGroup");
        servicesGroupLayout = new QVBoxLayout(servicesGroup);
        servicesGroupLayout->setSpacing(6);
        servicesGroupLayout->setObjectName("servicesGroupLayout");
        servicesGroupLayout->setContentsMargins(8, 8, 8, 8);
        servicesCardsHost = new QWidget(servicesGroup);
        servicesCardsHost->setObjectName("servicesCardsHost");
        servicesCardsLayout = new QVBoxLayout(servicesCardsHost);
        servicesCardsLayout->setSpacing(4);
        servicesCardsLayout->setObjectName("servicesCardsLayout");
        servicesCardsLayout->setContentsMargins(0, 0, 0, 0);

        servicesGroupLayout->addWidget(servicesCardsHost);

        servicesButtonsLayout = new QHBoxLayout();
        servicesButtonsLayout->setSpacing(6);
        servicesButtonsLayout->setObjectName("servicesButtonsLayout");
        restartServicesButton = new QPushButton(servicesGroup);
        restartServicesButton->setObjectName("restartServicesButton");
        restartServicesButton->setMinimumHeight(30);

        servicesButtonsLayout->addWidget(restartServicesButton);

        stopServicesButton = new QPushButton(servicesGroup);
        stopServicesButton->setObjectName("stopServicesButton");
        stopServicesButton->setMinimumHeight(30);

        servicesButtonsLayout->addWidget(stopServicesButton);


        servicesGroupLayout->addLayout(servicesButtonsLayout);

        tuneAutoButton = new QPushButton(servicesGroup);
        tuneAutoButton->setObjectName("tuneAutoButton");
        tuneAutoButton->setMinimumHeight(32);

        servicesGroupLayout->addWidget(tuneAutoButton);

        servicesSpacer = new QSpacerItem(20, 10, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        servicesGroupLayout->addItem(servicesSpacer);


        servicesPanelLayout->addWidget(servicesGroup);

        topSplitter->addWidget(servicesPanel);
        mainSplitter->addWidget(topSplitter);
        journalArea = new QWidget(mainSplitter);
        journalArea->setObjectName("journalArea");
        journalAreaLayout = new QHBoxLayout(journalArea);
        journalAreaLayout->setSpacing(6);
        journalAreaLayout->setObjectName("journalAreaLayout");
        journalAreaLayout->setContentsMargins(0, 0, 0, 0);
        journalSplitter = new QSplitter(journalArea);
        journalSplitter->setObjectName("journalSplitter");
        journalSplitter->setOrientation(Qt::Horizontal);
        journalSplitter->setChildrenCollapsible(false);
        spotsGroup = new QGroupBox(journalSplitter);
        spotsGroup->setObjectName("spotsGroup");
        spotsLayout = new QVBoxLayout(spotsGroup);
        spotsLayout->setObjectName("spotsLayout");
        spotsLayout->setContentsMargins(6, 5, 6, 5);
        spotBandFilterLayout = new QHBoxLayout();
        spotBandFilterLayout->setSpacing(3);
        spotBandFilterLayout->setObjectName("spotBandFilterLayout");
        spotBandAll = new QRadioButton(spotsGroup);
        spotBandAll->setObjectName("spotBandAll");
        spotBandAll->setChecked(true);

        spotBandFilterLayout->addWidget(spotBandAll);

        spotBand160 = new QRadioButton(spotsGroup);
        spotBand160->setObjectName("spotBand160");

        spotBandFilterLayout->addWidget(spotBand160);

        spotBand80 = new QRadioButton(spotsGroup);
        spotBand80->setObjectName("spotBand80");

        spotBandFilterLayout->addWidget(spotBand80);

        spotBand40 = new QRadioButton(spotsGroup);
        spotBand40->setObjectName("spotBand40");

        spotBandFilterLayout->addWidget(spotBand40);

        spotBand30 = new QRadioButton(spotsGroup);
        spotBand30->setObjectName("spotBand30");

        spotBandFilterLayout->addWidget(spotBand30);

        spotBand20 = new QRadioButton(spotsGroup);
        spotBand20->setObjectName("spotBand20");

        spotBandFilterLayout->addWidget(spotBand20);

        spotBand17 = new QRadioButton(spotsGroup);
        spotBand17->setObjectName("spotBand17");

        spotBandFilterLayout->addWidget(spotBand17);

        spotBand15 = new QRadioButton(spotsGroup);
        spotBand15->setObjectName("spotBand15");

        spotBandFilterLayout->addWidget(spotBand15);

        spotBand12 = new QRadioButton(spotsGroup);
        spotBand12->setObjectName("spotBand12");

        spotBandFilterLayout->addWidget(spotBand12);

        spotBand10 = new QRadioButton(spotsGroup);
        spotBand10->setObjectName("spotBand10");

        spotBandFilterLayout->addWidget(spotBand10);

        spotBandFilterSpacer = new QSpacerItem(10, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        spotBandFilterLayout->addItem(spotBandFilterSpacer);


        spotsLayout->addLayout(spotBandFilterLayout);

        spots = new QTableWidget(spotsGroup);
        if (spots->columnCount() < 7)
            spots->setColumnCount(7);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(0, __qtablewidgetitem7);
        QTableWidgetItem *__qtablewidgetitem8 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(1, __qtablewidgetitem8);
        QTableWidgetItem *__qtablewidgetitem9 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(2, __qtablewidgetitem9);
        QTableWidgetItem *__qtablewidgetitem10 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(3, __qtablewidgetitem10);
        QTableWidgetItem *__qtablewidgetitem11 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(4, __qtablewidgetitem11);
        QTableWidgetItem *__qtablewidgetitem12 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(5, __qtablewidgetitem12);
        QTableWidgetItem *__qtablewidgetitem13 = new QTableWidgetItem();
        spots->setHorizontalHeaderItem(6, __qtablewidgetitem13);
        spots->setObjectName("spots");
        spots->setEditTriggers(QAbstractItemView::NoEditTriggers);
        spots->setSelectionBehavior(QAbstractItemView::SelectRows);
        spots->setSelectionMode(QAbstractItemView::SingleSelection);
        spots->setAlternatingRowColors(true);

        spotsLayout->addWidget(spots);

        journalSplitter->addWidget(spotsGroup);
        logGroup = new QGroupBox(journalSplitter);
        logGroup->setObjectName("logGroup");
        logLayout = new QVBoxLayout(logGroup);
        logLayout->setObjectName("logLayout");
        logLayout->setContentsMargins(6, 5, 6, 5);
        log = new QTableWidget(logGroup);
        if (log->columnCount() < 4)
            log->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem14 = new QTableWidgetItem();
        log->setHorizontalHeaderItem(0, __qtablewidgetitem14);
        QTableWidgetItem *__qtablewidgetitem15 = new QTableWidgetItem();
        log->setHorizontalHeaderItem(1, __qtablewidgetitem15);
        QTableWidgetItem *__qtablewidgetitem16 = new QTableWidgetItem();
        log->setHorizontalHeaderItem(2, __qtablewidgetitem16);
        QTableWidgetItem *__qtablewidgetitem17 = new QTableWidgetItem();
        log->setHorizontalHeaderItem(3, __qtablewidgetitem17);
        log->setObjectName("log");
        log->setEditTriggers(QAbstractItemView::NoEditTriggers);
        log->setSelectionBehavior(QAbstractItemView::SelectRows);

        logLayout->addWidget(log);

        journalSplitter->addWidget(logGroup);

        journalAreaLayout->addWidget(journalSplitter);

        filterPanel = new QWidget(journalArea);
        filterPanel->setObjectName("filterPanel");
        filterPanel->setMinimumWidth(205);
        filterPanel->setMaximumWidth(205);
        filterPanelLayout = new QVBoxLayout(filterPanel);
        filterPanelLayout->setSpacing(7);
        filterPanelLayout->setObjectName("filterPanelLayout");
        filterPanelLayout->setContentsMargins(0, 0, 0, 0);
        displayGroup = new QGroupBox(filterPanel);
        displayGroup->setObjectName("displayGroup");
        displayLayout = new QVBoxLayout(displayGroup);
        displayLayout->setSpacing(4);
        displayLayout->setObjectName("displayLayout");
        displayLayout->setContentsMargins(9, 7, 9, 9);
        filterHamAlert = new QCheckBox(displayGroup);
        filterHamAlert->setObjectName("filterHamAlert");

        displayLayout->addWidget(filterHamAlert);

        filterPSK = new QCheckBox(displayGroup);
        filterPSK->setObjectName("filterPSK");

        displayLayout->addWidget(filterPSK);

        filterDXCluster = new QCheckBox(displayGroup);
        filterDXCluster->setObjectName("filterDXCluster");

        displayLayout->addWidget(filterDXCluster);

        filterClusterTX = new QCheckBox(displayGroup);
        filterClusterTX->setObjectName("filterClusterTX");

        displayLayout->addWidget(filterClusterTX);

        filterClusterConnection = new QCheckBox(displayGroup);
        filterClusterConnection->setObjectName("filterClusterConnection");

        displayLayout->addWidget(filterClusterConnection);

        filterAccept = new QCheckBox(displayGroup);
        filterAccept->setObjectName("filterAccept");

        displayLayout->addWidget(filterAccept);

        filterReject = new QCheckBox(displayGroup);
        filterReject->setObjectName("filterReject");

        displayLayout->addWidget(filterReject);

        filterQSO = new QCheckBox(displayGroup);
        filterQSO->setObjectName("filterQSO");

        displayLayout->addWidget(filterQSO);

        filterDatabase = new QCheckBox(displayGroup);
        filterDatabase->setObjectName("filterDatabase");

        displayLayout->addWidget(filterDatabase);

        filterSystem = new QCheckBox(displayGroup);
        filterSystem->setObjectName("filterSystem");

        displayLayout->addWidget(filterSystem);


        filterPanelLayout->addWidget(displayGroup);

        clearGroup = new QGroupBox(filterPanel);
        clearGroup->setObjectName("clearGroup");
        clearLayout = new QVBoxLayout(clearGroup);
        clearLayout->setObjectName("clearLayout");
        clearLogButton = new QPushButton(clearGroup);
        clearLogButton->setObjectName("clearLogButton");

        clearLayout->addWidget(clearLogButton);


        filterPanelLayout->addWidget(clearGroup);

        saveGroup = new QGroupBox(filterPanel);
        saveGroup->setObjectName("saveGroup");
        saveLayout = new QVBoxLayout(saveGroup);
        saveLayout->setObjectName("saveLayout");
        saveLogButton = new QPushButton(saveGroup);
        saveLogButton->setObjectName("saveLogButton");

        saveLayout->addWidget(saveLogButton);


        filterPanelLayout->addWidget(saveGroup);

        filterSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        filterPanelLayout->addItem(filterSpacer);


        journalAreaLayout->addWidget(filterPanel);

        mainSplitter->addWidget(journalArea);

        rootLayout->addWidget(mainSplitter);


        retranslateUi(ChallengeDXCCWidget);

        QMetaObject::connectSlotsByName(ChallengeDXCCWidget);
    } // setupUi

    void retranslateUi(QWidget *ChallengeDXCCWidget)
    {
        challengeGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "CHALLENGE ACTIF", nullptr));
        nameLabel->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Nom", nullptr));
        adifLabel->setText(QCoreApplication::translate("ChallengeDXCCWidget", "ADIF", nullptr));
        chooseAdifButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Choisir ADIF\342\200\246", nullptr));
        startLabel->setText(QCoreApplication::translate("ChallengeDXCCWidget", "D\303\251but", nullptr));
        endLabel->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Fin", nullptr));
        applyProfileButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Recalculer / Appliquer", nullptr));
        QTableWidgetItem *___qtablewidgetitem = band->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Bande", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = band->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Travaill\303\251s", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = band->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Manquants", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = band->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("ChallengeDXCCWidget", "% r\303\251alis\303\251s", nullptr));
        missingTitle->setText(QCoreApplication::translate("ChallengeDXCCWidget", "DXCC MANQUANTS", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = missing->horizontalHeaderItem(0);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("ChallengeDXCCWidget", "DXCC", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = missing->horizontalHeaderItem(1);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Pr\303\251fixe", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = missing->horizontalHeaderItem(2);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Pays / Entit\303\251", nullptr));
        servicesGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "CONNEXIONS / SERVICES", nullptr));
        restartServicesButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Red\303\251marrer les services", nullptr));
        stopServicesButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Arr\303\252ter", nullptr));
        tuneAutoButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Tune Auto", nullptr));
        spotsGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "DX SPOTS", nullptr));
        spotBandAll->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Tous", nullptr));
        spotBand160->setText(QCoreApplication::translate("ChallengeDXCCWidget", "160", nullptr));
        spotBand80->setText(QCoreApplication::translate("ChallengeDXCCWidget", "80", nullptr));
        spotBand40->setText(QCoreApplication::translate("ChallengeDXCCWidget", "40", nullptr));
        spotBand30->setText(QCoreApplication::translate("ChallengeDXCCWidget", "30", nullptr));
        spotBand20->setText(QCoreApplication::translate("ChallengeDXCCWidget", "20", nullptr));
        spotBand17->setText(QCoreApplication::translate("ChallengeDXCCWidget", "17", nullptr));
        spotBand15->setText(QCoreApplication::translate("ChallengeDXCCWidget", "15", nullptr));
        spotBand12->setText(QCoreApplication::translate("ChallengeDXCCWidget", "12", nullptr));
        spotBand10->setText(QCoreApplication::translate("ChallengeDXCCWidget", "10", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = spots->horizontalHeaderItem(0);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Vu", nullptr));
        QTableWidgetItem *___qtablewidgetitem8 = spots->horizontalHeaderItem(1);
        ___qtablewidgetitem8->setText(QCoreApplication::translate("ChallengeDXCCWidget", "DX", nullptr));
        QTableWidgetItem *___qtablewidgetitem9 = spots->horizontalHeaderItem(2);
        ___qtablewidgetitem9->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Pays", nullptr));
        QTableWidgetItem *___qtablewidgetitem10 = spots->horizontalHeaderItem(3);
        ___qtablewidgetitem10->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Bande", nullptr));
        QTableWidgetItem *___qtablewidgetitem11 = spots->horizontalHeaderItem(4);
        ___qtablewidgetitem11->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Mode", nullptr));
        QTableWidgetItem *___qtablewidgetitem12 = spots->horizontalHeaderItem(5);
        ___qtablewidgetitem12->setText(QCoreApplication::translate("ChallengeDXCCWidget", "SNR", nullptr));
        QTableWidgetItem *___qtablewidgetitem13 = spots->horizontalHeaderItem(6);
        ___qtablewidgetitem13->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Fr\303\251q.", nullptr));
        logGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "JOURNAL DES COMMUNICATIONS ET \303\211V\303\211NEMENTS", nullptr));
        QTableWidgetItem *___qtablewidgetitem14 = log->horizontalHeaderItem(0);
        ___qtablewidgetitem14->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Heure", nullptr));
        QTableWidgetItem *___qtablewidgetitem15 = log->horizontalHeaderItem(1);
        ___qtablewidgetitem15->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Source", nullptr));
        QTableWidgetItem *___qtablewidgetitem16 = log->horizontalHeaderItem(2);
        ___qtablewidgetitem16->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Type", nullptr));
        QTableWidgetItem *___qtablewidgetitem17 = log->horizontalHeaderItem(3);
        ___qtablewidgetitem17->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Message", nullptr));
        displayGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "AFFICHER", nullptr));
        filterHamAlert->setText(QCoreApplication::translate("ChallengeDXCCWidget", "HamAlert RX", nullptr));
        filterPSK->setText(QCoreApplication::translate("ChallengeDXCCWidget", "PSK Reporter RX", nullptr));
        filterDXCluster->setText(QCoreApplication::translate("ChallengeDXCCWidget", "DX Cluster RX", nullptr));
        filterClusterTX->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Cluster TX", nullptr));
        filterClusterConnection->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Cluster connexion", nullptr));
        filterAccept->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Filtre ACCEPT", nullptr));
        filterReject->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Filtre REJECT", nullptr));
        filterQSO->setText(QCoreApplication::translate("ChallengeDXCCWidget", "QSO Logger32 / JTDX", nullptr));
        filterDatabase->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Base Insert / Delete", nullptr));
        filterSystem->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Info / Erreur", nullptr));
        clearGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "Effacer le journal", nullptr));
        clearLogButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Effacer", nullptr));
        saveGroup->setTitle(QCoreApplication::translate("ChallengeDXCCWidget", "Sauvegarder le journal", nullptr));
        saveLogButton->setText(QCoreApplication::translate("ChallengeDXCCWidget", "Enregistrer sous\342\200\246", nullptr));
        (void)ChallengeDXCCWidget;
    } // retranslateUi

};

namespace Ui {
    class ChallengeDXCCWidget: public Ui_ChallengeDXCCWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_WINDOW_H
