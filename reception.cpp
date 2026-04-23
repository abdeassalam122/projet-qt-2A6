#include "reception.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QDebug>
#include <QPrinter>
#include <QTextDocument>
#include <QFile>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>

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

    if (!QSqlDatabase::database().isOpen()) {
        qDebug() << "Database is not open!";
        return false;
    }

    int nextId = getNextId();
    if (nextId <= 0) {
        qDebug() << "Failed to get next ID";
        return false;
    }

    qDebug() << "Inserting reception with ID:" << nextId;

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
        return false;
    }

    qDebug() << "Reception added successfully with ID:" << nextId;
    return true;
}

/* ================= GET NEXT ID ================= */

int Reception::getNextId()
{
    QSqlQuery query;

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

    if (!QSqlDatabase::database().isOpen()) {
        qDebug() << "Database is not open in afficher!";
        return model;
    }

    model->setQuery("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY') as date_reception, "
                    "quantity_kg, quality_grade, client_id, status FROM reception ORDER BY id DESC");

    QSqlError err = model->lastError();
    if (err.isValid()) {
        qDebug() << "Error in afficher:" << err.text();
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

/* ================= FILTRER PAR STATUT ET DATE ================= */

QSqlQueryModel* Reception::filtrerParStatutEtDate(const QString& statut, const QString& dateDebut, const QString& dateFin)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;
    QString queryStr = "SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY') as date_reception, "
                       "quantity_kg, quality_grade, client_id, status "
                       "FROM reception WHERE 1=1";

    if (!statut.isEmpty()) {
        queryStr += " AND UPPER(status) = UPPER(:statut)";
    }

    if (!dateDebut.isEmpty()) {
        queryStr += " AND received_at >= TO_DATE(:dateDebut, 'DD/MM/YYYY')";
    }

    if (!dateFin.isEmpty()) {
        queryStr += " AND received_at <= TO_DATE(:dateFin, 'DD/MM/YYYY')";
    }

    queryStr += " ORDER BY received_at DESC";

    query.prepare(queryStr);

    if (!statut.isEmpty()) {
        query.bindValue(":statut", statut);
    }

    if (!dateDebut.isEmpty()) {
        query.bindValue(":dateDebut", dateDebut);
    }

    if (!dateFin.isEmpty()) {
        query.bindValue(":dateFin", dateFin);
    }

    if(query.exec())
    {
        model->setQuery(std::move(query));
        qDebug() << "Filtrage par statut:" << statut
                 << "et date:" << dateDebut << "->" << dateFin
                 << "-" << model->rowCount() << "résultats";
    }
    else
    {
        qDebug() << "Erreur filtrage:" << query.lastError();
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

/* ================= RECHERCHE AVEC FILTRES ================= */

QSqlQueryModel* Reception::rechercherAvecFiltres(const QString& rechercheLot,
                                                 const QString& statut,
                                                 const QString& dateDebut,
                                                 const QString& dateFin)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;
    QStringList conditions;
    QString queryStr = "SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY') as date_reception, "
                       "quantity_kg, quality_grade, client_id, status "
                       "FROM reception WHERE 1=1";

    if (!rechercheLot.isEmpty()) {
        conditions << "LOWER(lot_number) LIKE LOWER(:rechercheLot)";
    }

    if (!statut.isEmpty()) {
        conditions << "UPPER(status) = UPPER(:statut)";
    }

    if (!dateDebut.isEmpty()) {
        conditions << "received_at >= TO_DATE(:dateDebut, 'DD/MM/YYYY')";
    }

    if (!dateFin.isEmpty()) {
        conditions << "received_at <= TO_DATE(:dateFin, 'DD/MM/YYYY')";
    }

    if (!conditions.isEmpty()) {
        queryStr += " AND " + conditions.join(" AND ");
    }

    queryStr += " ORDER BY received_at DESC";

    query.prepare(queryStr);

    if (!rechercheLot.isEmpty()) {
        query.bindValue(":rechercheLot", "%" + rechercheLot + "%");
    }

    if (!statut.isEmpty()) {
        query.bindValue(":statut", statut);
    }

    if (!dateDebut.isEmpty()) {
        query.bindValue(":dateDebut", dateDebut);
    }

    if (!dateFin.isEmpty()) {
        query.bindValue(":dateFin", dateFin);
    }

    if(query.exec())
    {
        model->setQuery(std::move(query));
        qDebug() << "Recherche avancée - Lot:" << rechercheLot
                 << "Statut:" << statut
                 << "Date:" << dateDebut << "->" << dateFin
                 << "- Résultats:" << model->rowCount();
    }
    else
    {
        qDebug() << "Erreur recherche avancée:" << query.lastError();
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

/* ================= STATISTIQUES SIMPLES ================= */

QMap<QString, int> Reception::statistiquesParStatut()
{
    QMap<QString, int> stats;
    QSqlQuery query;

    query.prepare("SELECT status, COUNT(*) as nombre "
                  "FROM reception "
                  "GROUP BY status "
                  "ORDER BY nombre DESC");

    if(query.exec())
    {
        while(query.next())
        {
            QString statut = query.value(0).toString();
            int nombre = query.value(1).toInt();
            stats[statut] = nombre;
            qDebug() << "Statut:" << statut << "- Nombre:" << nombre;
        }
    }
    else
    {
        qDebug() << "Erreur statistiques par statut:" << query.lastError();
    }

    return stats;
}

QMap<QString, float> Reception::statistiquesQuantiteParStatut()
{
    QMap<QString, float> stats;
    QSqlQuery query;

    query.prepare("SELECT status, SUM(quantity_kg) as total_quantite "
                  "FROM reception "
                  "GROUP BY status");

    if(query.exec())
    {
        while(query.next())
        {
            QString statut = query.value(0).toString();
            float total = query.value(1).toFloat();
            stats[statut] = total;
            qDebug() << "Statut:" << statut << "- Total:" << total << "kg";
        }
    }
    else
    {
        qDebug() << "Erreur statistiques quantité:" << query.lastError();
    }

    return stats;
}

/* ================= STATISTIQUES DE PERFORMANCE ================= */

Reception::PerformanceStats Reception::getPerformanceStats()
{
    PerformanceStats stats;
    QSqlQuery query;

    stats.totalReceptions = 0;
    stats.totalQuantiteKg = 0;
    stats.moyenneQuantiteParReception = 0;
    stats.dureeMoyenneTraitement = 0;
    stats.tauxReussite = 0;
    stats.meilleurMois = "";
    stats.meilleurStatut = "";

    query.prepare("SELECT COUNT(*) as total, "
                  "SUM(quantity_kg) as total_kg, "
                  "AVG(quantity_kg) as moyenne_kg "
                  "FROM reception");

    if(query.exec() && query.next())
    {
        stats.totalReceptions = query.value(0).toInt();
        stats.totalQuantiteKg = query.value(1).toFloat();
        stats.moyenneQuantiteParReception = query.value(2).toFloat();
    }

    query.prepare("SELECT TO_CHAR(received_at, 'MM/YYYY') as mois, COUNT(*) as total "
                  "FROM reception "
                  "GROUP BY TO_CHAR(received_at, 'MM/YYYY') "
                  "ORDER BY total DESC "
                  "FETCH FIRST 1 ROW ONLY");

    if(query.exec() && query.next())
    {
        stats.meilleurMois = query.value(0).toString();
    }

    query.prepare("SELECT status, COUNT(*) as total "
                  "FROM reception "
                  "GROUP BY status "
                  "ORDER BY total DESC "
                  "FETCH FIRST 1 ROW ONLY");

    if(query.exec() && query.next())
    {
        stats.meilleurStatut = query.value(0).toString();
    }

    int recu = 0;
    query.prepare("SELECT COUNT(*) FROM reception WHERE UPPER(status) = 'REÇU'");
    if(query.exec() && query.next())
    {
        recu = query.value(0).toInt();
        stats.tauxReussite = (stats.totalReceptions > 0) ? (recu * 100.0f / stats.totalReceptions) : 0;
    }

    return stats;
}

/* ================= PERFORMANCE PAR MOIS ================= */

QSqlQueryModel* Reception::getPerformanceParMois()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT TO_CHAR(received_at, 'MM/YYYY') as \"Mois\", "
                  "COUNT(*) as \"Nb Réceptions\", "
                  "SUM(quantity_kg) as \"Total (kg)\", "
                  "ROUND(AVG(quantity_kg), 2) as \"Moyenne (kg)\" "
                  "FROM reception "
                  "GROUP BY TO_CHAR(received_at, 'MM/YYYY') "
                  "ORDER BY TO_DATE(TO_CHAR(received_at, 'MM/YYYY'), 'MM/YYYY') DESC");

    if(query.exec())
    {
        model->setQuery(std::move(query));
    }

    return model;
}

/* ================= PERFORMANCE PAR CLIENT ================= */

QSqlQueryModel* Reception::getPerformanceParClient()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT c.nom as \"Client\", "
                  "COUNT(r.id) as \"Nb Réceptions\", "
                  "SUM(r.quantity_kg) as \"Total (kg)\", "
                  "ROUND(AVG(r.quantity_kg), 2) as \"Moyenne (kg)\" "
                  "FROM reception r "
                  "LEFT JOIN client c ON r.client_id = c.id "
                  "GROUP BY c.nom, r.client_id "
                  "ORDER BY SUM(r.quantity_kg) DESC");

    if(query.exec())
    {
        model->setQuery(std::move(query));
    }

    return model;
}

