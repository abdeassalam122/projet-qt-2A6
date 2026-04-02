#include "mainwindow.h"
#include "reception.h"
#include <QMessageBox>
#include <QFormLayout>
#include <QDate>
#include <QDateEdit>
#include <QTableWidgetItem>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>
#include <QHeaderView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Initialize member variables
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    // Create pages
    createReceptionPage();

    // Set default page
    stackedWidget->setCurrentWidget(receptionPage);

    // Set window title
    setWindowTitle("Gestion des Réceptions");

    // Set minimum size
    setMinimumSize(1000, 600);
}

MainWindow::~MainWindow()
{
}

// ============================================================================
// SETUP UI - Helper function to create header widget
// ============================================================================

QWidget* MainWindow::createHeaderWidget(const QString &title)
{
    QWidget *headerWidget = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    return headerWidget;
}

// ============================================================================
// PAGE RÉCEPTION - Interface uniquement (sans requêtes SQL)
// ============================================================================

void MainWindow::createReceptionPage()
{
    receptionPage = new QWidget();
    QVBoxLayout *mainLay = new QVBoxLayout(receptionPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    // Header
    mainLay->addWidget(createHeaderWidget("Gestion des Réceptions"));

    // ======= Controls row (search + buttons) =======
    QHBoxLayout *ctrl = new QHBoxLayout();

    searchBoxReception = new QLineEdit();
    searchBoxReception->setObjectName("searchBoxReception");
    searchBoxReception->setPlaceholderText("Rechercher par LOT (ex: LOT-001) ...");
    searchBoxReception->setFixedHeight(34);
    connect(searchBoxReception, &QLineEdit::textChanged, this, &MainWindow::searchReception);

    QPushButton *addBtn = new QPushButton("+ Ajouter");
    addBtn->setFixedSize(110,34);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::showAddReceptionDialog);

    QPushButton *editBtn = new QPushButton("✎ Modifier");
    editBtn->setFixedSize(110,34);
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::editSelectedReception);

    QPushButton *delBtn = new QPushButton("🗑 Supprimer");
    delBtn->setFixedSize(120,34);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedReception);

    QPushButton *refreshBtn = new QPushButton("🔄 Rafraîchir");
    refreshBtn->setFixedSize(110,34);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshReceptionData);

    ctrl->addWidget(searchBoxReception);
    ctrl->addWidget(addBtn);
    ctrl->addWidget(editBtn);
    ctrl->addWidget(delBtn);
    ctrl->addWidget(refreshBtn);
    ctrl->addStretch();
    mainLay->addLayout(ctrl);

    // ======= Table Réception =======
    receptionTable = new QTableWidget();
    receptionTable->setObjectName("receptionTable");
    receptionTable->setColumnCount(7);
    receptionTable->setHorizontalHeaderLabels({"ID", "Lot", "Date", "Quantité (kg)", "Qualité", "Client ID", "Statut"});

    receptionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    receptionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    receptionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    receptionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    receptionTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    receptionTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);

    receptionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    receptionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    receptionTable->verticalHeader()->setVisible(false);
    receptionTable->setSelectionMode(QAbstractItemView::SingleSelection);

    mainLay->addWidget(receptionTable);

    // Charger les données
    refreshReceptionData();

    // Add to stacked widget
    stackedWidget->addWidget(receptionPage);
}

// ============================================================================
// FONCTIONS D'INTERFACE RÉCEPTION (appellent la classe Reception)
// ============================================================================

