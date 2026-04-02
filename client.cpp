#include "client.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

Client::Client()
    : m_id(0),
      m_createdAt(QDate::currentDate())
{
}

Client::Client(int id,
               const QString &name,
               const QString &email,
               const QString &phone,
               const QString &address,
               const QDate &createdAt,
               const QString &status)
    : m_id(id),
      m_name(name),
      m_email(email),
      m_phone(phone),
      m_address(address),
      m_createdAt(createdAt),
      m_status(status)
{
}

int Client::id() const { return m_id; }
QString Client::name() const { return m_name; }
QString Client::email() const { return m_email; }
QString Client::phone() const { return m_phone; }
QString Client::address() const { return m_address; }
QDate Client::createdAt() const { return m_createdAt; }
QString Client::status() const { return m_status; }

void Client::setId(int id) { m_id = id; }
void Client::setName(const QString &value) { m_name = value; }
void Client::setEmail(const QString &value) { m_email = value; }
void Client::setPhone(const QString &value) { m_phone = value; }
void Client::setAddress(const QString &value) { m_address = value; }
void Client::setCreatedAt(const QDate &value) { m_createdAt = value; }
void Client::setStatus(const QString &value) { m_status = value; }

bool Client::ajouter(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery q(db);
    q.prepare("INSERT INTO CLIENT (NAME, EMAIL, PHONE, ADDRESS, CREATED_AT, STATUS) "
              "VALUES (:name, :email, :phone, :address, TO_DATE(:createdAt, 'YYYY-MM-DD'), :status)");
    q.bindValue(":name", m_name);
    q.bindValue(":email", m_email);
    q.bindValue(":phone", m_phone);
    q.bindValue(":address", m_address);
    q.bindValue(":createdAt", m_createdAt.toString("yyyy-MM-dd"));
    q.bindValue(":status", m_status);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

bool Client::modifier(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery q(db);
    q.prepare("UPDATE CLIENT "
              "SET NAME=:name, EMAIL=:email, PHONE=:phone, ADDRESS=:address, "
              "CREATED_AT=TO_DATE(:createdAt, 'YYYY-MM-DD'), STATUS=:status "
              "WHERE id=:id");
    q.bindValue(":name", m_name);
    q.bindValue(":email", m_email);
    q.bindValue(":phone", m_phone);
    q.bindValue(":address", m_address);
    q.bindValue(":createdAt", m_createdAt.toString("yyyy-MM-dd"));
    q.bindValue(":status", m_status);
    q.bindValue(":id", m_id);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

bool Client::supprimer(QSqlDatabase &db, int id, QString *errorMessage)
{
    QSqlQuery q(db);
    q.prepare("DELETE FROM CLIENT WHERE id=:id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

QList<Client> Client::afficher(QSqlDatabase &db, QString *errorMessage)
{
    QList<Client> result;
    QSqlQuery q(db);

    if (!q.exec("SELECT ID, NAME, EMAIL, PHONE, ADDRESS, TO_CHAR(CREATED_AT, 'YYYY-MM-DD'), STATUS "
                "FROM CLIENT ORDER BY ID")) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return result;
    }

    while (q.next()) {
        Client client;
        client.setId(q.value(0).toInt());
        client.setName(q.value(1).toString());
        client.setEmail(q.value(2).toString());
        client.setPhone(q.value(3).toString());
        client.setAddress(q.value(4).toString());
        client.setCreatedAt(QDate::fromString(q.value(5).toString(), "yyyy-MM-dd"));
        client.setStatus(q.value(6).toString());
        result.append(client);
    }

    return result;
}
