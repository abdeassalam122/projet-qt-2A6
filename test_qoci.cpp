#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "App dir:" << QCoreApplication::applicationDirPath();
    qDebug() << "Library paths:" << QCoreApplication::libraryPaths();
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    if (!QSqlDatabase::isDriverAvailable("QOCI")) {
        qDebug() << "QOCI not available!";
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QOCI");
    db.setHostName("localhost");
    db.setPort(1521);
    db.setDatabaseName("XE");
    db.setUserName("awss");
    db.setPassword("123");

    qDebug() << "Attempting connection...";
    if (!db.open()) {
        qDebug() << "Connection FAILED:" << db.lastError().text();
        qDebug() << "Driver error:" << db.lastError().driverText();
        qDebug() << "Database error:" << db.lastError().databaseText();
        return 1;
    }

    qDebug() << "Connection SUCCESS!";
    db.close();
    return 0;
}
