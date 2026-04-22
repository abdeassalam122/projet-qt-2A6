#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "connection.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto appendStartupLog = [](const QString &message) {
        QFile logFile(QDir::temp().filePath("awss-db-startup.log"));
        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&logFile);
            stream << message << '\n';
        }
    };

    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList libraryPaths = QCoreApplication::libraryPaths();
    libraryPaths.prepend(appDir);
    libraryPaths.prepend(QDir::cleanPath(appDir + "/.."));
    libraryPaths.prepend(QStringLiteral("C:/Qt/6.11.0/mingw_64/plugins"));
    libraryPaths.removeDuplicates();
    QCoreApplication::setLibraryPaths(libraryPaths);

    qDebug() << "Starting application...";
    qDebug() << "Qt library paths:" << QCoreApplication::libraryPaths();
    appendStartupLog("Starting application...");
    appendStartupLog("Qt library paths: " + QCoreApplication::libraryPaths().join(" | "));

    // Load database drivers
    qDebug() << "Available drivers:" << QSqlDatabase::drivers();
    appendStartupLog("Available drivers: " + QSqlDatabase::drivers().join(", "));

    // Establish connection first
    Connection* c = Connection::instance();
    bool test = c->createConnect();
    appendStartupLog(QStringLiteral("Database open result: %1").arg(test ? QStringLiteral("success") : QStringLiteral("failure")));
    appendStartupLog("Last error: " + c->database().lastError().text());

    if(!test)
    {
        qDebug() << "Database connection failed!";
        const QString errorText = c->database().lastError().text();
        QMessageBox::warning(nullptr, QObject::tr("Database Connection Warning"),
                             QObject::tr("Failed to connect to database.\n"
                                         "Please check your connection settings.\n\n"
                                         "Driver error: %1\n\n"
                                         "The application will continue with limited functionality.")
                                     .arg(errorText.isEmpty() ? QObject::tr("Unknown error") : errorText),
                             QMessageBox::Ok);
        // Continue anyway - don't exit
    }
    else
    {
        qDebug() << "Database connection successful, creating main window...";
    }

    // Create and show main window AFTER connection is established
    MainWindow w;
    w.show();

    qDebug() << "Application started successfully";

    w.show();
    return a.exec();
}
