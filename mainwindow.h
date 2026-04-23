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
#include <QDialog>
#include <QFrame>
#include <QDateEdit>
#include <QString>
#include <QList>
#include <QSqlDatabase>
#include <QTextBrowser>
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ==================== RÉCEPTION - CRUD ====================
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

    // Navigation slots

    // ==================== RÉCEPTION - STATISTIQUES ====================
    void showReceptionStatistics();
    void showPerformanceChart();

    // ==================== RÉCEPTION - TRI ====================
    void sortReceptionBy(const QString& colonne, Qt::SortOrder order);
    void onSortById();
    void onSortByLot();
    void onSortByDate();
    void onSortByQuantity();

    // ==================== RÉCEPTION - FILTRAGE ====================
    void filterReceptionByStatus();
    void filterReceptionByDate();
    void filterReceptionByStatusAndDate();

    // ==================== RÉCEPTION - PDF ====================
    void exportReceptionsToPDF();
    void exportStatisticsToPDF();
    void exportFilteredReceptionsToPDF();

    // ==================== NAVIGATION ====================

    void switchToClients();
    void switchToCiternes();
    void switchToReception();
    void switchToExtraction();

    // ==================== CLIENTS ====================
    void showAddClientDialog();
    void editSelectedClient();
    void deleteSelectedClient();
    void viewSelectedClient();
    void sortClients();
    void exportClients();
    void searchClients(const QString &text);
    void showClientComparisonDashboard();
    void showClientRfmMatrix();

    // ==================== CITERNES ====================
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

    // ==================== QUALITY SIMULATOR ====================
    void openBlendingSimulator();
    void performBlending();
    void calculateBlendingResult();

    // ==================== NOTIFICATIONS & ALERTS ====================
    void checkLowLevelAlerts();
    void showNotifications();
    void configureThresholds();
    void viewFillingHistory();

    // ==================== MAINTENANCE PRÉDICTIVE ====================
    void checkPredictiveAlerts();
    void showEquipmentStatus();
    void detectAnomalies();

    // ==================== NAVIGATION PAGES ====================
    void showStatisticsView();
    void showMainListView();
    void showMainCiterneListView();

private:
    // ==================== UI SETUP ====================
    void setupUI();
    void createReceptionPage();
    void createClientsPage();
    void createCiternesPage();
    void createExtractionPage();
    void createStatisticsPage();

    void createCiterneStatisticsPage();
    // Main layout and sidebar
    QWidget *central = nullptr;

    QWidget* createHeaderWidget(const QString &title);

    QWidget* createSideBar();
    void applyStyles();

    // ==================== DONNÉES EXEMPLES ====================
    void populateClientsSampleData();
    void populateCiternesSampleData();

    // ==================== ORACLE ====================
    bool promptAndTestOracleConnection();
    void updateDatabaseStatusLabel();
    bool setupOracleSchema();
    void loadClientsFromOracle();
    void loadCiternesFromOracle();
    QWidget* createClientActionsWidget(int id);
    QWidget* createCiterneActionsWidget(int id);

    // ==================== HELPERS ====================
    int findClientRowById(int id);
    int findCiterneRowById(int id);
    void updateClientStatistics();
    QStringList buildCiterneAlerts(double thresholdPercent, int *criticalCount = nullptr, int *warningCount = nullptr, int *normalCount = nullptr) const;
    QWidget* createProgressWidget(int percent);
    QWidget* createProgressBar(int percent);

    // ==================== POINTEURS UI ====================
    QWidget *central = nullptr;
    QStackedWidget *stackedWidget = nullptr;
    QTabBar *moduleTabs = nullptr;
    QButtonGroup *navGroup = nullptr;

    // Boutons navigation
    QPushButton *btnClients = nullptr;
    QPushButton *btnCiternes = nullptr;
    QPushButton *btnReception = nullptr;
    QPushButton *btnExtraction = nullptr;

    // Pages
    QWidget *clientsPage = nullptr;
    QWidget *citernesPage = nullptr;
    QWidget *receptionPage = nullptr;
    QWidget *extractionPage = nullptr;
    QWidget *statisticsPage = nullptr;
    QWidget *citerneStatisticsPage = nullptr;

    // ==================== RÉCEPTION WIDGETS ====================
    QTableWidget *receptionTable = nullptr;
    QLineEdit *searchBoxReception = nullptr;
    void showPredictionDialog();
    void showProductionPriority();


    // Extraction widgets
    QTableWidget *extractionTable = nullptr;
    QLineEdit *searchBoxExtraction = nullptr;

    // Clients widgets

    // ==================== CLIENTS WIDGETS ====================

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

    // Statistiques labels
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

    // ==================== CITERNES WIDGETS ====================

    QTableWidget *citernesTable = nullptr;
    QLineEdit *searchBoxCiternes = nullptr;
    QPushButton *addCiterneBtn = nullptr;
    QPushButton *editCiterneBtn = nullptr;
    QPushButton *deleteCiterneBtn = nullptr;
    QPushButton *detailsCiterneBtn = nullptr;
    QPushButton *blendingBtn = nullptr;
    QPushButton *alertsBtn = nullptr;

    // ==================== DONNÉES ====================
    double lowLevelThreshold = 30.0;
    QStringList notificationHistory;
    QList<int> qualitySelectedRows;


    // Styling & data
    void applyStyles();
    void populateClientsSampleData();
    void populateCiternesSampleData();

    // Helpers
    int findClientRowById(int id);
    int findCiterneRowById(int id);
    void updateClientStatistics();
    void updateCiterneStatistics();
    QStringList buildCiterneAlerts(double thresholdPercent, int *criticalCount = nullptr, int *warningCount = nullptr, int *normalCount = nullptr) const;

    // Progress widget helpers
    QWidget* createProgressWidget(int percent);
    QWidget* createProgressBar(int percent);

    bool promptAndTestOracleConnection();
    void updateDatabaseStatusLabel();
    bool setupOracleSchema();
    void loadClientsFromOracle();
    void loadCiternesFromOracle();
    QWidget* createClientActionsWidget(int id);
    QWidget* createCiterneActionsWidget(int id);


    // ==================== ORACLE ====================
    QSqlDatabase db;
    QLabel *dbStatusLabel = nullptr;
    bool oracleActive = false;
    QWidget* createStatsCard(const QString& title, const QString& value, const QString& color);

};

#endif // MAINWINDOW_H
