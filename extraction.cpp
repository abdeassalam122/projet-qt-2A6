#include "extraction.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

Extraction::Extraction()
    : m_id(0),
      m_lotId(0),
      m_machineId(0),
      m_targetCiterneId(0),
      m_extractedAt(QDate::currentDate()),
      m_inputQuantityKg(0.0),
      m_outputOilL(0.0),
      m_status("PLANIFIE")
{
}

Extraction::Extraction(int id,
                       int lotId,
                       int machineId,
                       int targetCiterneId,
                       const QDate &extractedAt,
                       double inputQuantityKg,
                       double outputOilL,
                       const QString &status)
    : m_id(id),
      m_lotId(lotId),
      m_machineId(machineId),
      m_targetCiterneId(targetCiterneId),
      m_extractedAt(extractedAt),
      m_inputQuantityKg(inputQuantityKg),
      m_outputOilL(outputOilL),
      m_status(status)
{
}

int Extraction::id() const { return m_id; }
int Extraction::lotId() const { return m_lotId; }
int Extraction::machineId() const { return m_machineId; }
int Extraction::targetCiterneId() const { return m_targetCiterneId; }
QDate Extraction::extractedAt() const { return m_extractedAt; }
double Extraction::inputQuantityKg() const { return m_inputQuantityKg; }
double Extraction::outputOilL() const { return m_outputOilL; }
QString Extraction::status() const { return m_status; }

void Extraction::setId(int value) { m_id = value; }
void Extraction::setLotId(int value) { m_lotId = value; }
void Extraction::setMachineId(int value) { m_machineId = value; }
void Extraction::setTargetCiterneId(int value) { m_targetCiterneId = value; }
void Extraction::setExtractedAt(const QDate &value) { m_extractedAt = value; }
void Extraction::setInputQuantityKg(double value) { m_inputQuantityKg = value; }
void Extraction::setOutputOilL(double value) { m_outputOilL = value; }
void Extraction::setStatus(const QString &value) { m_status = value; }

int Extraction::getNextId(QSqlDatabase &db, QString *errorMessage)
{
    QSqlQuery query(db);
    if (!query.exec("SELECT NVL(MAX(id), 0) + 1 FROM extraction")) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return -1;
    }

    if (!query.next()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Impossible de calculer le prochain ID extraction.");
        }
        return -1;
    }

    return query.value(0).toInt();
}

bool Extraction::ajouter(QSqlDatabase &db, QString *errorMessage) const
{
    const int nextId = (m_id > 0) ? m_id : getNextId(db, errorMessage);
    if (nextId <= 0) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO extraction "
                  "(id, lot_id, machine_id, target_citerne_id, extracted_at, input_quantity_kg, output_oil_l, status) "
                  "VALUES (:id, :lot_id, :machine_id, :target_citerne_id, :extracted_at, :input_quantity_kg, :output_oil_l, :status)");

    query.bindValue(":id", nextId);
    query.bindValue(":lot_id", m_lotId);
    query.bindValue(":machine_id", m_machineId);
    query.bindValue(":target_citerne_id", m_targetCiterneId);
    query.bindValue(":extracted_at", m_extractedAt);
    query.bindValue(":input_quantity_kg", m_inputQuantityKg);
    query.bindValue(":output_oil_l", m_outputOilL);
    query.bindValue(":status", m_status.isEmpty() ? QStringLiteral("PLANIFIE") : m_status);

    if (!query.exec()) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool Extraction::modifier(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery query(db);
    query.prepare("UPDATE extraction SET "
                  "lot_id = :lot_id, "
                  "machine_id = :machine_id, "
                  "target_citerne_id = :target_citerne_id, "
                  "extracted_at = :extracted_at, "
                  "input_quantity_kg = :input_quantity_kg, "
                  "output_oil_l = :output_oil_l, "
                  "status = :status "
                  "WHERE id = :id");

    query.bindValue(":id", m_id);
    query.bindValue(":lot_id", m_lotId);
    query.bindValue(":machine_id", m_machineId);
    query.bindValue(":target_citerne_id", m_targetCiterneId);
    query.bindValue(":extracted_at", m_extractedAt);
    query.bindValue(":input_quantity_kg", m_inputQuantityKg);
    query.bindValue(":output_oil_l", m_outputOilL);
    query.bindValue(":status", m_status.isEmpty() ? QStringLiteral("PLANIFIE") : m_status);

    if (!query.exec()) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

bool Extraction::supprimer(QSqlDatabase &db, int id, QString *errorMessage)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM extraction WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    return true;
}

QList<Extraction> Extraction::afficher(QSqlDatabase &db, QString *errorMessage)
{
    QList<Extraction> list;

    QSqlQuery query(db);
    if (!query.exec("SELECT id, lot_id, machine_id, target_citerne_id, extracted_at, input_quantity_kg, output_oil_l, status "
                    "FROM extraction ORDER BY id DESC")) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return list;
    }

    while (query.next()) {
        list.append(Extraction(query.value(0).toInt(),
                               query.value(1).toInt(),
                               query.value(2).toInt(),
                               query.value(3).toInt(),
                               query.value(4).toDate(),
                               query.value(5).toDouble(),
                               query.value(6).toDouble(),
                               query.value(7).toString()));
    }

    return list;
}

QList<Extraction> Extraction::rechercher(QSqlDatabase &db, const QString &term, QString *errorMessage)
{
    QList<Extraction> list;

    QSqlQuery query(db);
    query.prepare("SELECT id, lot_id, machine_id, target_citerne_id, extracted_at, input_quantity_kg, output_oil_l, status "
                  "FROM extraction "
                  "WHERE TO_CHAR(id) LIKE :term "
                  "OR TO_CHAR(lot_id) LIKE :term "
                  "OR LOWER(status) LIKE LOWER(:term) "
                  "ORDER BY id DESC");
    query.bindValue(":term", "%" + term + "%");

    if (!query.exec()) {
        if (errorMessage != nullptr) {
            *errorMessage = query.lastError().text();
        }
        return list;
    }

    while (query.next()) {
        list.append(Extraction(query.value(0).toInt(),
                               query.value(1).toInt(),
                               query.value(2).toInt(),
                               query.value(3).toInt(),
                               query.value(4).toDate(),
                               query.value(5).toDouble(),
                               query.value(6).toDouble(),
                               query.value(7).toString()));
    }

    return list;
}
