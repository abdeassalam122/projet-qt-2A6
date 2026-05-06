#include "connection.h"
#include <QSqlError>
#include <QDebug>

Connection* Connection::p_instance = nullptr;

Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection* Connection::instance()
{
    if (p_instance == nullptr) {
        p_instance = new Connection();
    }
    return p_instance;
}

bool Connection::createConnect()
{
    if (db.isOpen()) db.close();

    db.setDatabaseName(QStringLiteral("AWSS_XE"));
    db.setUserName(QStringLiteral("system"));
    db.setPassword(QStringLiteral("123456"));

    if (!db.open()) {
        qDebug() << "Erreur de connexion:" << db.lastError().text();
        return false;
    }

    qDebug() << "Connexion à la base de données réussie";
    return true;
}

void Connection::closeConnection()
{
    if (db.isOpen()) db.close();
}

QSqlDatabase Connection::database() const
{
    return db;
}

bool Connection::isOpen() const
{
    return db.isOpen();
}

Connection::~Connection()
{
    closeConnection();
}