/* ================= PERFORMANCE PAR QUALITE ================= */

QSqlQueryModel* Reception::getPerformanceParQualite()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT quality_grade as \"Qualité\", "
                  "COUNT(*) as \"Nb Réceptions\", "
                  "SUM(quantity_kg) as \"Total (kg)\", "
                  "ROUND(AVG(quantity_kg), 2) as \"Moyenne (kg)\" "
                  "FROM reception "
                  "GROUP BY quality_grade "
                  "ORDER BY SUM(quantity_kg) DESC");

    if(query.exec())
    {
        model->setQuery(std::move(query));
    }

    return model;
}

/* ================= STATISTIQUES PAR STATUT MODEL ================= */

QSqlQueryModel* Reception::getStatistiquesParStatutModel()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT status as \"Statut\", "
                  "COUNT(*) as \"Nombre\", "
                  "SUM(quantity_kg) as \"Total (kg)\", "
                  "ROUND(AVG(quantity_kg), 2) as \"Moyenne (kg)\", "
                  "MIN(quantity_kg) as \"Minimum (kg)\", "
                  "MAX(quantity_kg) as \"Maximum (kg)\" "
                  "FROM reception "
                  "GROUP BY status "
                  "ORDER BY COUNT(*) DESC");

    if(query.exec())
    {
        model->setQuery(std::move(query));
        qDebug() << "Modèle statistiques créé avec" << model->rowCount() << "lignes";
    }
    else
    {
        qDebug() << "Erreur création modèle statistiques:" << query.lastError();
    }

    return model;
}

