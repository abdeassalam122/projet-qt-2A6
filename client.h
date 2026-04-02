#ifndef CLIENT_H
#define CLIENT_H

#include <QDate>
#include <QList>
#include <QString>
#include <QtSql/QSqlDatabase>

class Client
{
public:
    Client();
    Client(int id,
           const QString &name,
           const QString &email,
           const QString &phone,
           const QString &address,
           const QDate &createdAt,
           const QString &status);

    int id() const;
    QString name() const;
    QString email() const;
    QString phone() const;
    QString address() const;
    QDate createdAt() const;
    QString status() const;

    void setId(int id);
    void setName(const QString &value);
    void setEmail(const QString &value);
    void setPhone(const QString &value);
    void setAddress(const QString &value);
    void setCreatedAt(const QDate &value);
    void setStatus(const QString &value);

    bool ajouter(QSqlDatabase &db, QString *errorMessage = nullptr) const;
    bool modifier(QSqlDatabase &db, QString *errorMessage = nullptr) const;

    static bool supprimer(QSqlDatabase &db, int id, QString *errorMessage = nullptr);
    static QList<Client> afficher(QSqlDatabase &db, QString *errorMessage = nullptr);

private:
    int m_id;
    QString m_name;
    QString m_email;
    QString m_phone;
    QString m_address;
    QDate m_createdAt;
    QString m_status;
};

#endif // CLIENT_H