void MainWindow::showAddReceptionDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Ajouter Réception");
    dlg.setMinimumSize(400, 300);

    QFormLayout *form = new QFormLayout();

    QLineEdit *lot = new QLineEdit();
    lot->setPlaceholderText("LOT-001");

    // Add validator for LOT format
    QRegularExpression rxLot("^LOT-[0-9]{3}$");
    QRegularExpressionValidator *lotValidator = new QRegularExpressionValidator(rxLot, lot);
    lot->setValidator(lotValidator);

    QDoubleSpinBox *qty = new QDoubleSpinBox();
    qty->setMaximum(99999);
    qty->setMinimum(0.01);
    qty->setDecimals(2);
    qty->setSuffix(" kg");

    QLineEdit *qual = new QLineEdit();
    qual->setPlaceholderText("Qualité");

    // Add validator for quality (letters and spaces only)
    QRegularExpression rxQual("^[A-Za-zÀ-ÿ ]+$");
    QRegularExpressionValidator *qualValidator = new QRegularExpressionValidator(rxQual, qual);
    qual->setValidator(qualValidator);

    QSpinBox *client = new QSpinBox();
    client->setRange(1, 9999);
    client->setValue(1);

    form->addRow("Lot:", lot);
    form->addRow("Quantité:", qty);
    form->addRow("Qualité:", qual);
    form->addRow("Client ID:", client);

    QVBoxLayout *v = new QVBoxLayout(&dlg);
    v->addLayout(form);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    v->addWidget(bb);

    if(dlg.exec() == QDialog::Accepted)
    {
        // Validation
        QRegularExpression rxLotCheck("^LOT-[0-9]{3}$");
        QRegularExpression rxQualCheck("^[A-Za-zÀ-ÿ ]+$");

        if(!rxLotCheck.match(lot->text()).hasMatch())
        {
            QMessageBox::critical(this, "Erreur", "Format LOT invalide (doit être LOT-001, LOT-002, etc.)");
            return;
        }

        if(qty->value() <= 0)
        {
            QMessageBox::critical(this, "Erreur", "Quantité invalide (doit être > 0)");
            return;
        }

        if(!rxQualCheck.match(qual->text()).hasMatch())
        {
            QMessageBox::critical(this, "Erreur", "Qualité invalide (lettres uniquement)");
            return;
        }

        if(client->value() <= 0)
        {
            QMessageBox::critical(this, "Erreur", "Client ID invalide");
            return;
        }

        // Appel à la classe Reception
        Reception r(0, lot->text(), qty->value(), qual->text(), client->value(), "REÇU");

        if(r.ajouter()) {
            QMessageBox::information(this, "Succès", "Réception ajoutée dans la base");
            refreshReceptionData();
            searchBoxReception->clear(); // Clear search after adding
        } else {
            QMessageBox::critical(this, "Erreur", "Erreur lors de l'ajout");
        }
    }
}

