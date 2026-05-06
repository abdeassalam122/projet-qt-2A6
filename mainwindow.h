#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>

#include <QStackedWidget>
#include <QTabBar>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QStringList>
#include <QHeaderView>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QStackedWidget>
#include <QDialog>
#include <QHeaderView>
#include <QFrame>
#include <QDateEdit>
#include <QString>
#include <QList>
#include <QButtonGroup>
#include <QTabBar>
#include <QSqlDatabase>
#include <QStringListModel>
#include <QCompleter>
#include "Arduino.h"
#include "extraction.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);  // Fixed: added 'explicit' and removed duplicate
    ~MainWindow();

private slots:
    // Réception slots
    void showAddReceptionDialog();
    void editSelectedReception();
    void deleteSelectedReception();
    void searchReception(const QString &text);
    void refreshReceptionData();

    // Extraction slots
    void showAddExtractionDialog();
    void editSelectedExtraction();
    void deleteSelectedExtraction();
    void searchExtraction(const QString &text);
    void refreshExtractionData();
    void showPlanificationExtraction();
    void showTauxExtraction();
    void sortExtractionById();
    void exportExtractions();
    void showExtractionStatistics();

    // Navigation slots
    void switchToClients();
    void switchToCiternes();
    void switchToReception();
    void switchToExtraction();

    // Clients module slots
    void showAddClientDialog();
    void editSelectedClient();
    void deleteSelectedClient();
    void viewSelectedClient();
    void sortClients();
    void exportClients();
    void searchClients(const QString &text);
    void showClientComparisonDashboard();
    void showClientRfmMatrix();

    // Citernes module slots (advanced)
    void showAddCiterneDialog();
    void editSelectedCiterne();
    void deleteSelectedCiterne();
    void viewCiterneDetails();
    void onFillButtonClicked();
    void onDrainButtonClicked();
    void searchCiternes(const QString &text);
    void sortCiternes();
    void exportCiternes();
    void showCiterneStatistics();

    // Quality simulator slots
    void openBlendingSimulator();
    void performBlending();
    void calculateBlendingResult();

    // Notification & Alert slots
    void checkLowLevelAlerts();
    void showNotifications();
    void configureThresholds();
    void viewFillingHistory();

    // Predictive maintenance slots
    void checkPredictiveAlerts();
    void showEquipmentStatus();
    void detectAnomalies();

    // Navigation between pages
    void showStatisticsView();
    void showMainListView();
    void showMainCiterneListView();

