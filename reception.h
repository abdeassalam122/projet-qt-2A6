#ifndef RECEPTION_H
#define RECEPTION_H

#include <QString>
#include <QSqlQueryModel>
#include <QMap>
#include <QList>
#include <QDate>

class Reception
{
private:
    int id;
    QString lot_number;
    float quantity_kg;
    QString quality_grade;
    int client_id;
    QString status;

public:
    // Constructeurs
    Reception();
    Reception(int id, QString lot_number, float quantity_kg,
              QString quality_grade, int client_id, QString status);

    // Getters
    int getId() const;
    QString getLotNumber() const;
    float getQuantityKg() const;
    QString getQualityGrade() const;
    int getClientId() const;
    QString getStatus() const;

    // Setters
    void setId(int id);
    void setLotNumber(QString lot_number);
    void setQuantityKg(float quantity_kg);
    void setQualityGrade(QString quality_grade);
    void setClientId(int client_id);
    void setStatus(QString status);

    // CRUD
    bool ajouter();
    QSqlQueryModel* afficher();
    bool supprimer(int id);
    bool modifier();
    int getNextId();
    Reception getById(int id);

    // Recherche
    QSqlQueryModel* rechercher(const QString &lot);
    QSqlQueryModel* rechercherAvecFiltres(const QString& rechercheLot,
                                          const QString& statut,
                                          const QString& dateDebut,
                                          const QString& dateFin);

    // Filtrage
    QSqlQueryModel* filtrerParStatutEtDate(const QString& statut,
                                           const QString& dateDebut,
                                           const QString& dateFin);

    // Tri
    QSqlQueryModel* trierPar(const QString& colonne, Qt::SortOrder ordre);

    // Filtrage simple
    QSqlQueryModel* filtrerParStatut(const QString& statut);
    QSqlQueryModel* filtrerParDate(const QString& dateDebut, const QString& dateFin);

    // Structure pour les statistiques de performance
    struct PerformanceStats {
        int totalReceptions;
        float totalQuantiteKg;
        float moyenneQuantiteParReception;
        int dureeMoyenneTraitement;
        QString meilleurMois;
        QString meilleurStatut;
        float tauxReussite;
    };

    // Méthodes de statistiques
    PerformanceStats getPerformanceStats();
    QSqlQueryModel* getPerformanceParMois();
    QSqlQueryModel* getPerformanceParClient();
    QSqlQueryModel* getPerformanceParQualite();
    QSqlQueryModel* getStatistiquesParStatutModel();
    float getTendanceMensuelle(int mois, int annee);
    QMap<QString, float> getTopClients(int limite = 5);
    QSqlQueryModel* getDelaisTraitement();

    // Statistiques simples
    QMap<QString, int> statistiquesParStatut();
    QMap<QString, float> statistiquesQuantiteParStatut();

    // Méthodes PDF
    bool exporterPDF(const QString& cheminFichier);
    bool exporterStatistiquesPDF(const QString& cheminFichier);
    bool exporterReceptionsPDF(const QString& cheminFichier, QSqlQueryModel* model = nullptr);

    // ==================== PRÉDICTION PRODUCTION HUILE (ML simple) ====================
    struct RendementPrediction {
        float quantiteOlivesKg;      // Quantité d'olives en kg
        float rendementPredicted;    // Rendement prédit (%)
        float huileProduiteLitres;   // Huile produite en litres
        float confiance;             // Niveau de confiance (0-100)
        QString methode;             // Méthode utilisée
        QString qualiteOlives;       // Qualité des olives
    };

    RendementPrediction predireProductionHuile(int receptionId);
    RendementPrediction predireProductionHuileParLot(const QString& lotNumber);
    QSqlQueryModel* getHistoriqueRendement();  // Pour visualiser l'historique

    // ==================== PRIORITÉ DE PRODUCTION FIFO ====================
    struct PrioriteProduction {
        int id;
        QString lotNumber;
        QString dateReception;
        float quantiteKg;
        QString qualite;
        QString status;
        int ordreProduction;      // 1 = à produire en premier
    };

    QList<PrioriteProduction> getPrioriteProductionFIFO();        // Tri par date la plus ancienne
    QList<PrioriteProduction> getPrioriteProductionParQualiteEtDate();  // Tri par qualité puis date
    QSqlQueryModel* getFileAttenteProductionModel();              // Modèle pour affichage

private:
    // Méthodes privées pour le PDF
    QString getHTMLHeader();
    QString getHTMLFooter();
};

#endif // RECEPTION_H
