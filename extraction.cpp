#include "extraction.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

Extraction::Extraction()
    : m_id(0), m_lotId(0), m_machineId(0), m_targetCiterneId(0),
      m_extractedAt(QDate::currentDate()),
      m_inputQuantityKg(0.0), m_outputOilL(0.0), m_status("PLANIFIE")
{}

Extraction::Extraction(int id, int lotId, int machineId, int targetCiterneId,
                       const QDate &extractedAt, double inputQuantityKg,
                       double outputOilL, const QString &status)
    : m_id(id), m_lotId(lotId), m_machineId(machineId),
      m_targetCiterneId(targetCiterneId), m_extractedAt(extractedAt),
      m_inputQuantityKg(inputQuantityKg), m_outputOilL(outputOilL),
      m_status(status)
{}

int    Extraction::id()               const { return m_id; }
int    Extraction::lotId()            const { return m_lotId; }
int    Extraction::machineId()        const { return m_machineId; }
int    Extraction::targetCiterneId()  const { return m_targetCiterneId; }
QDate  Extraction::extractedAt()      const { return m_extractedAt; }
double Extraction::inputQuantityKg()  const { return m_inputQuantityKg; }
double Extraction::outputOilL()       const { return m_outputOilL; }
QString Extraction::status()          const { return m_status; }

void Extraction::setId(int v)                    { m_id = v; }
void Extraction::setLotId(int v)                 { m_lotId = v; }
void Extraction::setMachineId(int v)             { m_machineId = v; }
void Extraction::setTargetCiterneId(int v)       { m_targetCiterneId = v; }
void Extraction::setExtractedAt(const QDate &v)  { m_extractedAt = v; }
void Extraction::setInputQuantityKg(double v)    { m_inputQuantityKg = v; }
void Extraction::setOutputOilL(double v)         { m_outputOilL = v; }
void Extraction::setStatus(const QString &v)     { m_status = v; }

int Extraction::getNextId(QSqlDatabase &db, QString *errorMessage)
{
    QSqlQuery q(db);
    // Use the Oracle sequence defined in oracle_schema.sql
    if (!q.exec("SELECT SEQ_EXTRACTION.NEXTVAL FROM dual")) {
        if (errorMessage) *errorMessage = q.lastError().text();
        return -1;
    }
    if (!q.next()) {
        if (errorMessage) *errorMessage = "Impossible d'obtenir le prochain ID extraction.";
        return -1;
    }
    return q.value(0).toInt();
}

bool Extraction::ajouter(QSqlDatabase &db, QString *errorMessage) const
{
    // Let the trigger TRG_EXTRACTION_BI handle the ID via SEQ_EXTRACTION
    // We pass NULL for ID so the trigger fires
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO EXTRACTION "
        "(LOT_ID, MACHINE_ID, TARGET_CITERNE_ID, EXTRACTED_AT, "
        " INPUT_QUANTITY_KG, OUTPUT_OIL_L, YIELD_PERCENTAGE, QUALITY_RESULT, STATUS) "
        "VALUES (:lot_id, :machine_id, :target_citerne_id, :extracted_at, "
        "        :input_quantity_kg, :output_oil_l, :yield_pct, :quality_result, :status)"
    );

    const double yield = (m_inputQuantityKg > 0)
                         ? (m_outputOilL / m_inputQuantityKg) * 100.0
                         : 0.0;

    query.bindValue(":lot_id",           m_lotId);
    query.bindValue(":machine_id",       m_machineId);
    query.bindValue(":target_citerne_id",m_targetCiterneId);
    query.bindValue(":extracted_at",     m_extractedAt.toString("yyyy-MM-dd"));
    query.bindValue(":input_quantity_kg",m_inputQuantityKg);
    query.bindValue(":output_oil_l",     m_outputOilL);
    query.bindValue(":yield_pct",        yield);
    query.bindValue(":quality_result",   QVariant(QMetaType(QMetaType::QString))); // NULL
    query.bindValue(":status",           m_status.isEmpty() ? "PLANIFIE" : m_status);

    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

