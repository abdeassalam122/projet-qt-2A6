#ifndef RECEPTION_H
#define RECEPTION_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

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

    // CRUD operations
    bool ajouter();
    static QSqlQueryModel* afficher();
    bool modifier();
    static bool supprimer(int id);

    // Helper methods
    static int getNextId(); // ADD THIS

    // Recherche
    static QSqlQueryModel* rechercher(const QString &lot);
    static Reception getById(int id);
};

#endif // RECEPTION_H