/* ================= TOP CLIENTS ================= */

QMap<QString, float> Reception::getTopClients(int limite)
{
    QMap<QString, float> topClients;
    QSqlQuery query;

    QString queryStr = "SELECT c.nom, SUM(r.quantity_kg) as total_kg "
                       "FROM reception r "
                       "LEFT JOIN client c ON r.client_id = c.id "
                       "GROUP BY c.nom, r.client_id "
                       "ORDER BY total_kg DESC "
                       "FETCH FIRST " + QString::number(limite) + " ROWS ONLY";

    query.prepare(queryStr);

    if(query.exec())
    {
        while(query.next())
        {
            QString client = query.value(0).toString();
            float quantite = query.value(1).toFloat();
            topClients[client] = quantite;
        }
    }

    return topClients;
}

/* ================= TENDANCE MENSUELLE ================= */

float Reception::getTendanceMensuelle(int mois, int annee)
{
    QSqlQuery query;

    int moisPrec = (mois == 1) ? 12 : mois - 1;
    int anneePrec = (mois == 1) ? annee - 1 : annee;

    query.prepare("SELECT "
                  "(SELECT COUNT(*) FROM reception WHERE EXTRACT(MONTH FROM received_at) = :mois AND EXTRACT(YEAR FROM received_at) = :annee) - "
                  "(SELECT COUNT(*) FROM reception WHERE EXTRACT(MONTH FROM received_at) = :moisPrec AND EXTRACT(YEAR FROM received_at) = :anneePrec) "
                  "FROM DUAL");

    query.bindValue(":mois", mois);
    query.bindValue(":annee", annee);
    query.bindValue(":moisPrec", moisPrec);
    query.bindValue(":anneePrec", anneePrec);

    if(query.exec() && query.next())
    {
        return query.value(0).toFloat();
    }

    return 0;
}

/* ================= DELAIS TRAITEMENT ================= */

QSqlQueryModel* Reception::getDelaisTraitement()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT status as \"Statut\", "
                  "COUNT(*) as \"Nombre\" "
                  "FROM reception "
                  "GROUP BY status "
                  "ORDER BY COUNT(*) DESC");

    if(query.exec())
    {
        model->setQuery(std::move(query));
    }

    return model;
}

