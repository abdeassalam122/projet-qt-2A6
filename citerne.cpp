#include "citerne.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QDateTime>

namespace {
double qualityTextToIndex(const QString &value)
{
    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (ok) {
        return numeric;
    }

    const QString v = value.trimmed().toUpper();
    if (v == "EXTRA") return 95.0;
    if (v == "PREMIUM") return 90.0;
    if (v == "A") return 85.0;
    if (v == "B") return 75.0;
    if (v == "C") return 65.0;
    if (v == "FAIBLE") return 50.0;
    return 70.0;
}
}

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
    q.prepare("INSERT INTO CITERNE (CODE, CAPACITY_L, CURRENT_VOLUME_L, QUALITY_INDEX, TEMPERATURE_C, LAST_FILLING_AT, STATUS) "
              "VALUES (:code, :cap, :vol, :qidx, :temp, TO_DATE(:datev, 'YYYY-MM-DD'), :status)");
    q.bindValue(":code", QString("CT-%1").arg(QDateTime::currentMSecsSinceEpoch()));
    q.bindValue(":cap", m_capaciteL);
    q.bindValue(":vol", m_volumeL);
    q.bindValue(":qidx", qualityTextToIndex(m_qualite));
    q.bindValue(":temp", m_temperatureC);
    q.bindValue(":datev", m_dernierRemplissage.toString("yyyy-MM-dd"));
    q.bindValue(":status", "ACTIF");

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
    q.prepare("UPDATE CITERNE "
              "SET CAPACITY_L=:cap, CURRENT_VOLUME_L=:vol, QUALITY_INDEX=:qidx, TEMPERATURE_C=:temp, "
              "LAST_FILLING_AT=TO_DATE(:datev, 'YYYY-MM-DD') "
              "WHERE id=:id");
    q.bindValue(":cap", m_capaciteL);
    q.bindValue(":vol", m_volumeL);
    q.bindValue(":qidx", qualityTextToIndex(m_qualite));
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
    q.prepare("DELETE FROM CITERNE WHERE id=:id");
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
    q.prepare("UPDATE CITERNE SET CURRENT_VOLUME_L=:vol WHERE id=:id");
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

    if (!q.exec("SELECT ID, CAPACITY_L, CURRENT_VOLUME_L, QUALITY_INDEX, TEMPERATURE_C, "
                "TO_CHAR(LAST_FILLING_AT, 'YYYY-MM-DD') "
                "FROM CITERNE ORDER BY ID")) {
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
        citerne.setQualite(QString::number(q.value(3).toDouble(),'f',2));
        citerne.setTemperatureC(q.value(4).toDouble());
        citerne.setDernierRemplissage(QDate::fromString(q.value(5).toString(), "yyyy-MM-dd"));
        result.append(citerne);
    }

    return result;
}
bool Citerne::updateTemperature(QSqlDatabase &db, int id, double delta)
{
    QSqlQuery q(db);

    q.prepare("UPDATE CITERNE SET TEMPERATURE_C = TEMPERATURE_C + :d WHERE ID=:id");
    q.bindValue(":d", delta);
    q.bindValue(":id", id);

    if(!q.exec()) {
        qDebug() << "Erreur updateTemperature:" << q.lastError().text();
        return false;
    }
    return true;
}
