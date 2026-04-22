#include "connection.h"
#include <QSqlError>
#include <QDebug>
#include <QList>

// Initialisation du pointeur d'instance
Connection* Connection::p_instance = nullptr;

// Constructeur privé
Connection::Connection()
{
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

// Méthode pour établir la connexion (version originale)
bool Connection::createConnect()
{
    struct OdbcAttempt {
        QString databaseName;
        QString description;
    };

    const QList<OdbcAttempt> attempts = {
        {QStringLiteral("Driver={Oracle in XE};Dbq=//localhost:1521/XE;"), QStringLiteral("ODBC Oracle in XE via host/service")},
        {QStringLiteral("Driver={Oracle in XE};Dbq=XE;"), QStringLiteral("ODBC Oracle in XE via local service alias")}
    };

    db.setUserName("awss");
    db.setPassword("123");

    for (const OdbcAttempt &attempt : attempts) {
        if (db.isOpen()) {
            db.close();
        }

        db.setDatabaseName(attempt.databaseName);

        qDebug() << "Tentative de connexion Oracle via ODBC:" << attempt.description;
        if (db.open()) {
            qDebug() << "Connexion à la base de données réussie avec" << attempt.description;
            return true;
        }

        qDebug() << "Echec de connexion Oracle via ODBC avec" << attempt.description << ":" << db.lastError().text();
    }

    qDebug() << "Drivers disponibles:" << QSqlDatabase::drivers();
    return false;
}

// Nouvelle méthode pour la connexion ODBC avec paramètres
bool Connection::openOdbcConnection(const QString &dsn, const QString &user,
                                    const QString &password, QString &lastError)
{
    db.setDatabaseName(dsn);
    db.setUserName(user);
    db.setPassword(password);

    if (!db.open()) {
        lastError = db.lastError().text();
        qDebug() << "Erreur de connexion:" << lastError;
        return false;
    }

    qDebug() << "Connexion ODBC réussie à la base Oracle";
    return true;
}

// Fermer la connexion
void Connection::closeConnection()
{
    if (db.isOpen()) {
        db.close();
    }
}

// Retourne la base de données
QSqlDatabase Connection::database() const
{
    return db;
}

// Vérifie si la connexion est ouverte
bool Connection::isOpen() const
{
    return db.isOpen();
}

// Destructeur privé
Connection::~Connection()
{
    closeConnection();
}