private:
    // UI Setup methods
    void setupUI();
    void createReceptionPage();
    QWidget* createHeaderWidget(const QString &title);
    void createClientsPage();
    void createCiternesPage();
    void createExtractionPage();
    void createStatisticsPage();
    void createCiterneStatisticsPage();
    // Main layout and sidebar
    QWidget *central = nullptr;
    QWidget* createSideBar();
    QButtonGroup *navGroup = nullptr;
    QPushButton *btnClients = nullptr;
    QPushButton *btnCiternes = nullptr;
    QPushButton *btnReception = nullptr;
    QPushButton *btnExtraction = nullptr;
    QTabBar *moduleTabs = nullptr;

    // Stack and pages
    QStackedWidget *stackedWidget = nullptr;  // Moved here and initialized
    QWidget *clientsPage = nullptr;
    QWidget *citernesPage = nullptr;
    QWidget *receptionPage = nullptr;  // Only declared once now
    QWidget *extractionPage = nullptr;
    QWidget *statisticsPage = nullptr;
    QWidget *citerneStatisticsPage = nullptr;

    // Réception widgets
    QTableWidget *receptionTable = nullptr;
    QLineEdit *searchBoxReception = nullptr;

    // Extraction widgets
    QTableWidget *extractionTable = nullptr;
    QLineEdit *searchBoxExtraction = nullptr;
    QStringListModel *extractionCompleterModel = nullptr;

    // Clients widgets
    QTableWidget *clientsTable = nullptr;
    QLineEdit *searchBox = nullptr;
    QPushButton *addButton = nullptr;
    QPushButton *editButton = nullptr;
    QPushButton *deleteButton = nullptr;
    QPushButton *viewButton = nullptr;
    QPushButton *sortButton = nullptr;
    QPushButton *exportButton = nullptr;
    QPushButton *statsButton = nullptr;
    QPushButton *backFromStatsButton = nullptr;
    QLabel *statsTotalClientsValue = nullptr;
    QLabel *statsActiveClientsValue = nullptr;
    QLabel *statsInactiveClientsValue = nullptr;
    QLabel *statsNewThisMonthValue = nullptr;
    QLabel *statsCompleteProfilesValue = nullptr;
    QLabel *statsStatusBreakdownValue = nullptr;
    QLabel *statsTopCityValue = nullptr;
    QLabel *statsDonutChartLabel = nullptr;
    QLabel *statsLegendActiveValue = nullptr;
    QLabel *statsLegendInactiveValue = nullptr;
    QLabel *statsLegendUndefinedValue = nullptr;
    QLabel *statsLegendOtherValue = nullptr;

    // Citernes statistics widgets
    QPushButton *backFromCiterneStatsButton = nullptr;
    QLabel *citerneStatsTotalValue = nullptr;
    QLabel *citerneStatsCapacityValue = nullptr;
    QLabel *citerneStatsVolumeValue = nullptr;
    QLabel *citerneStatsFillRateValue = nullptr;
    QLabel *citerneStatsTempValue = nullptr;
    QLabel *citerneStatsCriticalValue = nullptr;
    QLabel *citerneStatsBreakdownValue = nullptr;
    QLabel *citerneStatsTopQualityValue = nullptr;
    QLabel *citerneStatsDonutChartLabel = nullptr;
    QLabel *citerneStatsLegendCriticalValue = nullptr;
    QLabel *citerneStatsLegendWarningValue = nullptr;
    QLabel *citerneStatsLegendNormalValue = nullptr;

    // Citernes widgets
    QTableWidget *citernesTable = nullptr;
    QLineEdit *searchBoxCiternes = nullptr;
    QPushButton *addCiterneBtn = nullptr;
    QPushButton *editCiterneBtn = nullptr;
    QPushButton *deleteCiterneBtn = nullptr;
    QPushButton *detailsCiterneBtn = nullptr;
    QPushButton *blendingBtn = nullptr;
    QPushButton *alertsBtn = nullptr;

    // Quality simulation & Notification data
    double lowLevelThreshold = 30.0; // Configurable threshold
    QStringList notificationHistory;
    QList<int> qualitySelectedRows;

    // Styling & data
    void applyStyles();
    void populateClientsSampleData();
    void populateCiternesSampleData();
    void populateExtractionSampleData();

    // Helpers
    int findClientRowById(int id);
    int findCiterneRowById(int id);
    void updateClientStatistics();
    void updateCiterneStatistics();
    QStringList buildCiterneAlerts(double thresholdPercent, int *criticalCount = nullptr, int *warningCount = nullptr, int *normalCount = nullptr) const;

    // Progress widget helpers
    QWidget* createProgressWidget(int percent);
    QWidget* createProgressBar(int percent);

    void updateDatabaseStatusLabel();
    void setupArduinoMonitoring();
    void processArduinoTemperatureData();
    void handleArduinoTemperature(double temperatureC);
    bool setupOracleSchema();
    void loadClientsFromOracle();
    void loadCiternesFromOracle();
    QWidget* createClientActionsWidget(int id);
    QWidget* createCiterneActionsWidget(int id);

    QSqlDatabase db;
    QLabel *dbStatusLabel = nullptr;
    bool oracleActive = false;
    Arduino m_arduino;
    QString m_arduinoBuffer;
    int m_temperatureAlertState = 0;

    // In-memory extraction list for demo mode
    QList<Extraction> m_extractions;
    int m_nextExtractionId = 1;
    bool m_extractionSortAsc = false; // toggle: false = DESC first click
};

#endif // MAINWINDOW_H