/* ================= GÉNÉRATION PDF - PRIVÉ ================= */

QString Reception::getHTMLHeader()
{
    return "<!DOCTYPE html>"
           "<html>"
           "<head>"
           "<meta charset='UTF-8'>"
           "<style>"
           "body { font-family: Arial, sans-serif; margin: 40px; }"
           "h1 { color: #2c3e50; text-align: center; border-bottom: 2px solid #3498db; padding-bottom: 10px; }"
           "h2 { color: #34495e; margin-top: 30px; }"
           "table { width: 100%; border-collapse: collapse; margin: 20px 0; }"
           "th { background-color: #3498db; color: white; padding: 12px; text-align: left; }"
           "td { border: 1px solid #ddd; padding: 8px; }"
           "tr:nth-child(even) { background-color: #f2f2f2; }"
           ".stats-box { background-color: #ecf0f1; padding: 15px; margin: 20px 0; border-radius: 5px; }"
           ".stat-item { margin: 10px 0; font-size: 14px; }"
           ".stat-label { font-weight: bold; color: #2c3e50; }"
           ".footer { text-align: center; margin-top: 50px; font-size: 12px; color: #7f8c8d; border-top: 1px solid #ddd; padding-top: 20px; }"
           "</style>"
           "</head>"
           "<body>";
}

QString Reception::getHTMLFooter()
{
    return "<div class='footer'>"
           "<p>Document généré le " + QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss") + "</p>"
                                                                            "<p>© Système de Gestion des Réceptions</p>"
                                                                            "</div>"
                                                                            "</body>"
                                                                            "</html>";
}

/* ================= EXPORTER PDF COMPLET ================= */

bool Reception::exporterPDF(const QString& cheminFichier)
{
    QString html = getHTMLHeader();

    html += "<h1>Rapport Complet des Réceptions</h1>";

    html += "<h2>📊 Statistiques Générales</h2>";
    html += "<div class='stats-box'>";

    PerformanceStats stats = getPerformanceStats();
    html += "<div class='stat-item'><span class='stat-label'>Total des réceptions :</span> " +
            QString::number(stats.totalReceptions) + "</div>";
    html += "<div class='stat-item'><span class='stat-label'>Quantité totale :</span> " +
            QString::number(stats.totalQuantiteKg, 'f', 2) + " kg</div>";
    html += "<div class='stat-item'><span class='stat-label'>Quantité moyenne :</span> " +
            QString::number(stats.moyenneQuantiteParReception, 'f', 2) + " kg</div>";
    html += "<div class='stat-item'><span class='stat-label'>Taux de réussite :</span> " +
            QString::number(stats.tauxReussite, 'f', 2) + "%</div>";
    html += "</div>";

    html += "<h2>📋 Liste des Réceptions</h2>";
    QSqlQueryModel* modelReceptions = afficher();
    if(modelReceptions->rowCount() > 0)
    {
        html += "<table>";
        html += "<tr><th>ID</th><th>Lot</th><th>Date</th><th>Quantité (kg)</th><th>Qualité</th><th>Client</th><th>Statut</th></tr>";
        for(int i = 0; i < modelReceptions->rowCount(); i++)
        {
            html += "<tr>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 0)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 1)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 2)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 3)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 4)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 5)).toString() + "</td>";
            html += "<td>" + modelReceptions->data(modelReceptions->index(i, 6)).toString() + "</td>";
            html += "</tr>";
        }
        html += "</table>";
    }
    delete modelReceptions;

    html += getHTMLFooter();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(cheminFichier);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    qDebug() << "PDF généré avec succès:" << cheminFichier;
    return true;
}

/* ================= EXPORTER STATISTIQUES PDF ================= */

bool Reception::exporterStatistiquesPDF(const QString& cheminFichier)
{
    QString html = getHTMLHeader();

    html += "<h1>📈 Rapport Statistique des Réceptions</h1>";

    PerformanceStats stats = getPerformanceStats();
    html += "<div class='stats-box'>";
    html += "<h2>Indicateurs Clés</h2>";
    html += "<div class='stat-item'><span class='stat-label'>📦 Total réceptions :</span> " +
            QString::number(stats.totalReceptions) + "</div>";
    html += "<div class='stat-item'><span class='stat-label'>⚖️ Quantité totale :</span> " +
            QString::number(stats.totalQuantiteKg, 'f', 2) + " kg</div>";
    html += "<div class='stat-item'><span class='stat-label'>📊 Moyenne par réception :</span> " +
            QString::number(stats.moyenneQuantiteParReception, 'f', 2) + " kg</div>";
    html += "<div class='stat-item'><span class='stat-label'>✅ Taux de réussite :</span> " +
            QString::number(stats.tauxReussite, 'f', 2) + "%</div>";
    html += "</div>";

    html += getHTMLFooter();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(cheminFichier);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    qDebug() << "PDF des statistiques généré:" << cheminFichier;
    return true;
}

