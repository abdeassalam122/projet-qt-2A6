#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "Starting application...";

    // Load database drivers
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    // Establish connection first
    Connection* c = Connection::instance();
    bool test = c->createConnect();

    if(!test)
    {
        qDebug() << "Database connection failed!";
        QMessageBox::critical(nullptr, QObject::tr("Database Connection Error"),
                              QObject::tr("Failed to connect to database.\n"
                                          "Please check your connection settings.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);
        return -1; // Exit if connection fails
    }

    qDebug() << "Database connection successful, creating main window...";

    // Create and show main window AFTER connection is established
    MainWindow w;
    w.show();

    qDebug() << "Application started successfully";

    w.show();
    return a.exec();
}