bool Extraction::modifier(QSqlDatabase &db, QString *errorMessage) const
{
    QSqlQuery query(db);
    query.prepare(
        "UPDATE EXTRACTION SET "
        "LOT_ID = :lot_id, MACHINE_ID = :machine_id, "
        "TARGET_CITERNE_ID = :target_citerne_id, "
        "EXTRACTED_AT = :extracted_at, "
        "INPUT_QUANTITY_KG = :input_quantity_kg, "
        "OUTPUT_OIL_L = :output_oil_l, "
        "YIELD_PERCENTAGE = :yield_pct, "
        "STATUS = :status "
        "WHERE ID = :id"
    );

    const double yield = (m_inputQuantityKg > 0)
                         ? (m_outputOilL / m_inputQuantityKg) * 100.0
                         : 0.0;

    query.bindValue(":id",               m_id);
    query.bindValue(":lot_id",           m_lotId);
    query.bindValue(":machine_id",       m_machineId);
    query.bindValue(":target_citerne_id",m_targetCiterneId);
    query.bindValue(":extracted_at",     m_extractedAt.toString("yyyy-MM-dd"));
    query.bindValue(":input_quantity_kg",m_inputQuantityKg);
    query.bindValue(":output_oil_l",     m_outputOilL);
    query.bindValue(":yield_pct",        yield);
    query.bindValue(":status",           m_status.isEmpty() ? "PLANIFIE" : m_status);

    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

bool Extraction::supprimer(QSqlDatabase &db, int id, QString *errorMessage)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM EXTRACTION WHERE ID = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return false;
    }
    return true;
}

QList<Extraction> Extraction::afficher(QSqlDatabase &db, QString *errorMessage)
{
    QList<Extraction> list;
    QSqlQuery query(db);
    if (!query.exec(
            "SELECT ID, LOT_ID, MACHINE_ID, TARGET_CITERNE_ID, "
            "EXTRACTED_AT, INPUT_QUANTITY_KG, OUTPUT_OIL_L, STATUS "
            "FROM EXTRACTION ORDER BY ID DESC")) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return list;
    }
    while (query.next()) {
        list.append(Extraction(
            query.value(0).toInt(),
            query.value(1).toInt(),
            query.value(2).toInt(),
            query.value(3).toInt(),
            query.value(4).toDate(),
            query.value(5).toDouble(),
            query.value(6).toDouble(),
            query.value(7).toString()
        ));
    }
    return list;
}

QList<Extraction> Extraction::rechercher(QSqlDatabase &db, const QString &term, QString *errorMessage)
{
    QList<Extraction> list;
    QSqlQuery query(db);
    query.prepare(
        "SELECT ID, LOT_ID, MACHINE_ID, TARGET_CITERNE_ID, "
        "EXTRACTED_AT, INPUT_QUANTITY_KG, OUTPUT_OIL_L, STATUS "
        "FROM EXTRACTION "
        "WHERE TO_CHAR(ID) LIKE :term "
        "OR TO_CHAR(LOT_ID) LIKE :term "
        "OR LOWER(STATUS) LIKE LOWER(:term) "
        "ORDER BY ID DESC"
    );
    query.bindValue(":term", "%" + term + "%");
    if (!query.exec()) {
        if (errorMessage) *errorMessage = query.lastError().text();
        return list;
    }
    while (query.next()) {
        list.append(Extraction(
            query.value(0).toInt(),
            query.value(1).toInt(),
            query.value(2).toInt(),
            query.value(3).toInt(),
            query.value(4).toDate(),
            query.value(5).toDouble(),
            query.value(6).toDouble(),
            query.value(7).toString()
        ));
    }
    return list;
}

QString Extraction::planification() const
{
    return QString("Extraction ID:%1 | Lot:%2 | Machine:%3 | Citerne:%4 | Date:%5 | Entrée:%6 kg | Statut:%7")
        .arg(m_id).arg(m_lotId).arg(m_machineId).arg(m_targetCiterneId)
        .arg(m_extractedAt.toString("dd/MM/yyyy"))
        .arg(m_inputQuantityKg).arg(m_status);
}

double Extraction::taux() const
{
    if (m_inputQuantityKg <= 0) return 0.0;
    return (m_outputOilL / m_inputQuantityKg) * 100.0;
}