/* ================= EXPORTER RECEPTIONS PDF ================= */

bool Reception::exporterReceptionsPDF(const QString& cheminFichier, QSqlQueryModel* model)
{
    QString html = getHTMLHeader();

    html += "<h1>📋 Liste des Réceptions</h1>";

    if(model == nullptr)
    {
        model = afficher();
    }

    if(model->rowCount() > 0)
    {
        html += "<table>";
        html += "<tr><th>ID</th><th>Lot</th><th>Date</th><th>Quantité (kg)</th><th>Qualité</th><th>Client</th><th>Statut</th></tr>";

        for(int i = 0; i < model->rowCount(); i++)
        {
            html += "<tr>";
            html += "<td>" + model->data(model->index(i, 0)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 1)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 2)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 3)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 4)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 5)).toString() + "</td>";
            html += "<td>" + model->data(model->index(i, 6)).toString() + "</td>";
            html += "</tr>";
        }
        html += "</table>";

        html += "<p><strong>Total:</strong> " + QString::number(model->rowCount()) + " réception(s)</p>";
    }
    else
    {
        html += "<p>Aucune réception trouvée.</p>";
    }

    if(model != nullptr && model->parent() == nullptr)
    {
        delete model;
    }

    html += getHTMLFooter();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(cheminFichier);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    qDebug() << "PDF des réceptions généré:" << cheminFichier;
    return true;
}

/* ================= TRIER PAR ================= */

QSqlQueryModel* Reception::trierPar(const QString& colonne, Qt::SortOrder ordre)
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QMap<QString, QString> colonnesMap;
    colonnesMap["id"] = "id";
    colonnesMap["lot"] = "lot_number";
    colonnesMap["date"] = "received_at";
    colonnesMap["quantite"] = "quantity_kg";
    colonnesMap["qualite"] = "quality_grade";
    colonnesMap["client"] = "client_id";
    colonnesMap["statut"] = "status";

    QString colonneSQL = colonnesMap.value(colonne.toLower(), "id");
    QString ordreSQL = (ordre == Qt::AscendingOrder) ? "ASC" : "DESC";

    QString queryStr = QString("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY'), "
                               "quantity_kg, quality_grade, client_id, status "
                               "FROM reception ORDER BY %1 %2").arg(colonneSQL, ordreSQL);

    model->setQuery(queryStr);

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Lot"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Quantité (kg)"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Qualité"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Client"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Statut"));

    return model;
}

/* ================= FILTRER PAR STATUT ================= */