void MainWindow::editSelectedReception()
{
    int row = receptionTable->currentRow();

    if(row < 0)
    {
        QMessageBox::warning(this, "Attention", "Sélectionnez une réception à modifier");
        return;
    }

    int id = receptionTable->item(row, 0)->text().toInt();
    QString currentLot = receptionTable->item(row, 1)->text();
    double currentQty = receptionTable->item(row, 3)->text().toDouble();
    QString currentQual = receptionTable->item(row, 4)->text();

    QDialog dlg(this);
    dlg.setWindowTitle("Modifier Réception");
    dlg.setMinimumSize(400, 250);

    QFormLayout *form = new QFormLayout();

    QLineEdit *lot = new QLineEdit(currentLot);
    // Add validator for LOT format
    QRegularExpression rxLot("^LOT-[0-9]{3}$");
    QRegularExpressionValidator *lotValidator = new QRegularExpressionValidator(rxLot, lot);
    lot->setValidator(lotValidator);

    QDoubleSpinBox *qty = new QDoubleSpinBox();
    qty->setMaximum(99999);
    qty->setMinimum(0.01);
    qty->setDecimals(2);
    qty->setSuffix(" kg");
    qty->setValue(currentQty);

    QLineEdit *qual = new QLineEdit(currentQual);
    // Add validator for quality
    QRegularExpression rxQual("^[A-Za-zÀ-ÿ ]+$");
    QRegularExpressionValidator *qualValidator = new QRegularExpressionValidator(rxQual, qual);
    qual->setValidator(qualValidator);

    form->addRow("Lot:", lot);
    form->addRow("Quantité:", qty);
    form->addRow("Qualité:", qual);

    QVBoxLayout *v = new QVBoxLayout(&dlg);
    v->addLayout(form);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    v->addWidget(bb);

    if(dlg.exec() == QDialog::Accepted)
    {
        QRegularExpression rxLotCheck("^LOT-[0-9]{3}$");
        QRegularExpression rxQualCheck("^[A-Za-zÀ-ÿ ]+$");

        if(!rxLotCheck.match(lot->text()).hasMatch())
        {
            QMessageBox::critical(this, "Erreur", "Format LOT invalide (doit être LOT-001, LOT-002, etc.)");
            return;
        }

        if(qty->value() <= 0)
        {
            QMessageBox::critical(this, "Erreur", "Veuillez entrer une quantité valide (> 0)");
            return;
        }

        if(!rxQualCheck.match(qual->text()).hasMatch())
        {
            QMessageBox::critical(this, "Erreur", "Veuillez entrer une qualité valide (lettres uniquement)");
            return;
        }

        // Get current client_id and status from table
        int currentClientId = receptionTable->item(row, 5)->text().toInt();
        QString currentStatus = receptionTable->item(row, 6)->text();

        // Appel à la classe Reception
        Reception r(id, lot->text(), qty->value(), qual->text(), currentClientId, currentStatus);

        if(r.modifier()) {
            QMessageBox::information(this, "Succès", "Réception modifiée avec succès");
            refreshReceptionData();
        } else {
            QMessageBox::critical(this, "Erreur", "Erreur lors de la modification");
        }
    }
}

void MainWindow::deleteSelectedReception()
{
    int row = receptionTable->currentRow();

    if(row < 0)
    {
        QMessageBox::warning(this, "Attention", "Sélectionnez une réception à supprimer");
        return;
    }

    int id = receptionTable->item(row, 0)->text().toInt();
    QString lot = receptionTable->item(row, 1)->text();

    QMessageBox::StandardButton rep = QMessageBox::question(
        this,
        "Confirmation de suppression",
        QString("Êtes-vous sûr de vouloir supprimer la réception %1 (ID: %2) ?").arg(lot).arg(id),
        QMessageBox::Yes | QMessageBox::No
        );

    if(rep == QMessageBox::Yes)
    {
        // Appel à la classe Reception
        if(Reception::supprimer(id)) {
            QMessageBox::information(this, "Succès", "Réception supprimée avec succès");
            refreshReceptionData();
        } else {
            QMessageBox::critical(this, "Erreur", "Erreur lors de la suppression");
        }
    }
}

void MainWindow::searchReception(const QString &text)
{
    // Utilisation de la classe Reception pour la recherche
    QSqlQueryModel *model = Reception::rechercher(text);

    // Mise à jour du tableau avec les résultats
    receptionTable->setRowCount(0);

    for(int i = 0; i < model->rowCount(); i++)
    {
        receptionTable->insertRow(i);
        for(int j = 0; j < 7; j++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(model->data(model->index(i, j)).toString());
            receptionTable->setItem(i, j, item);
        }
    }

    qDebug() << "Recherche réceptions:" << model->rowCount() << "résultat(s) trouvé(s)";
    delete model;
}

void MainWindow::refreshReceptionData()
{
    // Utilisation de la classe Reception pour l'affichage
    QSqlQueryModel *model = Reception::afficher();

    // Mise à jour du tableau
    receptionTable->setRowCount(0);

    for(int i = 0; i < model->rowCount(); i++)
    {
        receptionTable->insertRow(i);
        for(int j = 0; j < 7; j++)
        {
            QTableWidgetItem *item = new QTableWidgetItem(model->data(model->index(i, j)).toString());
            receptionTable->setItem(i, j, item);
        }
    }

    qDebug() << "Réceptions chargées depuis la base ✅ -" << model->rowCount() << "lignes";
    delete model;
}
