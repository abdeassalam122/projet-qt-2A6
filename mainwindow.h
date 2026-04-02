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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
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
};

#endif // MAINWINDOW_H