QSqlQueryModel* Reception::filtrerParStatut(const QString& statut)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;

    query.prepare("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY'), "
                  "quantity_kg, quality_grade, client_id, status "
                  "FROM reception WHERE UPPER(status) = UPPER(:statut) "
                  "ORDER BY received_at DESC");

    query.bindValue(":statut", statut);

    if(query.exec())
    {
        model->setQuery(std::move(query));
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

/* ================= FILTRER PAR DATE ================= */

QSqlQueryModel* Reception::filtrerParDate(const QString& dateDebut, const QString& dateFin)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;

    query.prepare("SELECT id, lot_number, TO_CHAR(received_at, 'DD/MM/YYYY'), "
                  "quantity_kg, quality_grade, client_id, status "
                  "FROM reception WHERE received_at >= TO_DATE(:dateDebut, 'DD/MM/YYYY') "
                  "AND received_at <= TO_DATE(:dateFin, 'DD/MM/YYYY') "
                  "ORDER BY received_at DESC");

    query.bindValue(":dateDebut", dateDebut);
    query.bindValue(":dateFin", dateFin);

    if(query.exec())
    {
        model->setQuery(std::move(query));
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

/* ================= PRÉDICTION PRODUCTION HUILE (ML simple) ================= */

Reception::RendementPrediction Reception::predireProductionHuile(int receptionId)
{
    RendementPrediction result;
    result.quantiteOlivesKg = 0;
    result.rendementPredicted = 0;
    result.huileProduiteLitres = 0;
    result.confiance = 0;
    result.methode = "Régression linéaire historique";
    result.qualiteOlives = "";

    // 1. Récupérer la réception cible
    QSqlQuery queryReception;
    queryReception.prepare("SELECT id, lot_number, quantity_kg, quality_grade, status, received_at "
                           "FROM reception WHERE id = :id");
    queryReception.bindValue(":id", receptionId);

    if (!queryReception.exec() || !queryReception.next()) {
        qDebug() << "Réception non trouvée:" << receptionId;
        return result;
    }

    result.quantiteOlivesKg = queryReception.value(2).toFloat();
    result.qualiteOlives = queryReception.value(3).toString().toUpper();
    QString currentLot = queryReception.value(1).toString();
    QDate currentDate = queryReception.value(5).toDate();

    // 2. Récupérer l'historique des réceptions précédentes pour apprentissage
    QSqlQuery historyQuery;
    historyQuery.prepare("SELECT id, quantity_kg, quality_grade, status, received_at "
                         "FROM reception "
                         "WHERE id != :currentId "
                         "ORDER BY received_at DESC "
                         "FETCH FIRST 50 ROWS ONLY");  // Dernières 50 réceptions

    historyQuery.bindValue(":currentId", receptionId);
    historyQuery.exec();

    // 3. Calculer les rendements historiques (simulés à partir des données existantes)
    //    On suppose que le rendement réel peut être déduit du statut et de la qualité
    QList<float> rendementsHistoriques;
    QList<float> qualiteScores;
    QList<float> quantiteScores;

    QMap<QString, float> rendementParQualite;
    QMap<QString, int> countParQualite;

    while (historyQuery.next()) {
        float quantite = historyQuery.value(1).toFloat();
        QString qualite = historyQuery.value(2).toString().toUpper();
        QString status = historyQuery.value(3).toString().toUpper();
        QDate date = historyQuery.value(4).toDate();

        // Calculer un rendement simulé basé sur:
        // - Qualité (poids principal)
        // - Statut (REÇU = succès, meilleur rendement)
        // - Quantité (lots plus grands = meilleur rendement industriel)
        // - Saison (mois de récolte Oct-Nov = meilleur)

        float rendementSimule = 0;

        // Score de qualité (0-100)
        float qualiteScore = 0;
        if (qualite == "EXTRA") qualiteScore = 95;
        else if (qualite == "PREMIUM") qualiteScore = 88;
        else if (qualite == "A") qualiteScore = 80;
        else if (qualite == "B") qualiteScore = 70;
        else if (qualite == "C") qualiteScore = 60;
        else qualiteScore = 65;

        // Score de statut
        float statusBonus = (status == "REÇU") ? 1.05f : 0.95f;

        // Score de quantité (normalisé)
        float quantiteBonus = qMin(1.15f, 0.85f + (quantite / 2000.0f));

        // Score saisonnier
        int mois = date.month();
        float saisonBonus = (mois >= 9 && mois <= 11) ? 1.08f : 0.96f;

        // Calcul du rendement simulé (%)
        // Formule: (qualiteScore / 100) * 25 * statusBonus * quantiteBonus * saisonBonus
        rendementSimule = (qualiteScore / 100.0f) * 25.0f * statusBonus * quantiteBonus * saisonBonus;
        rendementSimule = qMin(28.0f, qMax(8.0f, rendementSimule));  // Bornes réalistes

        rendementsHistoriques.append(rendementSimule);
        qualiteScores.append(qualiteScore);
        quantiteScores.append(quantite);

        // Agréger par qualité
        if (!qualite.isEmpty()) {
            rendementParQualite[qualite] += rendementSimule;
            countParQualite[qualite]++;
        }
    }

    // 4. Calculer le rendement prédit pour la réception cible

    // Score de qualité cible
    float qualiteScoreCible = 0;
    if (result.qualiteOlives == "EXTRA") qualiteScoreCible = 95;
    else if (result.qualiteOlives == "PREMIUM") qualiteScoreCible = 88;
    else if (result.qualiteOlives == "A") qualiteScoreCible = 80;
    else if (result.qualiteOlives == "B") qualiteScoreCible = 70;
    else if (result.qualiteOlives == "C") qualiteScoreCible = 60;
    else qualiteScoreCible = 65;

    // Score de quantité cible
    float quantiteBonusCible = qMin(1.15f, 0.85f + (result.quantiteOlivesKg / 2000.0f));

    // Score saisonnier cible
    int moisCible = currentDate.month();
    float saisonBonusCible = (moisCible >= 9 && moisCible <= 11) ? 1.08f : 0.96f;

    // Méthode 1: Moyenne par qualité (si assez de données)
    float rendementParQualitePred = 0;
    if (countParQualite.value(result.qualiteOlives, 0) >= 3) {
        rendementParQualitePred = rendementParQualite[result.qualiteOlives] / countParQualite[result.qualiteOlives];
    }

    // Méthode 2: Régression linéaire simple (qualitéScore -> rendement)
    float rendementRegression = 0;
    if (rendementsHistoriques.size() >= 5) {
        // Calcul de la corrélation qualité-rendement
        float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        for (int i = 0; i < rendementsHistoriques.size() && i < qualiteScores.size(); i++) {
            sumX += qualiteScores[i];
            sumY += rendementsHistoriques[i];
            sumXY += qualiteScores[i] * rendementsHistoriques[i];
            sumX2 += qualiteScores[i] * qualiteScores[i];
        }
        int n = rendementsHistoriques.size();
        if (n * sumX2 - sumX * sumX != 0) {
            float pente = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
            float intercept = (sumY - pente * sumX) / n;
            rendementRegression = pente * qualiteScoreCible + intercept;
            rendementRegression = qMin(28.0f, qMax(8.0f, rendementRegression));
        }
    }

    // Méthode 3: Formule standard
    float rendementStandard = (qualiteScoreCible / 100.0f) * 22.0f * quantiteBonusCible * saisonBonusCible;
    rendementStandard = qMin(28.0f, qMax(8.0f, rendementStandard));

    // Fusion des prédictions (pondérée)
    float predictionFinale = rendementStandard;

    if (rendementParQualitePred > 0) {
        predictionFinale = predictionFinale * 0.6f + rendementParQualitePred * 0.4f;
    }

    if (rendementRegression > 0 && rendementsHistoriques.size() >= 10) {
        predictionFinale = predictionFinale * 0.7f + rendementRegression * 0.3f;
        result.methode = "Régression historique + moyenne qualité";
    }

    result.rendementPredicted = predictionFinale;

    // Calcul de la confiance (basée sur nombre d'échantillons et variance)
    float confianceBase = qMin(95.0f, 50.0f + rendementsHistoriques.size());
    if (countParQualite.value(result.qualiteOlives, 0) > 1) {
        // Calcul simplifié de variance
        float varianceEstimee = 10.0f;  // Approximation
        confianceBase -= varianceEstimee / 2;
    }
    result.confiance = qMin(98.0f, qMax(30.0f, confianceBase));

    // Calcul de l'huile produite
    float huileKg = result.quantiteOlivesKg * (result.rendementPredicted / 100.0f);
    result.huileProduiteLitres = huileKg / 0.916f;

    qDebug() << "Prédiction pour lot" << currentLot << ":"
             << result.rendementPredicted << "% rendement ->"
             << result.huileProduiteLitres << "L d'huile (confiance:" << result.confiance << "%)";

    return result;
}

Reception::RendementPrediction Reception::predireProductionHuileParLot(const QString& lotNumber)
{
    QSqlQuery query;
    query.prepare("SELECT id FROM reception WHERE lot_number = :lot FETCH FIRST 1 ROW ONLY");
    query.bindValue(":lot", lotNumber);

    if (query.exec() && query.next()) {
        return predireProductionHuile(query.value(0).toInt());
    }

    RendementPrediction empty;
    empty.quantiteOlivesKg = 0;
    return empty;
}

QSqlQueryModel* Reception::getHistoriqueRendement()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT "
                  "lot_number as \"Lot\", "
                  "TO_CHAR(received_at, 'DD/MM/YYYY') as \"Date\", "
                  "quantity_kg as \"Olives (kg)\", "
                  "quality_grade as \"Qualité\", "
                  "status as \"Statut\", "
                  "CASE quality_grade "
                  "  WHEN 'EXTRA' THEN 22 "
                  "  WHEN 'PREMIUM' THEN 20 "
                  "  WHEN 'A' THEN 18 "
                  "  WHEN 'B' THEN 15 "
                  "  WHEN 'C' THEN 12 "
                  "  ELSE 14 "
                  "END as \"Rendement estimé (%)\", "
                  "ROUND(quantity_kg * (CASE quality_grade "
                  "  WHEN 'EXTRA' THEN 0.22 "
                  "  WHEN 'PREMIUM' THEN 0.20 "
                  "  WHEN 'A' THEN 0.18 "
                  "  WHEN 'B' THEN 0.15 "
                  "  WHEN 'C' THEN 0.12 "
                  "  ELSE 0.14 END) / 0.916, 2) as \"Huile estimée (L)\" "
                  "FROM reception "
                  "ORDER BY received_at DESC");

    if (query.exec()) {
        model->setQuery(std::move(query));
    }

    return model;
}

