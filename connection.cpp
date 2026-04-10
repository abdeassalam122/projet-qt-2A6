#include "connection.h"
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

// Méthode pour établir la connexion (version originale)
bool Connection::createConnect()
{
    bool test = false;

    db.setDatabaseName("Source_Projet2A");//inserer le nom de la source de données
    db.setUserName("mohamed");//inserer nom de l'utilisateur
    db.setPassword("1234");//inserer mot de passe de cet utilisateur

    if (db.open()) {
        test = true;
        qDebug() << "Connexion à la base de données réussie";
    } else {
        qDebug() << "Erreur de connexion:" << db.lastError().text();
    }

    return test;
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
