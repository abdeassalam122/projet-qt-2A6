



#include "connexion.h"
#include <QSqlError>
#include <QDebug>

// Initialisation du pointeur d'instance
Connection* Connection::p_instance = nullptr;

// Constructeur privé
Connection::Connection()
{
    // Initialisation de la base de données
    db = QSqlDatabase::addDatabase("QODBC");
}

// Méthode statique pour obtenir l'instance unique
Connection* Connection::instance()
{
    if (p_instance == nullptr) {
        p_instance = new Connection();
    }
    return p_instance;
}

// Méthode pour établir la connexion
bool Connection::createConnect()
{
    QString lastError;
    return openOdbcConnection("projetqt", "awss", "123", lastError);
}

bool Connection::openOdbcConnection(const QString &dsn, const QString &user, const QString &password, QString &lastError)
{
    if (db.isOpen()) {
        db.close();
    }

    db.setDatabaseName(dsn);
    db.setUserName(user);
    db.setPassword(password);

    if (db.open()) {
        m_currentDsn = dsn;
        lastError.clear();
        qDebug() << "Connexion à la base de données réussie";
        return true;
    }

    lastError = db.lastError().text();
    qDebug() << "Erreur de connexion:" << lastError;
    return false;
}

QSqlDatabase Connection::database() const
{
    return db;
}

QString Connection::currentDsn() const
{
    return m_currentDsn;
}

// Fermer la connexion
void Connection::closeConnection()
{
    if (db.isOpen()) {
        db.close();
    }
}

// Destructeur privé
Connection::~Connection()
{
    closeConnection();
}