/* ================= PRIORITÉ DE PRODUCTION FIFO ================= */

QList<Reception::PrioriteProduction> Reception::getPrioriteProductionFIFO()
{
    QList<PrioriteProduction> fileAttente;

    QSqlQuery query;
    // FIFO: tri par date la plus ancienne (ASC), seuls les statuts REÇU ou EN_ATTENTE
    query.prepare("SELECT id, lot_number, received_at, quantity_kg, quality_grade, status "
                  "FROM reception "
                  "WHERE UPPER(status) IN ('REÇU', 'EN_ATTENTE', 'EN_COURS') "
                  "ORDER BY received_at ASC");  // Plus ancien d'abord

    if (!query.exec()) {
        qDebug() << "Erreur FIFO:" << query.lastError();
        return fileAttente;
    }

    int rang = 1;
    while (query.next()) {
        PrioriteProduction p;
        p.id = query.value(0).toInt();
        p.lotNumber = query.value(1).toString();
        p.dateReception = query.value(2).toDate().toString("dd/MM/yyyy");
        p.quantiteKg = query.value(3).toFloat();
        p.qualite = query.value(4).toString();
        p.status = query.value(5).toString();
        p.ordreProduction = rang++;

        fileAttente.append(p);
    }

    qDebug() << "FIFO: " << fileAttente.size() << " lots en attente de production";
    return fileAttente;
}

