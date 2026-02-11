#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
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
#include <QButtonGroup>
#include <QTabBar>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Navigation slots
    void switchToClients();
    void switchToCiternes();
    void switchToReception();
    void switchToFacturation();

    // Clients module slots
    void showAddClientDialog();
    void editSelectedClient();
    void deleteSelectedClient();
    void viewSelectedClient();
    void sortClients();
    void exportClients();
    void searchClients(const QString &text);

    // Citernes module slots (basic)
    void showAddCiterneDialog();
    void editSelectedCiterne();
    void deleteSelectedCiterne();
    void onFillButtonClicked();
    void onDrainButtonClicked();
    void searchCiternes(const QString &text);
    void sortCiternes();
    void exportCiternes();

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
    QPushButton *btnFacturation = nullptr;
    QTabBar *moduleTabs = nullptr;

    // Stack and pages
    QStackedWidget *stackedWidget = nullptr;
    QWidget *clientsPage = nullptr;
    QWidget *citernesPage = nullptr;
    QWidget *receptionPage = nullptr;
    QWidget *facturationPage = nullptr;

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

    // Citernes widgets
    QTableWidget *citernesTable = nullptr;
    QLineEdit *searchBoxCiternes = nullptr;
    QPushButton *addCiterneBtn = nullptr;
    QPushButton *editCiterneBtn = nullptr;
    QPushButton *deleteCiterneBtn = nullptr;

    // Build UI
    void setupUI();
    QWidget* createHeaderWidget(const QString &title);
    void createClientsPage();
    void createCiternesPage();
    void createReceptionPage();
    void createFacturationPage();
    void createStatisticsPage();

    // Styling & data
    void applyStyles();
    void populateClientsSampleData();
    void populateCiternesSampleData();

    // Helpers
    int findClientRowById(int id);
    int findCiterneRowById(int id);

    // Progress widget helpers (DECLARED HERE to avoid "undeclared identifier")
    QWidget* createProgressWidget(int percent);
    QWidget* createProgressBar(int percent); // alias used in code
};

#endif // MAINWINDOW_H
