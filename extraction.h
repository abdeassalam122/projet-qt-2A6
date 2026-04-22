#ifndef EXTRACTION_H
#define EXTRACTION_H

#include <QDate>
#include <QList>
#include <QString>
#include <QtSql/QSqlDatabase>

class Extraction
{
public:
    Extraction();
    Extraction(int id,
               int lotId,
               int machineId,
               int targetCiterneId,
               const QDate &extractedAt,
               double inputQuantityKg,
               double outputOilL,
               const QString &status);

    int id() const;
    int lotId() const;
    int machineId() const;
    int targetCiterneId() const;
    QDate extractedAt() const;
    double inputQuantityKg() const;
    double outputOilL() const;
    QString status() const;

    void setId(int value);
    void setLotId(int value);
    void setMachineId(int value);
    void setTargetCiterneId(int value);
    void setExtractedAt(const QDate &value);
    void setInputQuantityKg(double value);
    void setOutputOilL(double value);
    void setStatus(const QString &value);

    bool ajouter(QSqlDatabase &db, QString *errorMessage = nullptr) const;
    bool modifier(QSqlDatabase &db, QString *errorMessage = nullptr) const;

    static bool supprimer(QSqlDatabase &db, int id, QString *errorMessage = nullptr);
    static QList<Extraction> afficher(QSqlDatabase &db, QString *errorMessage = nullptr);
    static QList<Extraction> rechercher(QSqlDatabase &db, const QString &term, QString *errorMessage = nullptr);
    static int getNextId(QSqlDatabase &db, QString *errorMessage = nullptr);

private:
    int m_id;
    int m_lotId;
    int m_machineId;
    int m_targetCiterneId;
    QDate m_extractedAt;
    double m_inputQuantityKg;
    double m_outputOilL;
    QString m_status;
};

#endif // EXTRACTION_H