QList<Reception::PrioriteProduction> Reception::getPrioriteProductionParQualiteEtDate()
{
    QList<PrioriteProduction> fileAttente;

    // Priorité: d'abord la qualité (EXTRA > PREMIUM > A > B > C), puis date ancienne
    QSqlQuery query;
    query.prepare("SELECT id, lot_number, received_at, quantity_kg, quality_grade, status, "
                  "CASE UPPER(quality_grade) "
                  "  WHEN 'EXTRA' THEN 1 "
                  "  WHEN 'PREMIUM' THEN 2 "
                  "  WHEN 'A' THEN 3 "
                  "  WHEN 'B' THEN 4 "
                  "  WHEN 'C' THEN 5 "
                  "  ELSE 6 "
                  "END as priorite_qualite "
                  "FROM reception "
                  "WHERE UPPER(status) IN ('REÇU', 'EN_ATTENTE', 'EN_COURS') "
                  "ORDER BY priorite_qualite ASC, received_at ASC");

    if (!query.exec()) {
        qDebug() << "Erreur priorité qualité:" << query.lastError();
        return fileAttente;
    }

    int rang = 1;
    while (query.next()) {
        PrioriteProduction p;
        p.id = query.value(0).toInt();
        p.lotNumber = query.value(1).toString();
        p.dateReception = query.value(2).toDate().toString("dd/MM/yyyy");
        p.quantiteKg = query.value(3).toFloat();
        p.qualite = query.value(4).toString();
        p.status = query.value(5).toString();
        p.ordreProduction = rang++;

        fileAttente.append(p);
    }

    return fileAttente;
}

QSqlQueryModel* Reception::getFileAttenteProductionModel()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT "
                  "ROW_NUMBER() OVER (ORDER BY received_at ASC) as \"Ordre\", "
                  "lot_number as \"Lot\", "
                  "TO_CHAR(received_at, 'DD/MM/YYYY') as \"Date réception\", "
                  "quantity_kg as \"Quantité (kg)\", "
                  "quality_grade as \"Qualité\", "
                  "status as \"Statut\", "
                  "CASE "
                  "  WHEN ROW_NUMBER() OVER (ORDER BY received_at ASC) = 1 "
                  "  THEN '🔴 À PRODUIRE MAINTENANT' "
                  "  WHEN ROW_NUMBER() OVER (ORDER BY received_at ASC) <= 3 "
                  "  THEN '🟠 Production imminente' "
                  "  ELSE '🟢 En attente' "
                  "END as \"Recommandation\" "
                  "FROM reception "
                  "WHERE UPPER(status) IN ('REÇU', 'EN_ATTENTE', 'EN_COURS') "
                  "ORDER BY received_at ASC");

    if (query.exec()) {
        model->setQuery(std::move(query));
    }

    return model;
}
