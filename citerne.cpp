#include "citerne.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

Citerne::Citerne()
    : m_id(0),
      m_capaciteL(0.0),
      m_volumeL(0.0),
      m_temperatureC(0.0),
      m_dernierRemplissage(QDate::currentDate())
{
}

Citerne::Citerne(int id,
                 double capaciteL,
                 double volumeL,
                 const QString &qualite,
                 double temperatureC,
                 const QDate &dernierRemplissage)
    : m_id(id),
      m_capaciteL(capaciteL),
      m_volumeL(volumeL),
      m_qualite(qualite),
      m_temperatureC(temperatureC),
      m_dernierRemplissage(dernierRemplissage)
{
}

int Citerne::id() const { return m_id; }
double Citerne::capaciteL() const { return m_capaciteL; }
double Citerne::volumeL() const { return m_volumeL; }
QString Citerne::qualite() const { return m_qualite; }
double Citerne::temperatureC() const { return m_temperatureC; }
QDate Citerne::dernierRemplissage() const { return m_dernierRemplissage; }

void Citerne::setId(int id) { m_id = id; }
void Citerne::setCapaciteL(double value) { m_capaciteL = value; }
void Citerne::setVolumeL(double value) { m_volumeL = value; }
void Citerne::setQualite(const QString &value) { m_qualite = value; }
void Citerne::setTemperatureC(double value) { m_temperatureC = value; }
void Citerne::setDernierRemplissage(const QDate &value) { m_dernierRemplissage = value; }

bool Citerne::ajouter(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery q(db);
    q.prepare("INSERT INTO citernes_app (capacite_l, volume_l, qualite, temperature_c, dernier_remplissage) "
              "VALUES (:cap, :vol, :qual, :temp, TO_DATE(:datev, 'YYYY-MM-DD'))");
    q.bindValue(":cap", m_capaciteL);
    q.bindValue(":vol", m_volumeL);
    q.bindValue(":qual", m_qualite);
    q.bindValue(":temp", m_temperatureC);
    q.bindValue(":datev", m_dernierRemplissage.toString("yyyy-MM-dd"));

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

bool Citerne::modifier(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery q(db);
    q.prepare("UPDATE citernes_app "
              "SET capacite_l=:cap, volume_l=:vol, qualite=:qual, temperature_c=:temp, "
              "dernier_remplissage=TO_DATE(:datev, 'YYYY-MM-DD') "
              "WHERE id=:id");
    q.bindValue(":cap", m_capaciteL);
    q.bindValue(":vol", m_volumeL);
    q.bindValue(":qual", m_qualite);
    q.bindValue(":temp", m_temperatureC);
    q.bindValue(":datev", m_dernierRemplissage.toString("yyyy-MM-dd"));
    q.bindValue(":id", m_id);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

bool Citerne::supprimer(QSqlDatabase &db, int id, QString *errorMessage)
{
    QSqlQuery q(db);
    q.prepare("DELETE FROM citernes_app WHERE id=:id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

bool Citerne::mettreAJourVolume(QSqlDatabase &db, int id, double nouveauVolume, QString *errorMessage)
{
    QSqlQuery q(db);
    q.prepare("UPDATE citernes_app SET volume_l=:vol WHERE id=:id");
    q.bindValue(":vol", nouveauVolume);
    q.bindValue(":id", id);

    if (!q.exec()) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return false;
    }
    return true;
}

QList<Citerne> Citerne::afficher(QSqlDatabase &db, QString *errorMessage)
{
    QList<Citerne> result;
    QSqlQuery q(db);

    if (!q.exec("SELECT id, capacite_l, volume_l, qualite, temperature_c, "
                "TO_CHAR(dernier_remplissage, 'YYYY-MM-DD') "
                "FROM citernes_app ORDER BY id")) {
        if (errorMessage) {
            *errorMessage = q.lastError().text();
        }
        return result;
    }

    while (q.next()) {
        Citerne citerne;
        citerne.setId(q.value(0).toInt());
        citerne.setCapaciteL(q.value(1).toDouble());
        citerne.setVolumeL(q.value(2).toDouble());
        citerne.setQualite(q.value(3).toString());
        citerne.setTemperatureC(q.value(4).toDouble());
        citerne.setDernierRemplissage(QDate::fromString(q.value(5).toString(), "yyyy-MM-dd"));
        result.append(citerne);
    }

    return result;
}
