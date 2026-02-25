#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql/QSqlDatabase>
#include <QSqlQuery>
#include <QString>

class Connection
{
public:
    static Connection* instance(); // Accès à l'instance unique
    bool createConnect(); // Méthode pour créer la connexion
    bool openOdbcConnection(const QString &dsn, const QString &user, const QString &password, QString &lastError);
    QSqlDatabase database() const;
    QString currentDsn() const;
    void closeConnection(); // Fermer la connexion

private:
    Connection(); // Constructeur privé
    ~Connection(); // Destructeur privé
    Connection(const Connection&) = delete; // Supprimer le constructeur de copie
    Connection& operator=(const Connection&) = delete; // Supprimer l'opérateur d'affectation

    static Connection* p_instance; // Pointeur vers l'instance unique
    QSqlDatabase db;
    QString m_currentDsn;
};

#endif // CONNECTION_H

