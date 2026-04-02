#include "reception.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QDebug>

/* ================= CONSTRUCTEURS ================= */

Reception::Reception()
{
    id = 0;
    lot_number = "";
    quantity_kg = 0;
    quality_grade = "";
    client_id = 0;
    status = "";
}

Reception::Reception(int id, QString lot_number, float quantity_kg,
                     QString quality_grade, int client_id, QString status)
{
    this->id = id;
    this->lot_number = lot_number;
    this->quantity_kg = quantity_kg;
    this->quality_grade = quality_grade;
    this->client_id = client_id;
    this->status = status;
}

/* ================= GETTERS ================= */

int Reception::getId() const { return id; }
QString Reception::getLotNumber() const { return lot_number; }
float Reception::getQuantityKg() const { return quantity_kg; }
QString Reception::getQualityGrade() const { return quality_grade; }
int Reception::getClientId() const { return client_id; }
QString Reception::getStatus() const { return status; }

/* ================= SETTERS ================= */

void Reception::setId(int id) { this->id = id; }
void Reception::setLotNumber(QString lot_number) { this->lot_number = lot_number; }
void Reception::setQuantityKg(float quantity_kg) { this->quantity_kg = quantity_kg; }
void Reception::setQualityGrade(QString quality_grade) { this->quality_grade = quality_grade; }
void Reception::setClientId(int client_id) { this->client_id = client_id; }
void Reception::setStatus(QString status) { this->status = status; }

/* ================= AJOUTER ================= */

bool Reception::ajouter()
{
    QSqlQuery query;

    // Check if database is open
    if (!QSqlDatabase::database().isOpen()) {
        qDebug() << "Database is not open!";
        return false;
    }

    // Get the next ID value
    int nextId = getNextId();
    if (nextId <= 0) {
        qDebug() << "Failed to get next ID";
        return false;
    }

    qDebug() << "Inserting reception with ID:" << nextId;

    // Insert with explicit ID (no sequence needed)
    query.prepare("INSERT INTO reception "
                  "(id, lot_number, quantity_kg, quality_grade, client_id, status, received_at) "
                  "VALUES (:id, :lot_number, :quantity_kg, "
                  ":quality_grade, :client_id, :status, SYSDATE)");

    query.bindValue(":id", nextId);
    query.bindValue(":lot_number", lot_number);
    query.bindValue(":quantity_kg", quantity_kg);
    query.bindValue(":quality_grade", quality_grade);
    query.bindValue(":client_id", client_id);
    query.bindValue(":status", status.isEmpty() ? "REÇU" : status);

    if(!query.exec())
    {
        qDebug() << "Erreur ajout réception:" << query.lastError();
        qDebug() << "Query:" << query.lastQuery();
        qDebug() << "Bound values - ID:" << nextId
                 << "Lot:" << lot_number
                 << "Qty:" << quantity_kg
                 << "Qual:" << quality_grade
                 << "Client:" << client_id;
        return false;
    }

    qDebug() << "Reception added successfully with ID:" << nextId;
    return true;
}

/* ================= GET NEXT ID ================= */

int Reception::getNextId()
{
    QSqlQuery query;

    // Try to get max ID + 1
    if (query.exec("SELECT NVL(MAX(id), 0) + 1 FROM reception")) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    }

    qDebug() << "Failed to get next ID:" << query.lastError();
    return -1;
}

/* ================= AFFICHER ================= */

QSqlQueryModel* Reception::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    // Check if database is open
    if (!QSqlDatabase::database().isOpen()) {
        qDebug() << "Database is not open in afficher!";
        return model;
    }

    model->setQuery("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY') as date_reception, "
                    "quantity_kg, quality_grade, client_id, status FROM reception ORDER BY id DESC");

    // Check for errors
    QSqlError err = model->lastError();
    if (err.isValid()) {
        qDebug() << "Error in afficher:" << err.text();
        qDebug() << "Database error:" << err.databaseText();
        qDebug() << "Driver error:" << err.driverText();
    } else {
        qDebug() << "Afficher successful, rows:" << model->rowCount();
    }

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Lot"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Quantité (kg)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Qualité"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Client"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Statut"));

    return model;
}

/* ================= SUPPRIMER ================= */

bool Reception::supprimer(int id)
{
    QSqlQuery query;

    query.prepare("DELETE FROM reception WHERE id = :id");
    query.bindValue(":id", id);

    if(!query.exec())
    {
        qDebug() << "Erreur suppression réception:" << query.lastError();
        return false;
    }

    qDebug() << "Reception deleted successfully, ID:" << id;
    return true;
}

/* ================= MODIFIER ================= */

bool Reception::modifier()
{
    QSqlQuery query;

    query.prepare("UPDATE reception SET "
                  "lot_number = :lot_number, "
                  "quantity_kg = :quantity_kg, "
                  "quality_grade = :quality_grade, "
                  "client_id = :client_id, "
                  "status = :status "
                  "WHERE id = :id");

    query.bindValue(":id", id);
    query.bindValue(":lot_number", lot_number);
    query.bindValue(":quantity_kg", quantity_kg);
    query.bindValue(":quality_grade", quality_grade);
    query.bindValue(":client_id", client_id);
    query.bindValue(":status", status);

    if(!query.exec())
    {
        qDebug() << "Erreur modification réception:" << query.lastError();
        return false;
    }

    qDebug() << "Reception modified successfully, ID:" << id;
    return true;
}

/* ================= RECHERCHER ================= */

QSqlQueryModel* Reception::rechercher(const QString &lot)
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY'), "
                  "quantity_kg, quality_grade, client_id, status "
                  "FROM reception WHERE LOWER(lot_number) LIKE LOWER(:lot) "
                  "ORDER BY id DESC");

    query.bindValue(":lot", "%" + lot + "%");

    if(query.exec())
    {
        model->setQuery(std::move(query));
        qDebug() << "Search found" << model->rowCount() << "results for:" << lot;
    }
    else
    {
        qDebug() << "Error in rechercher:" << query.lastError();
    }

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Lot"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Quantité (kg)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Qualité"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Client"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Statut"));

    return model;
}

/* ================= GET BY ID ================= */

Reception Reception::getById(int id)
{
    Reception r;
    QSqlQuery query;

    query.prepare("SELECT id, lot_number, quantity_kg, quality_grade, client_id, status "
                  "FROM reception WHERE id = :id");
    query.bindValue(":id", id);

    if(query.exec() && query.next())
    {
        r.setId(query.value(0).toInt());
        r.setLotNumber(query.value(1).toString());
        r.setQuantityKg(query.value(2).toFloat());
        r.setQualityGrade(query.value(3).toString());
        r.setClientId(query.value(4).toInt());
        r.setStatus(query.value(5).toString());
        qDebug() << "Reception found with ID:" << id;
    }
    else
    {
        qDebug() << "No reception found with ID:" << id;
    }

    return r;
}
