#ifndef CITERNE_H
#define CITERNE_H

#include <QDate>
#include <QList>
#include <QString>
#include <QtSql/QSqlDatabase>

class Citerne
{
public:
    Citerne();
    Citerne(int id,
            double capaciteL,
            double volumeL,
            const QString &qualite,
            double temperatureC,
            const QDate &dernierRemplissage);

    int id() const;
    double capaciteL() const;
    double volumeL() const;
    QString qualite() const;
    double temperatureC() const;
    QDate dernierRemplissage() const;

    void setId(int id);
    void setCapaciteL(double value);
    void setVolumeL(double value);
    void setQualite(const QString &value);
    void setTemperatureC(double value);
    void setDernierRemplissage(const QDate &value);

    bool ajouter(QSqlDatabase &db, QString *errorMessage = nullptr) const;
    bool modifier(QSqlDatabase &db, QString *errorMessage = nullptr) const;

    static bool supprimer(QSqlDatabase &db, int id, QString *errorMessage = nullptr);
    static bool mettreAJourVolume(QSqlDatabase &db, int id, double nouveauVolume, QString *errorMessage = nullptr);
    static QList<Citerne> afficher(QSqlDatabase &db, QString *errorMessage = nullptr);

    // ✅ NOUVEAU
    static bool updateTemperature(QSqlDatabase &db, int id, double delta);

private:
    int m_id;
    double m_capaciteL;
    double m_volumeL;
    QString m_qualite;
    double m_temperatureC;
    QDate m_dernierRemplissage;
};

#endif
