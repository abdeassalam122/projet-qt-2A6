#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
<<<<<<< HEAD
#include <QStackedWidget>
#include <QTabBar>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QStringList>
#include <QHeaderView>
=======
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
>>>>>>> b329a1b3fa3d42c3171c81d32f3580292407b86c

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
<<<<<<< HEAD
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Réception
    void showAddReceptionDialog();
    void editSelectedReception();
    void deleteSelectedReception();
    void searchReception(const QString &text);
    void refreshReceptionData();

private:
    // UI Setup methods
    void setupUI(); // You need to add this if it exists
    void createReceptionPage(); // ADD THIS DECLARATION
    QWidget* createHeaderWidget(const QString &title); // ADD THIS DECLARATION

    // Réception widgets
    QTableWidget *receptionTable;
    QLineEdit *searchBoxReception;
    QWidget *receptionPage; // ADD THIS - missing member variable
    QStackedWidget *stackedWidget; // ADD THIS - missing member variable
=======
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
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

private:
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
    QStackedWidget *stackedWidget = nullptr;
    QWidget *clientsPage = nullptr;
    QWidget *citernesPage = nullptr;
    QWidget *receptionPage = nullptr;
    QWidget *extractionPage = nullptr;
    QWidget *statisticsPage = nullptr;

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

    // Build UI
    void setupUI();
    QWidget* createHeaderWidget(const QString &title);
    void createClientsPage();
    void createCiternesPage();
    void createReceptionPage();
    void createExtractionPage();
    void createStatisticsPage();

    // Styling & data
    void applyStyles();
    void populateClientsSampleData();
    void populateCiternesSampleData();

    // Helpers
    int findClientRowById(int id);
    int findCiterneRowById(int id);
    void updateClientStatistics();
    QStringList buildCiterneAlerts(double thresholdPercent, int *criticalCount = nullptr, int *warningCount = nullptr, int *normalCount = nullptr) const;

    // Progress widget helpers (DECLARED HERE to avoid "undeclared identifier")
    QWidget* createProgressWidget(int percent);
    QWidget* createProgressBar(int percent); // alias used in code

    bool promptAndTestOracleConnection();
    void updateDatabaseStatusLabel();
    bool setupOracleSchema();
    void loadClientsFromOracle();
    void loadCiternesFromOracle();
    QWidget* createClientActionsWidget(int id);
    QWidget* createCiterneActionsWidget(int id);

    QSqlDatabase db;
    QLabel *dbStatusLabel = nullptr;
    bool oracleActive = false;
>>>>>>> b329a1b3fa3d42c3171c81d32f3580292407b86c
};

#endif // MAINWINDOW_H
