#include "mainwindow.h"
#include "reception.h"
#include "extraction.h"
#include "connection.h"
#include "client.h"
#include "citerne.h"

#include <QMessageBox>
#include <QFormLayout>
#include <QDate>
#include <QDateEdit>
#include <QTableWidgetItem>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QGridLayout>
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QGroupBox>
#include <QScrollArea>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>
#include <QHeaderView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSqlError>
#include <QSqlRecord>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDoubleValidator>
#include <QMap>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QPdfWriter>
#include <QTextDocument>
#include <QComboBox>
#include <algorithm>
#include <numeric>

namespace {

QPixmap buildDonutChartPixmap(const QSize &size,
                              const QList<int> &values,
                              const QList<QColor> &colors,
                              const QString &centerValue,
                              const QString &centerLabel)
{
    QPixmap px(size);
    px.fill(Qt::transparent);

    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int side = qMin(size.width(), size.height());
    const QRectF rect((size.width() - side) / 2.0 + 8.0,
                      (size.height() - side) / 2.0 + 8.0,
                      side - 16.0,
                      side - 16.0);

    const int total = std::accumulate(values.begin(), values.end(), 0);
    const qreal outerRadius = rect.width() * 0.5;
    const qreal innerRadius = outerRadius * 0.57;
    const QPointF c = rect.center();
    QRectF hole(c.x() - innerRadius, c.y() - innerRadius, innerRadius * 2.0, innerRadius * 2.0);

    // Background donut base ring.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#E6ECE9"));
    p.drawEllipse(rect);

    if (total <= 0) {
        p.setBrush(QColor("#F7F9F8"));
        p.drawEllipse(hole);
        return px;
    }

    int startAngle = 90 * 16;
    for (int i = 0; i < values.size() && i < colors.size(); ++i) {
        if (values[i] <= 0) {
            continue;
        }
        const qreal ratio = qreal(values[i]) / qreal(total);
        int span = qRound(ratio * 360.0 * 16.0);
        if (span <= 0) {
            continue;
        }

        // Keep small spacing between slices.
        const int gap = 2 * 16;
        const int drawSpan = qMax(0, span - gap);
        p.setBrush(colors[i]);
        p.drawPie(rect, startAngle, -drawSpan);
        startAngle -= span;
    }

    p.setBrush(QColor("#F7F9F8"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(hole);

    QFont valueFont("Segoe UI", 24, QFont::Black);
    QFont labelFont("Segoe UI", 12, QFont::DemiBold);
    p.setPen(QColor("#0A2E2A"));
    p.setFont(valueFont);
    p.drawText(hole.adjusted(0, -12, 0, -4), Qt::AlignCenter, centerValue);
    p.setPen(QColor("#2F4E49"));
    p.setFont(labelFont);
    p.drawText(hole.adjusted(0, 18, 0, 8), Qt::AlignCenter, centerLabel);

    return px;
}

QString formatLegendLine(const QString &name, int value, int total)
{
    if (total <= 0) {
        return QString("%1: 0 (0%)").arg(name);
    }
    const int pct = qRound((double(value) * 100.0) / double(total));
    return QString("%1: %2 (%3%)").arg(name).arg(value).arg(pct);
}

} // namespace

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
    setupUI();
    applyStyles();
    oracleActive = promptAndTestOracleConnection();
    if (oracleActive) {
        oracleActive = setupOracleSchema();
    }
    updateDatabaseStatusLabel();

    if (oracleActive) {
        loadClientsFromOracle();
        loadCiternesFromOracle();
    } else {
        populateClientsSampleData();
        populateCiternesSampleData();
    }
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

// ============================================================================
// RECEPTION PAGE - Interface complète comme les autres pages
// ============================================================================

void MainWindow::createReceptionPage()
{
    receptionPage = new QWidget();
    receptionPage->setObjectName("receptionPage");
    QVBoxLayout *mainLay = new QVBoxLayout(receptionPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    // Header
    mainLay->addWidget(createHeaderWidget("Gestion des Réceptions"));

    // Content
    QWidget *content = new QWidget();
    content->setObjectName("moduleCard");
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(8,8,8,8);
    contentLay->setSpacing(10);

    // Controls row (search)
    QHBoxLayout *ctrl = new QHBoxLayout();

    searchBoxReception = new QLineEdit();
    searchBoxReception->setObjectName("searchBoxReception");
    searchBoxReception->setPlaceholderText("Rechercher par LOT...");
    searchBoxReception->setFixedHeight(34);
    connect(searchBoxReception, &QLineEdit::textChanged, this, &MainWindow::searchReception);

    ctrl->addWidget(searchBoxReception);
    ctrl->addStretch();
    contentLay->addLayout(ctrl);

    // Table Réception
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
    receptionTable->verticalHeader()->setDefaultSectionSize(44);
    receptionTable->verticalHeader()->setMinimumSectionSize(40);
    receptionTable->setAlternatingRowColors(true);
    receptionTable->setShowGrid(false);

    contentLay->addWidget(receptionTable);

    // Actions row (like Clients/Citernes)
    QHBoxLayout *actions = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton("+ Ajouter");
    addBtn->setProperty("role", "primary");
    addBtn->setFixedSize(120,36);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::showAddReceptionDialog);

    QPushButton *editBtn = new QPushButton("✎ Éditer");
    editBtn->setProperty("role", "secondary");
    editBtn->setFixedSize(100,36);
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::editSelectedReception);

    QPushButton *delBtn = new QPushButton("🗑 Supprimer");
    delBtn->setProperty("role", "danger");
    delBtn->setFixedSize(120,36);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedReception);

    QPushButton *refreshBtn = new QPushButton("🔄 Rafraîchir");
    refreshBtn->setProperty("role", "secondary");
    refreshBtn->setFixedSize(130,36);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshReceptionData);

    actions->addWidget(addBtn);
    actions->addWidget(editBtn);
    actions->addWidget(delBtn);
    actions->addWidget(refreshBtn);
    actions->addStretch();
    contentLay->addLayout(actions);

    mainLay->addWidget(content);

    // Charger les données
    refreshReceptionData();

    stackedWidget->addWidget(receptionPage);
}

// ============================================================================
// SETUP UI - central widget with sidebar + stacked pages
// ============================================================================
void MainWindow::setupUI()
{
    setWindowTitle("Administration - Multi Modules");
    setMinimumSize(1200, 720);

    central = new QWidget(this);
    QHBoxLayout *hroot = new QHBoxLayout(central);
    hroot->setContentsMargins(0,0,0,0);
    hroot->setSpacing(0);

    // Sidebar on the left
    QWidget *side = createSideBar();
    hroot->addWidget(side, 0);

    // Content area on the right (tabs + stacked pages)
    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(16,16,16,16);
    contentLayout->setSpacing(8);

    moduleTabs = new QTabBar();
    moduleTabs->setObjectName("moduleTabs");
    moduleTabs->setExpanding(false);
    moduleTabs->setMovable(false);
    moduleTabs->setDrawBase(false);
    moduleTabs->addTab("Clients");
    moduleTabs->addTab("Citernes");
    moduleTabs->addTab("Réception");
    moduleTabs->addTab("Extraction");
    contentLayout->addWidget(moduleTabs, 0);

    stackedWidget = new QStackedWidget();
    contentLayout->addWidget(stackedWidget, 1);

    hroot->addWidget(content, 1);

    setCentralWidget(central);

    // Create pages and add to stack
    createClientsPage();
    createCiternesPage();
    createReceptionPage();
    createExtractionPage();
    createStatisticsPage();
    createCiterneStatisticsPage();

    // Default page = clients
    switchToClients();

    if (moduleTabs) {
        connect(moduleTabs, &QTabBar::currentChanged, stackedWidget, &QStackedWidget::setCurrentIndex);
        connect(stackedWidget, &QStackedWidget::currentChanged, moduleTabs, &QTabBar::setCurrentIndex);
    }
}

// ============================================================================
// SIDEBAR - vertical buttons for modules
// ============================================================================
QWidget* MainWindow::createSideBar()
{
    QWidget *side = new QWidget();
    side->setObjectName("sideBar");
    side->setFixedWidth(220);

    QVBoxLayout *v = new QVBoxLayout(side);
    v->setContentsMargins(16,16,16,16);
    v->setSpacing(10);

    QLabel *brand = new QLabel("Huilerie");
    brand->setObjectName("sideBrand");
    QLabel *subtitle = new QLabel("Administration");
    subtitle->setObjectName("sideSubtitle");

    v->addWidget(brand);
    v->addWidget(subtitle);
    v->addSpacing(12);

    navGroup = new QButtonGroup(this);
    navGroup->setExclusive(true);

    btnClients = new QPushButton("Clients");
    btnCiternes = new QPushButton("Citernes");
    btnReception = new QPushButton("Réception");
    btnExtraction = new QPushButton("Extraction");

    QList<QPushButton*> buttons = {btnClients, btnCiternes, btnReception, btnExtraction};
    for (QPushButton *b : buttons) {
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(42);
        b->setProperty("nav", true);
        navGroup->addButton(b);
        v->addWidget(b);
    }

    connect(btnClients, &QPushButton::clicked, this, &MainWindow::switchToClients);
    connect(btnCiternes, &QPushButton::clicked, this, &MainWindow::switchToCiternes);
    connect(btnReception, &QPushButton::clicked, this, &MainWindow::switchToReception);
    connect(btnExtraction, &QPushButton::clicked, this, &MainWindow::switchToExtraction);

    dbStatusLabel = new QLabel("Base Oracle: non connectée");
    dbStatusLabel->setObjectName("dbStatusLabel");
    dbStatusLabel->setWordWrap(true);
    dbStatusLabel->setStyleSheet("padding:8px; border-radius:8px;");

    v->addStretch();
    v->addWidget(dbStatusLabel);
    return side;
}

// ============================================================================
// NAV SLOTS
// ============================================================================
void MainWindow::switchToClients()
{
    // index 0 = clients page (we added pages in creation order)
    stackedWidget->setCurrentWidget(clientsPage);
    if (btnClients) btnClients->setChecked(true);
    if (moduleTabs) moduleTabs->setCurrentIndex(0);
}

void MainWindow::switchToCiternes()
{
    stackedWidget->setCurrentWidget(citernesPage);
    if (btnCiternes) btnCiternes->setChecked(true);
    if (moduleTabs) moduleTabs->setCurrentIndex(1);
}

void MainWindow::switchToReception()
{
    stackedWidget->setCurrentWidget(receptionPage);
    if (btnReception) btnReception->setChecked(true);
    if (moduleTabs) moduleTabs->setCurrentIndex(2);
}

void MainWindow::switchToExtraction()
{
    stackedWidget->setCurrentWidget(extractionPage);
    if (btnExtraction) btnExtraction->setChecked(true);
    if (moduleTabs) moduleTabs->setCurrentIndex(3);
}

// ============================================================================
// CLIENTS PAGE - full clients UI (table + controls)
// ============================================================================
void MainWindow::createClientsPage()
{
    clientsPage = new QWidget();
    clientsPage->setObjectName("clientsPage");
    QVBoxLayout *mainLay = new QVBoxLayout(clientsPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    // Header
    mainLay->addWidget(createHeaderWidget("Gestion des Clients"));

    // Controls + table
    QWidget *content = new QWidget();
    content->setObjectName("moduleCard");
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(8,8,8,8);
    contentLay->setSpacing(10);

    // Controls row
    QHBoxLayout *ctrl = new QHBoxLayout();
    searchBox = new QLineEdit();
    searchBox->setObjectName("searchBox");
    searchBox->setPlaceholderText("Rechercher (nom, email, téléphone)...");
    searchBox->setFixedHeight(34);
    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::searchClients);

    sortButton = new QPushButton("Trier");
    sortButton->setProperty("role", "secondary");
    sortButton->setFixedSize(80,34);
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::sortClients);

    exportButton = new QPushButton("Exporter");
    exportButton->setProperty("role", "secondary");
    exportButton->setFixedSize(100,34);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportClients);

    statsButton = new QPushButton("Statistiques");
    statsButton->setProperty("role", "accent");
    statsButton->setFixedSize(110,34);
    connect(statsButton, &QPushButton::clicked, this, &MainWindow::showStatisticsView);

    QPushButton *compareButton = new QPushButton("Comparatif");
    compareButton->setProperty("role", "secondary");
    compareButton->setFixedSize(110,34);
    connect(compareButton, &QPushButton::clicked, this, &MainWindow::showClientComparisonDashboard);

    QPushButton *rfmButton = new QPushButton("Matrice RFM");
    rfmButton->setProperty("role", "secondary");
    rfmButton->setFixedSize(120,34);
    connect(rfmButton, &QPushButton::clicked, this, &MainWindow::showClientRfmMatrix);

    ctrl->addWidget(searchBox);
    ctrl->addWidget(sortButton);
    ctrl->addWidget(exportButton);
    ctrl->addWidget(statsButton);
    ctrl->addWidget(compareButton);
    ctrl->addWidget(rfmButton);
    ctrl->addStretch();

    contentLay->addLayout(ctrl);

    // Table
    clientsTable = new QTableWidget();
    clientsTable->setColumnCount(8);
    clientsTable->setHorizontalHeaderLabels({"ID","Nom","Email","Téléphone","Adresse","Inscrit le","Statut","Actions"});
    clientsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    clientsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    clientsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    clientsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    clientsTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    clientsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    clientsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    clientsTable->verticalHeader()->setVisible(false);
    clientsTable->verticalHeader()->setDefaultSectionSize(44);
    clientsTable->verticalHeader()->setMinimumSectionSize(40);
    clientsTable->setAlternatingRowColors(true);
    clientsTable->setShowGrid(false);
    clientsTable->setObjectName("clientsTable");
    contentLay->addWidget(clientsTable);

    // Action buttons
    QHBoxLayout *actions = new QHBoxLayout();
    addButton = new QPushButton("+ Ajouter");
    addButton->setProperty("role", "primary");
    addButton->setFixedSize(120,36);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::showAddClientDialog);

    editButton = new QPushButton("✎ Éditer");
    editButton->setProperty("role", "secondary");
    editButton->setFixedSize(100,36);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedClient);

    viewButton = new QPushButton("👁 Voir");
    viewButton->setProperty("role", "secondary");
    viewButton->setFixedSize(100,36);
    connect(viewButton, &QPushButton::clicked, this, &MainWindow::viewSelectedClient);

    deleteButton = new QPushButton("🗑 Supprimer");
    deleteButton->setProperty("role", "danger");
    deleteButton->setFixedSize(120,36);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedClient);

    actions->addWidget(addButton);
    actions->addWidget(editButton);
    actions->addWidget(viewButton);
    actions->addWidget(deleteButton);
    actions->addStretch();

    contentLay->addLayout(actions);

    mainLay->addWidget(content);

    stackedWidget->addWidget(clientsPage);
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

void MainWindow::showAddExtractionDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Ajouter Extraction");
    dlg.setMinimumSize(430, 320);

    QFormLayout *form = new QFormLayout();

    QSpinBox *lotId = new QSpinBox();
    lotId->setRange(1, 999999);

    QSpinBox *machineId = new QSpinBox();
    machineId->setRange(1, 999999);

    QSpinBox *targetCiterneId = new QSpinBox();
    targetCiterneId->setRange(1, 999999);

    QDateEdit *extractedAt = new QDateEdit(QDate::currentDate());
    extractedAt->setCalendarPopup(true);
    extractedAt->setDisplayFormat("dd/MM/yyyy");

    QDoubleSpinBox *inputKg = new QDoubleSpinBox();
    inputKg->setRange(0.01, 999999.0);
    inputKg->setDecimals(2);
    inputKg->setSuffix(" kg");

    QDoubleSpinBox *outputOil = new QDoubleSpinBox();
    outputOil->setRange(0.0, 999999.0);
    outputOil->setDecimals(2);
    outputOil->setSuffix(" L");

    QLineEdit *status = new QLineEdit("PLANIFIE");

    form->addRow("Lot ID:", lotId);
    form->addRow("Machine ID:", machineId);
    form->addRow("Citerne cible ID:", targetCiterneId);
    form->addRow("Date extraction:", extractedAt);
    form->addRow("Entrée:", inputKg);
    form->addRow("Huile sortie:", outputOil);
    form->addRow("Statut:", status);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    if (inputKg->value() <= 0.0) {
        QMessageBox::warning(this, "Validation", "La quantité d'entrée doit être > 0.");
        return;
    }

    Extraction extraction(0,
                          lotId->value(),
                          machineId->value(),
                          targetCiterneId->value(),
                          extractedAt->date(),
                          inputKg->value(),
                          outputOil->value(),
                          status->text().trimmed().isEmpty() ? QStringLiteral("PLANIFIE") : status->text().trimmed());

    QString error;
    if (!extraction.ajouter(db, &error)) {
        QMessageBox::critical(this, "Erreur", "Ajout extraction impossible:\n" + error);
        return;
    }

    QMessageBox::information(this, "Succès", "Extraction ajoutée avec succès.");
    refreshExtractionData();
    if (searchBoxExtraction) {
        searchBoxExtraction->clear();
    }
}

void MainWindow::editSelectedExtraction()
{
    const int row = extractionTable ? extractionTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::warning(this, "Attention", "Sélectionnez une extraction à modifier.");
        return;
    }

    const int id = extractionTable->item(row, 0)->text().toInt();

    QDialog dlg(this);
    dlg.setWindowTitle("Modifier Extraction");
    dlg.setMinimumSize(430, 320);

    QFormLayout *form = new QFormLayout();

    QSpinBox *lotId = new QSpinBox();
    lotId->setRange(1, 999999);
    lotId->setValue(extractionTable->item(row, 1)->text().toInt());

    QSpinBox *machineId = new QSpinBox();
    machineId->setRange(1, 999999);
    machineId->setValue(extractionTable->item(row, 2)->text().toInt());

    QSpinBox *targetCiterneId = new QSpinBox();
    targetCiterneId->setRange(1, 999999);
    targetCiterneId->setValue(extractionTable->item(row, 3)->text().toInt());

    QDateEdit *extractedAt = new QDateEdit();
    extractedAt->setCalendarPopup(true);
    extractedAt->setDisplayFormat("dd/MM/yyyy");
    const QDate tableDate = QDate::fromString(extractionTable->item(row, 4)->text(), "dd/MM/yyyy");
    extractedAt->setDate(tableDate.isValid() ? tableDate : QDate::currentDate());

    QDoubleSpinBox *inputKg = new QDoubleSpinBox();
    inputKg->setRange(0.01, 999999.0);
    inputKg->setDecimals(2);
    inputKg->setSuffix(" kg");
    inputKg->setValue(extractionTable->item(row, 5)->text().toDouble());

    QDoubleSpinBox *outputOil = new QDoubleSpinBox();
    outputOil->setRange(0.0, 999999.0);
    outputOil->setDecimals(2);
    outputOil->setSuffix(" L");
    outputOil->setValue(extractionTable->item(row, 6)->text().toDouble());

    QLineEdit *status = new QLineEdit(extractionTable->item(row, 7)->text());

    form->addRow("Lot ID:", lotId);
    form->addRow("Machine ID:", machineId);
    form->addRow("Citerne cible ID:", targetCiterneId);
    form->addRow("Date extraction:", extractedAt);
    form->addRow("Entrée:", inputKg);
    form->addRow("Huile sortie:", outputOil);
    form->addRow("Statut:", status);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addLayout(form);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(bb);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    Extraction extraction(id,
                          lotId->value(),
                          machineId->value(),
                          targetCiterneId->value(),
                          extractedAt->date(),
                          inputKg->value(),
                          outputOil->value(),
                          status->text().trimmed().isEmpty() ? QStringLiteral("PLANIFIE") : status->text().trimmed());

    QString error;
    if (!extraction.modifier(db, &error)) {
        QMessageBox::critical(this, "Erreur", "Modification extraction impossible:\n" + error);
        return;
    }

    QMessageBox::information(this, "Succès", "Extraction modifiée avec succès.");
    refreshExtractionData();
}

void MainWindow::deleteSelectedExtraction()
{
    const int row = extractionTable ? extractionTable->currentRow() : -1;
    if (row < 0) {
        QMessageBox::warning(this, "Attention", "Sélectionnez une extraction à supprimer.");
        return;
    }

    const int id = extractionTable->item(row, 0)->text().toInt();

    if (QMessageBox::question(this,
                              "Confirmation",
                              QString("Supprimer l'extraction ID %1 ?").arg(id),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!Extraction::supprimer(db, id, &error)) {
        QMessageBox::critical(this, "Erreur", "Suppression extraction impossible:\n" + error);
        return;
    }

    QMessageBox::information(this, "Succès", "Extraction supprimée avec succès.");
    refreshExtractionData();
}

void MainWindow::searchExtraction(const QString &text)
{
    QString error;
    QList<Extraction> rows;

    if (text.trimmed().isEmpty()) {
        rows = Extraction::afficher(db, &error);
    } else {
        rows = Extraction::rechercher(db, text.trimmed(), &error);
    }

    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Erreur", "Recherche extraction impossible:\n" + error);
        return;
    }

    extractionTable->setRowCount(0);
    int rowIndex = 0;
    for (const Extraction &e : rows) {
        extractionTable->insertRow(rowIndex);
        extractionTable->setItem(rowIndex, 0, new QTableWidgetItem(QString::number(e.id())));
        extractionTable->setItem(rowIndex, 1, new QTableWidgetItem(QString::number(e.lotId())));
        extractionTable->setItem(rowIndex, 2, new QTableWidgetItem(QString::number(e.machineId())));
        extractionTable->setItem(rowIndex, 3, new QTableWidgetItem(QString::number(e.targetCiterneId())));
        extractionTable->setItem(rowIndex, 4, new QTableWidgetItem(e.extractedAt().toString("dd/MM/yyyy")));
        extractionTable->setItem(rowIndex, 5, new QTableWidgetItem(QString::number(e.inputQuantityKg(), 'f', 2)));
        extractionTable->setItem(rowIndex, 6, new QTableWidgetItem(QString::number(e.outputOilL(), 'f', 2)));
        extractionTable->setItem(rowIndex, 7, new QTableWidgetItem(e.status()));
        ++rowIndex;
    }
}

void MainWindow::refreshExtractionData()
{
    QString error;
    const QList<Extraction> rows = Extraction::afficher(db, &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(this, "Erreur", "Chargement extractions impossible:\n" + error);
        return;
    }

    extractionTable->setRowCount(0);
    int rowIndex = 0;
    for (const Extraction &e : rows) {
        extractionTable->insertRow(rowIndex);
        extractionTable->setItem(rowIndex, 0, new QTableWidgetItem(QString::number(e.id())));
        extractionTable->setItem(rowIndex, 1, new QTableWidgetItem(QString::number(e.lotId())));
        extractionTable->setItem(rowIndex, 2, new QTableWidgetItem(QString::number(e.machineId())));
        extractionTable->setItem(rowIndex, 3, new QTableWidgetItem(QString::number(e.targetCiterneId())));
        extractionTable->setItem(rowIndex, 4, new QTableWidgetItem(e.extractedAt().toString("dd/MM/yyyy")));
        extractionTable->setItem(rowIndex, 5, new QTableWidgetItem(QString::number(e.inputQuantityKg(), 'f', 2)));
        extractionTable->setItem(rowIndex, 6, new QTableWidgetItem(QString::number(e.outputOilL(), 'f', 2)));
        extractionTable->setItem(rowIndex, 7, new QTableWidgetItem(e.status()));
        ++rowIndex;
    }
}
// ============================================================================
// CITERNE PAGE - reuse simple table + actions (you can expand later)
// ============================================================================
void MainWindow::createCiternesPage()
{
    citernesPage = new QWidget();
    citernesPage->setObjectName("citernesPage");
    QVBoxLayout *mainLay = new QVBoxLayout(citernesPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    QWidget *hdr = createHeaderWidget("Gestion Avancée des Citernes");
    mainLay->addWidget(hdr);

    // Content
    QWidget *content = new QWidget();
    content->setObjectName("moduleCard");
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(8,8,8,8);
    contentLay->setSpacing(10);

    // Search row for citernes (only search input)
    QHBoxLayout *ctrl = new QHBoxLayout();
    searchBoxCiternes = new QLineEdit();
    searchBoxCiternes->setObjectName("searchBoxCiternes");
    searchBoxCiternes->setPlaceholderText("Rechercher par ID / Qualité ...");
    searchBoxCiternes->setFixedHeight(34);
    connect(searchBoxCiternes, &QLineEdit::textChanged, this, &MainWindow::searchCiternes);

    ctrl->addWidget(searchBoxCiternes);
    ctrl->addStretch();
    contentLay->addLayout(ctrl);
    
    // Advanced features row
    QHBoxLayout *advCtrl = new QHBoxLayout();
    blendingBtn = new QPushButton("🧪 Simulateur Qualité");
    blendingBtn->setProperty("role", "accent");
    blendingBtn->setFixedSize(180,34);
    connect(blendingBtn, &QPushButton::clicked, this, &MainWindow::openBlendingSimulator);
    
    alertsBtn = new QPushButton("⚠️ Alertes & Notifications");
    alertsBtn->setProperty("role", "secondary");
    alertsBtn->setFixedSize(180,34);
    connect(alertsBtn, &QPushButton::clicked, this, &MainWindow::showNotifications);
    
    QPushButton *maintBtn = new QPushButton("🔧 Maintenance Prédictive");
    maintBtn->setProperty("role", "secondary");
    maintBtn->setFixedSize(180,34);
    connect(maintBtn, &QPushButton::clicked, this, &MainWindow::showEquipmentStatus);

    QPushButton *statsCiterneBtn = new QPushButton("📊 Statistiques");
    statsCiterneBtn->setProperty("role", "secondary");
    statsCiterneBtn->setFixedSize(140,34);
    connect(statsCiterneBtn, &QPushButton::clicked, this, &MainWindow::showCiterneStatistics);
    
    advCtrl->addWidget(blendingBtn);
    advCtrl->addWidget(alertsBtn);
    advCtrl->addWidget(maintBtn);
    advCtrl->addWidget(statsCiterneBtn);
    advCtrl->addStretch();
    contentLay->addLayout(advCtrl);

    // Table for citernes
    citernesTable = new QTableWidget();
    citernesTable->setColumnCount(8);
    citernesTable->setHorizontalHeaderLabels({"ID","Capacité (L)","Volume (L)","Remplissage","Qualité","Temp (°C)","Dernier remplissage","Actions"});
    citernesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    citernesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    citernesTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    citernesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    citernesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    citernesTable->verticalHeader()->setVisible(false);
    citernesTable->verticalHeader()->setDefaultSectionSize(44);
    citernesTable->verticalHeader()->setMinimumSectionSize(40);
    citernesTable->setAlternatingRowColors(true);
    citernesTable->setShowGrid(false);
    citernesTable->setObjectName("citernesTable");
    contentLay->addWidget(citernesTable);

    // action buttons below table just like clients
    QHBoxLayout *actions = new QHBoxLayout();
    addCiterneBtn = new QPushButton("+ Ajouter");
    addCiterneBtn->setProperty("role", "primary");
    addCiterneBtn->setFixedSize(120,36);
    connect(addCiterneBtn, &QPushButton::clicked, this, &MainWindow::showAddCiterneDialog);

    editCiterneBtn = new QPushButton("✎ Éditer");
    editCiterneBtn->setProperty("role", "secondary");
    editCiterneBtn->setFixedSize(100,36);
    connect(editCiterneBtn, &QPushButton::clicked, this, &MainWindow::editSelectedCiterne);

    detailsCiterneBtn = new QPushButton("👁 Voir");
    detailsCiterneBtn->setProperty("role", "secondary");
    detailsCiterneBtn->setFixedSize(100,36);
    connect(detailsCiterneBtn, &QPushButton::clicked, this, &MainWindow::viewCiterneDetails);

    deleteCiterneBtn = new QPushButton("🗑 Supprimer");
    deleteCiterneBtn->setProperty("role", "danger");
    deleteCiterneBtn->setFixedSize(120,36);
    connect(deleteCiterneBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedCiterne);

    actions->addWidget(addCiterneBtn);
    actions->addWidget(editCiterneBtn);
    actions->addWidget(detailsCiterneBtn);
    actions->addWidget(deleteCiterneBtn);
    actions->addStretch();
    contentLay->addLayout(actions);

    mainLay->addWidget(content);
    stackedWidget->addWidget(citernesPage);
}

// ============================================================================
// RECEPTION and EXTRACTION pages (simple placeholders to extend later)
// ============================================================================


void MainWindow::createExtractionPage()
{
    extractionPage = new QWidget();
    extractionPage->setObjectName("extractionPage");
    QVBoxLayout *mainLay = new QVBoxLayout(extractionPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    mainLay->addWidget(createHeaderWidget("Gestion des Extractions"));

    QWidget *content = new QWidget();
    content->setObjectName("moduleCard");
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(8,8,8,8);
    contentLay->setSpacing(10);

    QHBoxLayout *ctrl = new QHBoxLayout();

    searchBoxExtraction = new QLineEdit();
    searchBoxExtraction->setObjectName("searchBoxExtraction");
    searchBoxExtraction->setPlaceholderText("Rechercher par ID / Lot / Statut...");
    searchBoxExtraction->setFixedHeight(34);
    connect(searchBoxExtraction, &QLineEdit::textChanged, this, &MainWindow::searchExtraction);

    ctrl->addWidget(searchBoxExtraction);
    ctrl->addStretch();
    contentLay->addLayout(ctrl);

    extractionTable = new QTableWidget();
    extractionTable->setObjectName("extractionTable");
    extractionTable->setColumnCount(8);
    extractionTable->setHorizontalHeaderLabels({"ID", "Lot ID", "Machine ID", "Citerne cible", "Date", "Entrée (kg)", "Huile (L)", "Statut"});
    extractionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    extractionTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    extractionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    extractionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    extractionTable->verticalHeader()->setVisible(false);
    extractionTable->verticalHeader()->setDefaultSectionSize(44);
    extractionTable->verticalHeader()->setMinimumSectionSize(40);
    extractionTable->setAlternatingRowColors(true);
    extractionTable->setShowGrid(false);
    contentLay->addWidget(extractionTable);

    // Actions row (like Clients/Citernes)
    QHBoxLayout *actions = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton("+ Ajouter");
    addBtn->setProperty("role", "primary");
    addBtn->setFixedSize(120,36);
    connect(addBtn, &QPushButton::clicked, this, &MainWindow::showAddExtractionDialog);

    QPushButton *editBtn = new QPushButton("✎ Éditer");
    editBtn->setProperty("role", "secondary");
    editBtn->setFixedSize(100,36);
    connect(editBtn, &QPushButton::clicked, this, &MainWindow::editSelectedExtraction);

    QPushButton *delBtn = new QPushButton("🗑 Supprimer");
    delBtn->setProperty("role", "danger");
    delBtn->setFixedSize(120,36);
    connect(delBtn, &QPushButton::clicked, this, &MainWindow::deleteSelectedExtraction);

    QPushButton *refreshBtn = new QPushButton("🔄 Rafraîchir");
    refreshBtn->setProperty("role", "secondary");
    refreshBtn->setFixedSize(130,36);
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshExtractionData);

    actions->addWidget(addBtn);
    actions->addWidget(editBtn);
    actions->addWidget(delBtn);
    actions->addWidget(refreshBtn);
    actions->addStretch();
    contentLay->addLayout(actions);

    mainLay->addWidget(content);
    refreshExtractionData();
    stackedWidget->addWidget(extractionPage);
}

// ============================================================================
// STATISTICS page (simple placeholder)
// ============================================================================
void MainWindow::createStatisticsPage()
{
    statisticsPage = new QWidget();
    statisticsPage->setObjectName("statisticsPage");
    QVBoxLayout *mainLayout = new QVBoxLayout(statisticsPage);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(createHeaderWidget("Statistiques Clients"));

    QFrame *hero = new QFrame();
    hero->setObjectName("statsHero");
    QHBoxLayout *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(18, 14, 18, 14);
    heroLayout->setSpacing(12);

    QVBoxLayout *heroText = new QVBoxLayout();
    heroText->setSpacing(2);
    QLabel *heroTitle = new QLabel("Vue d'ensemble commerciale");
    heroTitle->setObjectName("statsHeroTitle");
    QLabel *heroSubtitle = new QLabel("KPI clients, activité récente et qualité des profils.");
    heroSubtitle->setObjectName("statsHeroSubtitle");
    heroText->addWidget(heroTitle);
    heroText->addWidget(heroSubtitle);

    QLabel *heroChip = new QLabel("Temps réel");
    heroChip->setObjectName("statsHeroChip");

    heroLayout->addLayout(heroText);
    heroLayout->addStretch();
    heroLayout->addWidget(heroChip);
    mainLayout->addWidget(hero);

    QWidget *content = new QWidget();
    content->setObjectName("statsSurface");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(14, 14, 14, 14);
    contentLayout->setSpacing(14);

    QGridLayout *cardsGrid = new QGridLayout();
    cardsGrid->setHorizontalSpacing(10);
    cardsGrid->setVerticalSpacing(10);

    auto createStatCard = [&](const QString &title, QLabel **valueLabel, const QString &tone) {
        QFrame *card = new QFrame();
        card->setObjectName("statCard");
        card->setProperty("tone", tone);
        QVBoxLayout *lay = new QVBoxLayout(card);
        lay->setContentsMargins(12, 10, 12, 10);
        lay->setSpacing(3);
        QLabel *titleLabel = new QLabel(title);
        titleLabel->setObjectName("statCardTitle");
        QLabel *val = new QLabel("0");
        val->setObjectName("statCardValue");
        lay->addWidget(titleLabel);
        lay->addWidget(val);
        lay->addStretch();
        *valueLabel = val;
        return card;
    };

    cardsGrid->addWidget(createStatCard("Total clients", &statsTotalClientsValue, "primary"), 0, 0);
    cardsGrid->addWidget(createStatCard("Clients actifs", &statsActiveClientsValue, "success"), 0, 1);
    cardsGrid->addWidget(createStatCard("Clients inactifs", &statsInactiveClientsValue, "danger"), 0, 2);
    cardsGrid->addWidget(createStatCard("Nouveaux ce mois", &statsNewThisMonthValue, "accent"), 1, 0);
    cardsGrid->addWidget(createStatCard("Profils complets", &statsCompleteProfilesValue, "neutral"), 1, 1);

    QFrame *cityCard = new QFrame();
    cityCard->setObjectName("statsInsightCard");
    QVBoxLayout *cityLay = new QVBoxLayout(cityCard);
    cityLay->setContentsMargins(12, 10, 12, 10);
    QLabel *cityTitle = new QLabel("Ville la plus représentée");
    cityTitle->setObjectName("statsInsightTitle");
    statsTopCityValue = new QLabel("N/A");
    statsTopCityValue->setObjectName("statsInsightValue");
    cityLay->addWidget(cityTitle);
    cityLay->addWidget(statsTopCityValue);
    cityLay->addStretch();
    cardsGrid->addWidget(cityCard, 1, 2);

    QFrame *analyticsRow = new QFrame();
    analyticsRow->setObjectName("statsRow");
    QHBoxLayout *analyticsLayout = new QHBoxLayout(analyticsRow);
    analyticsLayout->setContentsMargins(0, 0, 0, 0);
    analyticsLayout->setSpacing(10);

    QFrame *breakdownCard = new QFrame();
    breakdownCard->setObjectName("statsInsightCard");
    QVBoxLayout *breakLay = new QVBoxLayout(breakdownCard);
    breakLay->setContentsMargins(12, 10, 12, 10);
    QLabel *breakTitle = new QLabel("Répartition par statut");
    breakTitle->setObjectName("statsInsightTitle");
    statsStatusBreakdownValue = new QLabel("Aucune donnée");
    statsStatusBreakdownValue->setWordWrap(true);
    statsStatusBreakdownValue->setObjectName("statsBreakdownValue");
    breakLay->addWidget(breakTitle);
    breakLay->addWidget(statsStatusBreakdownValue);

    backFromStatsButton = new QPushButton("Retour à la liste clients");
    backFromStatsButton->setProperty("role", "secondary");
    backFromStatsButton->setFixedHeight(34);
    connect(backFromStatsButton, &QPushButton::clicked, this, &MainWindow::showMainListView);
    breakLay->addStretch();
    breakLay->addWidget(backFromStatsButton, 0, Qt::AlignRight);

    QFrame *donutCard = new QFrame();
    donutCard->setObjectName("statsInsightCard");
    donutCard->setMinimumWidth(360);
    donutCard->setMinimumHeight(330);
    QVBoxLayout *donutLay = new QVBoxLayout(donutCard);
    donutLay->setContentsMargins(12, 10, 12, 10);
    donutLay->setSpacing(6);

    QLabel *donutTitle = new QLabel("Distribution des statuts");
    donutTitle->setObjectName("statsInsightTitle");
    donutLay->addWidget(donutTitle);

    statsDonutChartLabel = new QLabel();
    statsDonutChartLabel->setObjectName("statsDonutChart");
    statsDonutChartLabel->setFixedSize(210, 210);
    statsDonutChartLabel->setAlignment(Qt::AlignCenter);
    donutLay->addWidget(statsDonutChartLabel, 0, Qt::AlignCenter);

    auto createLegendItem = [&](const QString &colorHex, const QString &baseText, QLabel **valueOut) {
        QWidget *item = new QWidget();
        item->setObjectName("statsLegendItem");
        QHBoxLayout *hl = new QHBoxLayout(item);
        hl->setContentsMargins(8, 6, 8, 6);
        hl->setSpacing(8);

        QLabel *dot = new QLabel();
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(colorHex));

        QLabel *txt = new QLabel(baseText);
        txt->setObjectName("statsLegendValue");
        hl->addWidget(dot);
        hl->addWidget(txt, 1);

        *valueOut = txt;
        return item;
    };

    QGridLayout *legendGrid = new QGridLayout();
    legendGrid->setHorizontalSpacing(8);
    legendGrid->setVerticalSpacing(8);
    legendGrid->addWidget(createLegendItem("#1F9FE0", "Non défini", &statsLegendUndefinedValue), 0, 0);
    legendGrid->addWidget(createLegendItem("#FF3A3A", "Inactif", &statsLegendInactiveValue), 0, 1);
    legendGrid->addWidget(createLegendItem("#38C86A", "Actif", &statsLegendActiveValue), 1, 0);
    legendGrid->addWidget(createLegendItem("#FF7A00", "Autre", &statsLegendOtherValue), 1, 1);
    donutLay->addLayout(legendGrid);
    donutLay->addStretch();

    analyticsLayout->addWidget(donutCard, 1);
    analyticsLayout->addWidget(breakdownCard, 1);

    contentLayout->addLayout(cardsGrid);
    contentLayout->addWidget(analyticsRow);

    mainLayout->addWidget(content);
    stackedWidget->addWidget(statisticsPage);
    updateClientStatistics();
}

void MainWindow::createCiterneStatisticsPage()
{
    citerneStatisticsPage = new QWidget();
    citerneStatisticsPage->setObjectName("citerneStatisticsPage");
    QVBoxLayout *mainLayout = new QVBoxLayout(citerneStatisticsPage);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(createHeaderWidget("Statistiques Citernes"));

    QFrame *hero = new QFrame();
    hero->setObjectName("statsHero");
    QHBoxLayout *heroLayout = new QHBoxLayout(hero);
    heroLayout->setContentsMargins(18, 14, 18, 14);
    heroLayout->setSpacing(12);

    QVBoxLayout *heroText = new QVBoxLayout();
    heroText->setSpacing(2);
    QLabel *heroTitle = new QLabel("Vue d'ensemble des citernes");
    heroTitle->setObjectName("statsHeroTitle");
    QLabel *heroSubtitle = new QLabel("Capacité, remplissage, alertes et qualité en temps réel.");
    heroSubtitle->setObjectName("statsHeroSubtitle");
    heroText->addWidget(heroTitle);
    heroText->addWidget(heroSubtitle);

    QLabel *heroChip = new QLabel("Temps réel");
    heroChip->setObjectName("statsHeroChip");

    heroLayout->addLayout(heroText);
    heroLayout->addStretch();
    heroLayout->addWidget(heroChip);
    mainLayout->addWidget(hero);

    QWidget *content = new QWidget();
    content->setObjectName("statsSurface");
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(14, 14, 14, 14);
    contentLayout->setSpacing(14);

    QGridLayout *cardsGrid = new QGridLayout();
    cardsGrid->setHorizontalSpacing(10);
    cardsGrid->setVerticalSpacing(10);

    auto createStatCard = [&](const QString &title, QLabel **valueLabel, const QString &tone) {
        QFrame *card = new QFrame();
        card->setObjectName("statCard");
        card->setProperty("tone", tone);
        QVBoxLayout *lay = new QVBoxLayout(card);
        lay->setContentsMargins(12, 10, 12, 10);
        lay->setSpacing(3);
        QLabel *titleLabel = new QLabel(title);
        titleLabel->setObjectName("statCardTitle");
        QLabel *val = new QLabel("0");
        val->setObjectName("statCardValue");
        lay->addWidget(titleLabel);
        lay->addWidget(val);
        lay->addStretch();
        *valueLabel = val;
        return card;
    };

    cardsGrid->addWidget(createStatCard("Total citernes", &citerneStatsTotalValue, "primary"), 0, 0);
    cardsGrid->addWidget(createStatCard("Capacité totale (L)", &citerneStatsCapacityValue, "success"), 0, 1);
    cardsGrid->addWidget(createStatCard("Volume total (L)", &citerneStatsVolumeValue, "accent"), 0, 2);
    cardsGrid->addWidget(createStatCard("Taux moyen (%)", &citerneStatsFillRateValue, "neutral"), 1, 0);
    cardsGrid->addWidget(createStatCard("Température moyenne", &citerneStatsTempValue, "primary"), 1, 1);
    cardsGrid->addWidget(createStatCard("Alertes critiques", &citerneStatsCriticalValue, "danger"), 1, 2);

    QFrame *analyticsRow = new QFrame();
    analyticsRow->setObjectName("statsRow");
    QHBoxLayout *analyticsLayout = new QHBoxLayout(analyticsRow);
    analyticsLayout->setContentsMargins(0, 0, 0, 0);
    analyticsLayout->setSpacing(10);

    QFrame *breakdownCard = new QFrame();
    breakdownCard->setObjectName("statsInsightCard");
    QVBoxLayout *breakLay = new QVBoxLayout(breakdownCard);
    breakLay->setContentsMargins(12, 10, 12, 10);
    QLabel *breakTitle = new QLabel("Répartition des alertes");
    breakTitle->setObjectName("statsInsightTitle");
    citerneStatsBreakdownValue = new QLabel("Aucune donnée");
    citerneStatsBreakdownValue->setWordWrap(true);
    citerneStatsBreakdownValue->setObjectName("statsBreakdownValue");

    QLabel *topQualityTitle = new QLabel("Indice qualité moyen");
    topQualityTitle->setObjectName("statsInsightTitle");
    citerneStatsTopQualityValue = new QLabel("N/A");
    citerneStatsTopQualityValue->setObjectName("statsInsightValue");

    breakLay->addWidget(breakTitle);
    breakLay->addWidget(citerneStatsBreakdownValue);
    breakLay->addSpacing(8);
    breakLay->addWidget(topQualityTitle);
    breakLay->addWidget(citerneStatsTopQualityValue);

    backFromCiterneStatsButton = new QPushButton("Retour à la liste citernes");
    backFromCiterneStatsButton->setProperty("role", "secondary");
    backFromCiterneStatsButton->setFixedHeight(34);
    connect(backFromCiterneStatsButton, &QPushButton::clicked, this, &MainWindow::showMainCiterneListView);
    breakLay->addStretch();
    breakLay->addWidget(backFromCiterneStatsButton, 0, Qt::AlignRight);

    QFrame *donutCard = new QFrame();
    donutCard->setObjectName("statsInsightCard");
    donutCard->setMinimumWidth(360);
    donutCard->setMinimumHeight(330);
    QVBoxLayout *donutLay = new QVBoxLayout(donutCard);
    donutLay->setContentsMargins(12, 10, 12, 10);
    donutLay->setSpacing(6);

    QLabel *donutTitle = new QLabel("Distribution des alertes");
    donutTitle->setObjectName("statsInsightTitle");
    donutLay->addWidget(donutTitle);

    citerneStatsDonutChartLabel = new QLabel();
    citerneStatsDonutChartLabel->setObjectName("statsDonutChart");
    citerneStatsDonutChartLabel->setFixedSize(210, 210);
    citerneStatsDonutChartLabel->setAlignment(Qt::AlignCenter);
    donutLay->addWidget(citerneStatsDonutChartLabel, 0, Qt::AlignCenter);

    auto createLegendItem = [&](const QString &colorHex, const QString &baseText, QLabel **valueOut) {
        QWidget *item = new QWidget();
        item->setObjectName("statsLegendItem");
        QHBoxLayout *hl = new QHBoxLayout(item);
        hl->setContentsMargins(8, 6, 8, 6);
        hl->setSpacing(8);

        QLabel *dot = new QLabel();
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(colorHex));

        QLabel *txt = new QLabel(baseText);
        txt->setObjectName("statsLegendValue");
        hl->addWidget(dot);
        hl->addWidget(txt, 1);

        *valueOut = txt;
        return item;
    };

    QGridLayout *legendGrid = new QGridLayout();
    legendGrid->setHorizontalSpacing(8);
    legendGrid->setVerticalSpacing(8);
    legendGrid->addWidget(createLegendItem("#FF3A3A", "Critique", &citerneStatsLegendCriticalValue), 0, 0);
    legendGrid->addWidget(createLegendItem("#FF7A00", "Warning", &citerneStatsLegendWarningValue), 0, 1);
    legendGrid->addWidget(createLegendItem("#38C86A", "Normal", &citerneStatsLegendNormalValue), 1, 0);
    donutLay->addLayout(legendGrid);
    donutLay->addStretch();

    analyticsLayout->addWidget(donutCard, 1);
    analyticsLayout->addWidget(breakdownCard, 1);

    contentLayout->addLayout(cardsGrid);
    contentLayout->addWidget(analyticsRow);

    mainLayout->addWidget(content);
    stackedWidget->addWidget(citerneStatisticsPage);
    updateCiterneStatistics();
}

// ============================================================================
// HEADER CREATOR (for pages)
// ============================================================================

// ============================================================================
// STYLES
// ============================================================================
void MainWindow::applyStyles()
{
    QString qss = R"(
        QWidget { background: #F3F4EE; font-family: 'Segoe UI', 'Trebuchet MS', 'Candara', 'Verdana'; color: #1A2B28; }
        QLabel { background: transparent; color: #1A2B28; }
        #clientsPage, #citernesPage, #statisticsPage, #citerneStatisticsPage { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #F7F4EB, stop:1 #EEF6F4); }
        #sideBar { background: #102B2A; border-right: 1px solid #2B4945; }
        #sideBar QLabel { background: transparent; }
        #sideBrand { color: #F9EEE0; font-size: 22px; font-weight: 800; letter-spacing: 0.8px; }
        #sideSubtitle { color: #9CC4BE; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; }
        #dbStatusLabel { background: #1C3F3B; color: #EAF5F0; font-size: 11px; border: 1px solid #2A5A54; }
        QPushButton[nav="true"] { text-align: left; padding: 10px 12px; border-radius: 10px; color: #EAF5F0; background: transparent; border: 1px solid transparent; }
        QPushButton[nav="true"]:hover { background: #1F4843; border: 1px solid #2E6A62; }
        QPushButton[nav="true"]:checked { background: #2B5D56; color: #FFFFFF; font-weight: 700; border: 1px solid #4D8A80; }
        #pageHeader { background-color:#0D5B52; border-radius: 10px; }
        #moduleTabs { background: transparent; border: none; }
        QTabBar { border: none; }
        QTabBar::tab { background: #E8E8DF; color: #21423D; padding: 8px 16px; margin-right: 6px; border: 1px solid #CBD8D4; border-top-left-radius: 9px; border-top-right-radius: 9px; }
        QTabBar::tab:selected { background: #0D5B52; color: #FFFFFF; border: 1px solid #0D5B52; }
        #moduleCard { background: #FFFEFB; border: 1px solid #D8E2DE; border-radius: 12px; }
        #searchBox, #searchBoxCiternes { border:1px solid #A8BDB6; border-radius:8px; padding:7px 10px; background: #FBFCFA; selection-background-color: #0D5B52; }
        #searchBox:focus, #searchBoxCiternes:focus { border: 1px solid #0D5B52; background: #FFFFFF; }
        QPushButton { border-radius:8px; padding:7px 12px; border: 1px solid #C8D8D3; background: #F3F7F5; }
        QPushButton:hover { background: #E7EFEC; }
        QPushButton[role="primary"] { background: #0D5B52; color: #FFFFFF; border: 1px solid #0D5B52; font-weight: 700; }
        QPushButton[role="primary"]:hover { background: #0A4B44; }
        QPushButton[role="secondary"] { background: #EEF5F2; color: #22443E; border: 1px solid #BFD4CC; font-weight: 600; }
        QPushButton[role="secondary"]:hover { background: #E0ECE8; }
        QPushButton[role="accent"] { background: #C67D37; color: #FFFFFF; border: 1px solid #B06E2F; font-weight: 700; }
        QPushButton[role="accent"]:hover { background: #B06E2F; }
        QPushButton[role="danger"] { background: #8A3131; color: #FFFFFF; border: 1px solid #772727; font-weight: 700; }
        QPushButton[role="danger"]:hover { background: #742323; }
        #tableActionsCell { background: transparent; }
        #tableActionsCell QPushButton { border-radius: 6px; padding: 1px 8px; min-height: 28px; max-height: 28px; font-size: 11px; font-weight: 700; border: 1px solid transparent; }
        #tableActionsCell QPushButton[tableAction="view"] { background: #D9ECE8; color: #0E4A44; border-color: #A7CCC3; }
        #tableActionsCell QPushButton[tableAction="view"]:hover { background: #CBE4DF; }
        #tableActionsCell QPushButton[tableAction="edit"] { background: #E5E8FB; color: #2F3E8A; border-color: #C3CCF0; }
        #tableActionsCell QPushButton[tableAction="edit"]:hover { background: #DADFF7; }
        #tableActionsCell QPushButton[tableAction="danger"] { background: #F7DDDD; color: #872525; border-color: #E6B4B4; }
        #tableActionsCell QPushButton[tableAction="danger"]:hover { background: #F2CECE; }
        #tableActionsCell QPushButton[tableAction="fill"] { background: #DDF1DE; color: #1E6B2A; border-color: #B7DEB9; }
        #tableActionsCell QPushButton[tableAction="fill"]:hover { background: #CDEACF; }
        #tableActionsCell QPushButton[tableAction="drain"] { background: #F8E6CE; color: #8B5A16; border-color: #E7CCA4; }
        #tableActionsCell QPushButton[tableAction="drain"]:hover { background: #F3DCBB; }
        #statsHero { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #124E48, stop:1 #1C6A60); border: 1px solid #2F7D72; border-radius: 12px; }
        #statsHeroTitle { color: #FFFFFF; font-size: 18px; font-weight: 900; }
        #statsHeroSubtitle { color: #E4F2EE; font-size: 12px; font-weight: 600; }
        #statsHeroChip { background: #E8F4F1; color: #0C4E47; font-size: 11px; font-weight: 800; border-radius: 10px; padding: 4px 9px; border: 1px solid #C6E0D9; }
        #statsSurface { background: #FFFEFB; border: 1px solid #D8E2DE; border-radius: 12px; }
        #statCard { background: #FFFFFF; border: 1px solid #DFE8E4; border-radius: 10px; }
        #statCard[tone="primary"] { border-left: 4px solid #1F6A63; }
        #statCard[tone="success"] { border-left: 4px solid #2F8652; }
        #statCard[tone="danger"] { border-left: 4px solid #9A3D3D; }
        #statCard[tone="accent"] { border-left: 4px solid #C67D37; }
        #statCard[tone="neutral"] { border-left: 4px solid #51656A; }
        #statCardTitle { color: #38504B; font-size: 13px; font-weight: 800; }
        #statCardValue { color: #0E3D37; font-size: 28px; font-weight: 900; }
        #statsInsightCard { background: #FFFFFF; border: 1px solid #DFE8E4; border-radius: 10px; }
        #statsInsightTitle { color: #36514B; font-size: 14px; font-weight: 800; }
        #statsInsightValue { color: #0C4F47; font-size: 26px; font-weight: 900; }
        #statsBreakdownValue { color: #172927; font-size: 14px; font-weight: 700; line-height: 1.35; }
        #statsDonutChart { background: transparent; }
        #statsLegendItem { background: #F6FAF8; border: 1px solid #E2ECE8; border-radius: 8px; }
        #statsLegendValue { color: #1E3431; font-size: 12px; font-weight: 800; }
        QTableWidget { background: #FFFFFF; border: 1px solid #DCE5E1; border-radius: 10px; alternate-background-color: #F6FBF9; gridline-color: #ECF2F0; selection-background-color: #D6EAE3; selection-color: #0D2F2A; }
        QTableWidget::item { padding: 6px; border-bottom: 1px solid #EDF2F0; }
        QHeaderView::section { background: #114E47; color: white; padding: 9px; border: none; font-weight: 700; }
    )";
    setStyleSheet(qss);
}

// ============================================================================
// SAMPLE DATA - Clients
// ============================================================================
void MainWindow::populateClientsSampleData()
{
    struct C { int id; QString name, email, phone, address; QString reg; QString status; };
    QList<C> list = {
        {1, "Alice Dupont", "alice@example.com", "+33 6 11 22 33 44", "10 Rue A, Paris", "2021-05-10", "Actif"},
        {2, "Bob Martin", "bob@example.com", "+33 6 22 33 44 55", "5 Ave B, Lyon", "2022-01-20", "Actif"},
        {3, "Clara Lopez", "clara@example.com", "+33 6 33 44 55 66", "12 Bd C, Marseille", "2020-11-02", "Inactif"},
        {4, "David Rossi", "david@example.com", "+33 6 44 55 66 77", "7 Pl D, Lille", "2023-02-12", "Actif"}
    };

    clientsTable->setRowCount(list.size());
    for (int i=0;i<list.size();++i) {
        const C &c = list[i];
        clientsTable->setItem(i,0, new QTableWidgetItem(QString::number(c.id)));
        clientsTable->setItem(i,1, new QTableWidgetItem(c.name));
        clientsTable->setItem(i,2, new QTableWidgetItem(c.email));
        clientsTable->setItem(i,3, new QTableWidgetItem(c.phone));
        clientsTable->setItem(i,4, new QTableWidgetItem(c.address));
        clientsTable->setItem(i,5, new QTableWidgetItem(c.reg));
        clientsTable->setItem(i,6, new QTableWidgetItem(c.status));
        clientsTable->setCellWidget(i,7, createClientActionsWidget(c.id));
    }
    clientsTable->resizeColumnsToContents();
    updateClientStatistics();
}

// ============================================================================
// SAMPLE DATA - Citernes
// ============================================================================
void MainWindow::populateCiternesSampleData()
{
    QStringList data = {
        "1|1000|650|1.20|18.5|2026-02-01T10:00:00Z",
        "2|2000|1500|1.05|16.8|2026-01-28T09:30:00Z",
        "3|500|120|0.95|19.2|2026-02-02T08:15:00Z",
        "4|750|700|1.30|17.5|2026-01-30T14:40:00Z"
    };

    citernesTable->setRowCount(data.size());
    for (int i=0;i<data.size(); ++i) {
        QStringList parts = data[i].split("|");
        int id = parts[0].toInt();
        double capacite = parts[1].toDouble();
        double volume = parts[2].toDouble();
        QString qual = parts[3];
        double temp = parts[4].toDouble();
        QString last = parts[5];

        citernesTable->setItem(i,0, new QTableWidgetItem(QString::number(id)));
        citernesTable->setItem(i,1, new QTableWidgetItem(QString::number(capacite,'f',2)));
        citernesTable->setItem(i,2, new QTableWidgetItem(QString::number(volume,'f',2)));
        int pct = (capacite>0) ? int((volume/capacite)*100.0) : 0;
        citernesTable->setCellWidget(i,3, createProgressBar(pct));
        citernesTable->setItem(i,4, new QTableWidgetItem(qual));
        citernesTable->setItem(i,5, new QTableWidgetItem(QString::number(temp,'f',1)));
        citernesTable->setItem(i,6, new QTableWidgetItem(last));

        citernesTable->setCellWidget(i,7, createCiterneActionsWidget(id));
    }
    citernesTable->resizeColumnsToContents();
}

bool MainWindow::setupOracleSchema()
{
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery q(db);
    auto execSql = [&](const QString &sql, QString &lastError) {
        if (!q.exec(sql)) {
            lastError = q.lastError().text();
            return false;
        }
        return true;
    };

    QString lastError;
    const QStringList ddl = {
        "BEGIN EXECUTE IMMEDIATE 'DROP TRIGGER TRG_CLIENTS_APP_BI'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -4080 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'DROP TRIGGER TRG_CITERNES_APP_BI'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -4080 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE SEQ_CLIENTS_APP'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'DROP SEQUENCE SEQ_CITERNES_APP'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -2289 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'DROP TABLE CLIENTS_APP CASCADE CONSTRAINTS PURGE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'DROP TABLE CITERNES_APP CASCADE CONSTRAINTS PURGE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -942 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE TABLE CLIENT ("
        "ID NUMBER PRIMARY KEY, "
        "NAME VARCHAR2(120) NOT NULL, "
        "EMAIL VARCHAR2(180), "
        "PHONE VARCHAR2(40), "
        "ADDRESS VARCHAR2(240), "
        "CREATED_AT DATE DEFAULT SYSDATE NOT NULL, "
        "STATUS VARCHAR2(30))'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE TABLE CITERNE ("
        "ID NUMBER PRIMARY KEY, "
        "CODE VARCHAR2(40) NOT NULL UNIQUE, "
        "CAPACITY_L NUMBER(12,2) NOT NULL, "
        "CURRENT_VOLUME_L NUMBER(12,2) DEFAULT 0 NOT NULL, "
        "QUALITY_INDEX NUMBER(6,2), "
        "TEMPERATURE_C NUMBER(5,2), "
        "LAST_FILLING_AT DATE, "
        "STATUS VARCHAR2(30), "
        "CONSTRAINT CK_CITERNE_CAP_POSITIVE CHECK (CAPACITY_L > 0), "
        "CONSTRAINT CK_CITERNE_VOL_VALID CHECK (CURRENT_VOLUME_L >= 0 AND CURRENT_VOLUME_L <= CAPACITY_L))'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE SEQUENCE SEQ_CLIENT START WITH 1 INCREMENT BY 1 NOCACHE NOCYCLE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE SEQUENCE SEQ_CITERNE START WITH 1 INCREMENT BY 1 NOCACHE NOCYCLE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "CREATE OR REPLACE TRIGGER TRG_CLIENT_BI "
        "BEFORE INSERT ON CLIENT "
        "FOR EACH ROW "
        "WHEN (NEW.id IS NULL) "
        "BEGIN "
        "SELECT SEQ_CLIENT.NEXTVAL INTO :NEW.ID FROM dual; "
        "END;",

        "CREATE OR REPLACE TRIGGER TRG_CITERNE_BI "
        "BEFORE INSERT ON CITERNE "
        "FOR EACH ROW "
        "WHEN (NEW.id IS NULL) "
        "BEGIN "
        "SELECT SEQ_CITERNE.NEXTVAL INTO :NEW.ID FROM dual; "
        "END;"
    };

    for (const QString &sql : ddl) {
        if (!execSql(sql, lastError)) {
            QMessageBox::critical(this, "Oracle", "Initialisation du schéma échouée:\n" + lastError);
            return false;
        }
    }

    return true;
}

void MainWindow::loadClientsFromOracle()
{
    if (!oracleActive || !db.isOpen()) {
        return;
    }

    QString errorMessage;
    const QList<Client> clients = Client::afficher(db, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, "Oracle", "Chargement clients échoué:\n" + errorMessage);
        return;
    }

    clientsTable->setRowCount(0);
    int row = 0;
    for (const Client &client : clients) {
        const int id = client.id();
        clientsTable->insertRow(row);
        clientsTable->setItem(row,0, new QTableWidgetItem(QString::number(id)));
        clientsTable->setItem(row,1, new QTableWidgetItem(client.name()));
        clientsTable->setItem(row,2, new QTableWidgetItem(client.email()));
        clientsTable->setItem(row,3, new QTableWidgetItem(client.phone()));
        clientsTable->setItem(row,4, new QTableWidgetItem(client.address()));
        clientsTable->setItem(row,5, new QTableWidgetItem(client.createdAt().toString("yyyy-MM-dd")));
        clientsTable->setItem(row,6, new QTableWidgetItem(client.status()));
        clientsTable->setCellWidget(row,7, createClientActionsWidget(id));
        ++row;
    }
    clientsTable->resizeColumnsToContents();
    updateClientStatistics();
}

void MainWindow::loadCiternesFromOracle()
{
    if (!oracleActive || !db.isOpen()) {
        return;
    }

    QString errorMessage;
    const QList<Citerne> citernes = Citerne::afficher(db, &errorMessage);
    if (!errorMessage.isEmpty()) {
        QMessageBox::critical(this, "Oracle", "Chargement citernes échoué:\n" + errorMessage);
        return;
    }

    citernesTable->setRowCount(0);
    int row = 0;
    for (const Citerne &citerne : citernes) {
        const int id = citerne.id();
        const double capacite = citerne.capaciteL();
        const double volume = citerne.volumeL();
        citernesTable->insertRow(row);
        citernesTable->setItem(row,0, new QTableWidgetItem(QString::number(id)));
        citernesTable->setItem(row,1, new QTableWidgetItem(QString::number(capacite,'f',2)));
        citernesTable->setItem(row,2, new QTableWidgetItem(QString::number(volume,'f',2)));
        citernesTable->setCellWidget(row,3, createProgressWidget(capacite > 0 ? int((volume / capacite) * 100.0) : 0));
        citernesTable->setItem(row,4, new QTableWidgetItem(citerne.qualite()));
        citernesTable->setItem(row,5, new QTableWidgetItem(QString::number(citerne.temperatureC(),'f',1)));
        citernesTable->setItem(row,6, new QTableWidgetItem(citerne.dernierRemplissage().toString("yyyy-MM-dd")));
        citernesTable->setCellWidget(row,7, createCiterneActionsWidget(id));
        ++row;
    }
    citernesTable->resizeColumnsToContents();
}

QWidget* MainWindow::createClientActionsWidget(int id)
{
    QWidget *w = new QWidget();
    w->setObjectName("tableActionsCell");
    QHBoxLayout *hl = new QHBoxLayout(w);
    hl->setContentsMargins(0,0,0,0);
    hl->setSpacing(4);

    QPushButton *view = new QPushButton("Voir");
    view->setProperty("tableAction", "view");
    view->setFixedSize(62,28);
    view->setCursor(Qt::PointingHandCursor);
    view->setToolTip("Afficher le client");
    view->setProperty("client_id", id);
    connect(view, &QPushButton::clicked, this, &MainWindow::viewSelectedClient);

    QPushButton *edit = new QPushButton("Edit");
    edit->setProperty("tableAction", "edit");
    edit->setFixedSize(62,28);
    edit->setCursor(Qt::PointingHandCursor);
    edit->setToolTip("Modifier le client");
    edit->setProperty("client_id", id);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editSelectedClient);

    QPushButton *del = new QPushButton("Suppr");
    del->setProperty("tableAction", "danger");
    del->setFixedSize(66,28);
    del->setCursor(Qt::PointingHandCursor);
    del->setToolTip("Supprimer le client");
    del->setProperty("client_id", id);
    connect(del, &QPushButton::clicked, this, &MainWindow::deleteSelectedClient);

    hl->addWidget(view);
    hl->addWidget(edit);
    hl->addWidget(del);
    hl->addStretch();
    return w;
}

QWidget* MainWindow::createCiterneActionsWidget(int id)
{
    QWidget *aw = new QWidget();
    aw->setObjectName("tableActionsCell");
    QHBoxLayout *al = new QHBoxLayout(aw);
    al->setContentsMargins(0,0,0,0);
    al->setSpacing(4);

    QPushButton *fill = new QPushButton("Rempl");
    fill->setProperty("tableAction", "fill");
    fill->setFixedSize(68,28);
    fill->setCursor(Qt::PointingHandCursor);
    fill->setToolTip("Ajouter du volume");
    fill->setProperty("citerne_id", id);
    connect(fill, &QPushButton::clicked, this, &MainWindow::onFillButtonClicked);

    QPushButton *drain = new QPushButton("Vider");
    drain->setProperty("tableAction", "drain");
    drain->setFixedSize(62,28);
    drain->setCursor(Qt::PointingHandCursor);
    drain->setToolTip("Retirer du volume");
    drain->setProperty("citerne_id", id);
    connect(drain, &QPushButton::clicked, this, &MainWindow::onDrainButtonClicked);

    QPushButton *edit = new QPushButton("Edit");
    edit->setProperty("tableAction", "edit");
    edit->setFixedSize(62,28);
    edit->setCursor(Qt::PointingHandCursor);
    edit->setToolTip("Modifier la citerne");
    edit->setProperty("citerne_id", id);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editSelectedCiterne);

    al->addWidget(fill);
    al->addWidget(drain);
    al->addWidget(edit);
    al->addStretch();
    return aw;
}

// small helper: create progress widget for citernes
QWidget* MainWindow::createProgressWidget(int percent)
{
    QWidget *w = new QWidget();
    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setContentsMargins(4,4,4,4);
    QProgressBar *pb = new QProgressBar();
    pb->setRange(0,100);
    pb->setValue(qBound(0,percent,100));
    pb->setFixedHeight(18);
    lay->addWidget(pb);
    return w;
}

// For citernes sample we used createProgressBar reference earlier — implement alias:
QWidget* MainWindow::createProgressBar(int percent) { return createProgressWidget(percent); }

// ============================================================================
// HELPERS: find rows by id
// ============================================================================
int MainWindow::findClientRowById(int id)
{
    for (int r=0;r<clientsTable->rowCount();++r) {
        QTableWidgetItem *it = clientsTable->item(r,0);
        if (it && it->text().toInt()==id) return r;
    }
    return -1;
}

int MainWindow::findCiterneRowById(int id)
{
    for (int r=0;r<citernesTable->rowCount();++r) {
        QTableWidgetItem *it = citernesTable->item(r,0);
        if (it && it->text().toInt()==id) return r;
    }
    return -1;
}

void MainWindow::updateClientStatistics()
{
    if (!clientsTable || !statsTotalClientsValue || !statsStatusBreakdownValue || !statsTopCityValue) {
        return;
    }

    const int total = clientsTable->rowCount();
    int active = 0;
    int inactive = 0;
    int undefinedStatus = 0;
    int otherStatus = 0;
    int newThisMonth = 0;
    int completeProfiles = 0;
    QMap<QString, int> statusCount;
    QMap<QString, int> cityCount;

    const QDate now = QDate::currentDate();
    for (int r = 0; r < total; ++r) {
        const QString email = clientsTable->item(r, 2) ? clientsTable->item(r, 2)->text().trimmed() : QString();
        const QString phone = clientsTable->item(r, 3) ? clientsTable->item(r, 3)->text().trimmed() : QString();
        const QString address = clientsTable->item(r, 4) ? clientsTable->item(r, 4)->text().trimmed() : QString();
        const QString dateText = clientsTable->item(r, 5) ? clientsTable->item(r, 5)->text().trimmed() : QString();
        const QString status = clientsTable->item(r, 6) ? clientsTable->item(r, 6)->text().trimmed() : QString();

        const QString normalizedStatus = status.isEmpty() ? QString("Non défini") : status;
        statusCount[normalizedStatus]++;
        if (normalizedStatus.compare("Actif", Qt::CaseInsensitive) == 0) {
            ++active;
        } else if (normalizedStatus.compare("Inactif", Qt::CaseInsensitive) == 0) {
            ++inactive;
        } else if (normalizedStatus.compare("Non défini", Qt::CaseInsensitive) == 0) {
            ++undefinedStatus;
        } else {
            ++otherStatus;
        }

        if (!email.isEmpty() && !phone.isEmpty() && !address.isEmpty()) {
            ++completeProfiles;
        }

        const QDate inscriptionDate = QDate::fromString(dateText, "yyyy-MM-dd");
        if (inscriptionDate.isValid() && inscriptionDate.year() == now.year() && inscriptionDate.month() == now.month()) {
            ++newThisMonth;
        }

        if (!address.isEmpty()) {
            QString city = address;
            const int commaIndex = address.lastIndexOf(',');
            if (commaIndex >= 0) {
                city = address.mid(commaIndex + 1).trimmed();
            }
            if (!city.isEmpty()) {
                cityCount[city]++;
            }
        }
    }

    QString topCity = "N/A";
    int topCityCount = 0;
    for (auto it = cityCount.constBegin(); it != cityCount.constEnd(); ++it) {
        if (it.value() > topCityCount) {
            topCity = it.key();
            topCityCount = it.value();
        }
    }
    if (topCityCount > 0) {
        topCity += QString(" (%1)").arg(topCityCount);
    }

    QStringList statusLines;
    for (auto it = statusCount.constBegin(); it != statusCount.constEnd(); ++it) {
        statusLines << QString("- %1: %2").arg(it.key()).arg(it.value());
    }
    if (statusLines.isEmpty()) {
        statusLines << "Aucune donnée";
    }

    statsTotalClientsValue->setText(QString::number(total));
    statsActiveClientsValue->setText(QString::number(active));
    statsInactiveClientsValue->setText(QString::number(inactive));
    statsNewThisMonthValue->setText(QString::number(newThisMonth));
    statsCompleteProfilesValue->setText(QString::number(completeProfiles));
    statsStatusBreakdownValue->setText(statusLines.join("\n"));
    statsTopCityValue->setText(topCity);

    if (statsDonutChartLabel) {
        const QList<int> donutValues = {undefinedStatus, inactive, active, otherStatus};
        const QList<QColor> donutColors = {
            QColor("#1F9FE0"),
            QColor("#FF3A3A"),
            QColor("#38C86A"),
            QColor("#FF7A00")
        };
        const QSize chartSize = statsDonutChartLabel->size().isValid() ? statsDonutChartLabel->size() : QSize(280, 240);
        statsDonutChartLabel->setPixmap(buildDonutChartPixmap(chartSize,
                                                              donutValues,
                                                              donutColors,
                                                              QString::number(total),
                                                              QString("clients")));
    }

    if (statsLegendActiveValue) {
        statsLegendActiveValue->setText(formatLegendLine("Actif", active, total));
    }
    if (statsLegendInactiveValue) {
        statsLegendInactiveValue->setText(formatLegendLine("Inactif", inactive, total));
    }
    if (statsLegendUndefinedValue) {
        statsLegendUndefinedValue->setText(formatLegendLine("Non défini", undefinedStatus, total));
    }
    if (statsLegendOtherValue) {
        statsLegendOtherValue->setText(formatLegendLine("Autre", otherStatus, total));
    }
}

void MainWindow::showClientComparisonDashboard()
{
    if (!clientsTable || clientsTable->rowCount() < 2) {
        QMessageBox::warning(this, "Comparatif clients", "Ajoutez au moins deux clients pour lancer la comparaison.");
        return;
    }

    auto parseReceptionDate = [](const QString &value) {
        const QString text = value.trimmed();
        if (text.isEmpty()) {
            return QDate();
        }
        QDate d = QDate::fromString(text, "yyyy-MM-dd");
        if (!d.isValid()) d = QDate::fromString(text, "dd/MM/yyyy");
        if (!d.isValid()) d = QDate::fromString(text, "yyyy/MM/dd");
        return d;
    };

    struct ClientMetrics {
        int lots = 0;
        double totalKg = 0.0;
        double avgKg = 0.0;
        QDate lastReception;
        int recencyDays = -1;
    };

    auto computeMetrics = [&](int clientId) {
        ClientMetrics metrics;
        const QDate today = QDate::currentDate();

        if (oracleActive && db.isOpen()) {
            QSqlQuery query(db);
            query.prepare("SELECT COUNT(*), NVL(SUM(quantity_kg), 0), NVL(AVG(quantity_kg), 0), MAX(received_at) "
                          "FROM reception WHERE client_id = :id");
            query.bindValue(":id", clientId);
            if (query.exec() && query.next()) {
                metrics.lots = query.value(0).toInt();
                metrics.totalKg = query.value(1).toDouble();
                metrics.avgKg = query.value(2).toDouble();
                metrics.lastReception = query.value(3).toDate();
            }
        } else if (receptionTable) {
            for (int r = 0; r < receptionTable->rowCount(); ++r) {
                const int rowClientId = receptionTable->item(r, 5) ? receptionTable->item(r, 5)->text().toInt() : 0;
                if (rowClientId != clientId) {
                    continue;
                }

                ++metrics.lots;
                metrics.totalKg += receptionTable->item(r, 3) ? receptionTable->item(r, 3)->text().toDouble() : 0.0;
                const QDate receivedAt = parseReceptionDate(receptionTable->item(r, 2) ? receptionTable->item(r, 2)->text() : QString());
                if (receivedAt.isValid() && (!metrics.lastReception.isValid() || receivedAt > metrics.lastReception)) {
                    metrics.lastReception = receivedAt;
                }
            }
            if (metrics.lots > 0) {
                metrics.avgKg = metrics.totalKg / double(metrics.lots);
            }
        }

        if (metrics.lastReception.isValid()) {
            metrics.recencyDays = metrics.lastReception.daysTo(today);
        }

        return metrics;
    };

    auto clientNameById = [&](int id) {
        const int row = findClientRowById(id);
        if (row >= 0 && clientsTable->item(row, 1)) {
            return clientsTable->item(row, 1)->text();
        }
        return QString("Client %1").arg(id);
    };

    QDialog dlg(this);
    dlg.setWindowTitle("Tableau de bord comparatif client");
    dlg.setMinimumSize(880, 520);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    QLabel *title = new QLabel("Comparer deux clients sur activité et volumes");
    title->setStyleSheet("font-size:16px; font-weight:800; color:#153E39;");
    mainLayout->addWidget(title);

    QHBoxLayout *selectors = new QHBoxLayout();
    QLabel *labelA = new QLabel("Client A:");
    QComboBox *clientA = new QComboBox();
    QLabel *labelB = new QLabel("Client B:");
    QComboBox *clientB = new QComboBox();
    QPushButton *refreshBtn = new QPushButton("Comparer");
    refreshBtn->setProperty("role", "primary");
    refreshBtn->setFixedHeight(32);

    for (int r = 0; r < clientsTable->rowCount(); ++r) {
        const int id = clientsTable->item(r, 0) ? clientsTable->item(r, 0)->text().toInt() : 0;
        const QString name = clientsTable->item(r, 1) ? clientsTable->item(r, 1)->text() : QString("Client %1").arg(id);
        const QString label = QString("%1 - %2").arg(id).arg(name);
        clientA->addItem(label, id);
        clientB->addItem(label, id);
    }
    if (clientB->count() > 1) {
        clientB->setCurrentIndex(1);
    }

    selectors->addWidget(labelA);
    selectors->addWidget(clientA, 1);
    selectors->addSpacing(8);
    selectors->addWidget(labelB);
    selectors->addWidget(clientB, 1);
    selectors->addSpacing(8);
    selectors->addWidget(refreshBtn);
    mainLayout->addLayout(selectors);

    QTableWidget *compareTable = new QTableWidget(6, 3);
    compareTable->setHorizontalHeaderLabels({"Indicateur", "Client A", "Client B"});
    compareTable->verticalHeader()->setVisible(false);
    compareTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    compareTable->setSelectionMode(QAbstractItemView::NoSelection);
    compareTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    compareTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    compareTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    compareTable->setAlternatingRowColors(true);

    const QStringList metricsLabels = {
        "Nombre de lots",
        "Volume total (kg)",
        "Moyenne par lot (kg)",
        "Dernière réception",
        "Jours depuis dernière réception",
        "Niveau d'activité"
    };
    for (int i = 0; i < metricsLabels.size(); ++i) {
        compareTable->setItem(i, 0, new QTableWidgetItem(metricsLabels[i]));
    }
    mainLayout->addWidget(compareTable, 1);

    QLabel *insight = new QLabel();
    insight->setWordWrap(true);
    insight->setStyleSheet("background:#F4FAF7; border:1px solid #D3E7DD; border-radius:8px; padding:8px; font-weight:600;");
    mainLayout->addWidget(insight);

    auto buildActivityLevel = [](const ClientMetrics &m) {
        if (m.lots == 0) return QString("Dormant");
        if (m.recencyDays >= 0 && m.recencyDays <= 30 && m.lots >= 5) return QString("Très actif");
        if (m.recencyDays >= 0 && m.recencyDays <= 90) return QString("Actif");
        return QString("À relancer");
    };

    auto refreshComparison = [&]() {
        const int idA = clientA->currentData().toInt();
        const int idB = clientB->currentData().toInt();
        if (idA == idB) {
            QMessageBox::warning(&dlg, "Comparatif clients", "Choisissez deux clients différents.");
            return;
        }

        const ClientMetrics a = computeMetrics(idA);
        const ClientMetrics b = computeMetrics(idB);
        const QString nameA = clientNameById(idA);
        const QString nameB = clientNameById(idB);

        compareTable->setHorizontalHeaderLabels({"Indicateur", nameA, nameB});
        compareTable->item(0, 1) ? compareTable->item(0, 1)->setText(QString::number(a.lots)) : compareTable->setItem(0, 1, new QTableWidgetItem(QString::number(a.lots)));
        compareTable->item(0, 2) ? compareTable->item(0, 2)->setText(QString::number(b.lots)) : compareTable->setItem(0, 2, new QTableWidgetItem(QString::number(b.lots)));

        compareTable->item(1, 1) ? compareTable->item(1, 1)->setText(QString::number(a.totalKg, 'f', 2)) : compareTable->setItem(1, 1, new QTableWidgetItem(QString::number(a.totalKg, 'f', 2)));
        compareTable->item(1, 2) ? compareTable->item(1, 2)->setText(QString::number(b.totalKg, 'f', 2)) : compareTable->setItem(1, 2, new QTableWidgetItem(QString::number(b.totalKg, 'f', 2)));

        compareTable->item(2, 1) ? compareTable->item(2, 1)->setText(QString::number(a.avgKg, 'f', 2)) : compareTable->setItem(2, 1, new QTableWidgetItem(QString::number(a.avgKg, 'f', 2)));
        compareTable->item(2, 2) ? compareTable->item(2, 2)->setText(QString::number(b.avgKg, 'f', 2)) : compareTable->setItem(2, 2, new QTableWidgetItem(QString::number(b.avgKg, 'f', 2)));

        const QString lastA = a.lastReception.isValid() ? a.lastReception.toString("yyyy-MM-dd") : QString("Aucune");
        const QString lastB = b.lastReception.isValid() ? b.lastReception.toString("yyyy-MM-dd") : QString("Aucune");
        compareTable->item(3, 1) ? compareTable->item(3, 1)->setText(lastA) : compareTable->setItem(3, 1, new QTableWidgetItem(lastA));
        compareTable->item(3, 2) ? compareTable->item(3, 2)->setText(lastB) : compareTable->setItem(3, 2, new QTableWidgetItem(lastB));

        const QString recA = (a.recencyDays >= 0) ? QString::number(a.recencyDays) : QString("N/A");
        const QString recB = (b.recencyDays >= 0) ? QString::number(b.recencyDays) : QString("N/A");
        compareTable->item(4, 1) ? compareTable->item(4, 1)->setText(recA) : compareTable->setItem(4, 1, new QTableWidgetItem(recA));
        compareTable->item(4, 2) ? compareTable->item(4, 2)->setText(recB) : compareTable->setItem(4, 2, new QTableWidgetItem(recB));

        const QString actA = buildActivityLevel(a);
        const QString actB = buildActivityLevel(b);
        compareTable->item(5, 1) ? compareTable->item(5, 1)->setText(actA) : compareTable->setItem(5, 1, new QTableWidgetItem(actA));
        compareTable->item(5, 2) ? compareTable->item(5, 2)->setText(actB) : compareTable->setItem(5, 2, new QTableWidgetItem(actB));

        int scoreA = 0;
        int scoreB = 0;
        if (a.lots >= b.lots) ++scoreA; else ++scoreB;
        if (a.totalKg >= b.totalKg) ++scoreA; else ++scoreB;
        if (a.avgKg >= b.avgKg) ++scoreA; else ++scoreB;
        if (a.recencyDays >= 0 && b.recencyDays >= 0) {
            if (a.recencyDays <= b.recencyDays) ++scoreA; else ++scoreB;
        }

        QString winner;
        if (scoreA == scoreB) {
            winner = QString("Équilibre: %1 et %2 ont des performances similaires.").arg(nameA, nameB);
        } else if (scoreA > scoreB) {
            winner = QString("Avantage: %1 (%2 critères gagnants sur 4). ").arg(nameA).arg(scoreA);
            winner += QString("%1 est plus performant actuellement.").arg(nameA);
        } else {
            winner = QString("Avantage: %1 (%2 critères gagnants sur 4). ").arg(nameB).arg(scoreB);
            winner += QString("%1 est plus performant actuellement.").arg(nameB);
        }
        insight->setText(winner);
    };

    connect(refreshBtn, &QPushButton::clicked, &dlg, refreshComparison);
    connect(clientA, qOverload<int>(&QComboBox::currentIndexChanged), &dlg, [&](int) { refreshComparison(); });
    connect(clientB, qOverload<int>(&QComboBox::currentIndexChanged), &dlg, [&](int) { refreshComparison(); });

    refreshComparison();
    dlg.exec();
}

void MainWindow::showClientRfmMatrix()
{
    if (!clientsTable || clientsTable->rowCount() == 0) {
        QMessageBox::warning(this, "Matrice RFM", "Aucun client à analyser.");
        return;
    }

    auto parseReceptionDate = [](const QString &value) {
        const QString text = value.trimmed();
        if (text.isEmpty()) {
            return QDate();
        }
        QDate d = QDate::fromString(text, "yyyy-MM-dd");
        if (!d.isValid()) d = QDate::fromString(text, "dd/MM/yyyy");
        if (!d.isValid()) d = QDate::fromString(text, "yyyy/MM/dd");
        return d;
    };

    struct RfmRow {
        int id = 0;
        QString name;
        int recencyDays = 9999;
        int frequency = 0;
        double monetary = 0.0;
        int rScore = 1;
        int fScore = 1;
        int mScore = 1;
        QString segment;
    };

    auto computeClientBase = [&](int clientId) {
        RfmRow row;
        row.id = clientId;
        const int clientRow = findClientRowById(clientId);
        row.name = (clientRow >= 0 && clientsTable->item(clientRow, 1))
                       ? clientsTable->item(clientRow, 1)->text()
                       : QString("Client %1").arg(clientId);

        QDate lastReception;
        if (oracleActive && db.isOpen()) {
            QSqlQuery query(db);
            query.prepare("SELECT COUNT(*), NVL(SUM(quantity_kg), 0), MAX(received_at) "
                          "FROM reception WHERE client_id = :id");
            query.bindValue(":id", clientId);
            if (query.exec() && query.next()) {
                row.frequency = query.value(0).toInt();
                row.monetary = query.value(1).toDouble();
                lastReception = query.value(2).toDate();
            }
        } else if (receptionTable) {
            for (int r = 0; r < receptionTable->rowCount(); ++r) {
                const int rowClientId = receptionTable->item(r, 5) ? receptionTable->item(r, 5)->text().toInt() : 0;
                if (rowClientId != clientId) {
                    continue;
                }

                ++row.frequency;
                row.monetary += receptionTable->item(r, 3) ? receptionTable->item(r, 3)->text().toDouble() : 0.0;
                const QDate receivedAt = parseReceptionDate(receptionTable->item(r, 2) ? receptionTable->item(r, 2)->text() : QString());
                if (receivedAt.isValid() && (!lastReception.isValid() || receivedAt > lastReception)) {
                    lastReception = receivedAt;
                }
            }
        }

        if (lastReception.isValid()) {
            row.recencyDays = lastReception.daysTo(QDate::currentDate());
        }
        return row;
    };

    QList<RfmRow> rows;
    QVector<double> recencyVals;
    QVector<double> freqVals;
    QVector<double> monetaryVals;

    for (int r = 0; r < clientsTable->rowCount(); ++r) {
        const int clientId = clientsTable->item(r, 0) ? clientsTable->item(r, 0)->text().toInt() : 0;
        if (clientId <= 0) {
            continue;
        }

        RfmRow row = computeClientBase(clientId);
        rows.append(row);
        recencyVals.append(double(row.recencyDays));
        freqVals.append(double(row.frequency));
        monetaryVals.append(row.monetary);
    }

    if (rows.isEmpty()) {
        QMessageBox::warning(this, "Matrice RFM", "Impossible de calculer la matrice RFM.");
        return;
    }

    auto scoreByTercile = [](const QVector<double> &allValues, double current, bool higherIsBetter) {
        if (allValues.isEmpty()) {
            return 1;
        }

        int lowerOrEqual = 0;
        for (double v : allValues) {
            if (v <= current) {
                ++lowerOrEqual;
            }
        }
        const double pct = (double(lowerOrEqual) * 100.0) / double(allValues.size());

        if (higherIsBetter) {
            if (pct <= 33.0) return 1;
            if (pct <= 66.0) return 2;
            return 3;
        }

        if (pct <= 33.0) return 3;
        if (pct <= 66.0) return 2;
        return 1;
    };

    auto segmentFromRf = [](int rScore, int fScore) {
        if (rScore == 3 && fScore == 3) return QString("Champions");
        if (rScore == 3 && fScore == 2) return QString("Loyaux");
        if (rScore == 3 && fScore == 1) return QString("Nouveaux prometteurs");
        if (rScore == 2 && fScore == 3) return QString("Fréquents à stimuler");
        if (rScore == 2 && fScore == 2) return QString("Stables");
        if (rScore == 2 && fScore == 1) return QString("Occasionnels");
        if (rScore == 1 && fScore == 3) return QString("À réactiver prioritaire");
        if (rScore == 1 && fScore == 2) return QString("Endormis");
        return QString("Perdus");
    };

    QMap<QString, int> segmentCount;
    int matrix[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    for (RfmRow &row : rows) {
        row.rScore = scoreByTercile(recencyVals, double(row.recencyDays), false);
        row.fScore = scoreByTercile(freqVals, double(row.frequency), true);
        row.mScore = scoreByTercile(monetaryVals, row.monetary, true);
        row.segment = segmentFromRf(row.rScore, row.fScore);

        matrix[3 - row.rScore][row.fScore - 1]++;
        segmentCount[row.segment]++;
    }

    std::sort(rows.begin(), rows.end(), [](const RfmRow &a, const RfmRow &b) {
        const int scoreA = a.rScore + a.fScore + a.mScore;
        const int scoreB = b.rScore + b.fScore + b.mScore;
        if (scoreA != scoreB) {
            return scoreA > scoreB;
        }
        return a.monetary > b.monetary;
    });

    QDialog dlg(this);
    dlg.setWindowTitle("Matrice RFM clients");
    dlg.setMinimumSize(980, 620);

    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    QLabel *title = new QLabel("Segmentation RFM (Récence, Fréquence, Montant)");
    title->setStyleSheet("font-size:16px; font-weight:800; color:#153E39;");
    mainLayout->addWidget(title);

    QLabel *subtitle = new QLabel("R=plus récent est meilleur, F=plus de lots est meilleur, M=plus de kg est meilleur.");
    subtitle->setStyleSheet("color:#2F5D56; font-weight:600;");
    mainLayout->addWidget(subtitle);

    QTableWidget *matrixTable = new QTableWidget(3, 3);
    matrixTable->setHorizontalHeaderLabels({"F1 Faible", "F2 Moyen", "F3 Élevé"});
    matrixTable->setVerticalHeaderLabels({"R3 Récent", "R2 Intermédiaire", "R1 Ancien"});
    matrixTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    matrixTable->setSelectionMode(QAbstractItemView::NoSelection);
    matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    matrixTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    matrixTable->setFixedHeight(190);

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(matrix[r][c]));
            item->setTextAlignment(Qt::AlignCenter);
            matrixTable->setItem(r, c, item);
        }
    }
    mainLayout->addWidget(matrixTable);

    QStringList segmentLines;
    for (auto it = segmentCount.constBegin(); it != segmentCount.constEnd(); ++it) {
        segmentLines << QString("%1: %2").arg(it.key()).arg(it.value());
    }
    QLabel *segmentSummary = new QLabel(segmentLines.isEmpty() ? QString("Aucun segment calculé") : segmentLines.join("   |   "));
    segmentSummary->setWordWrap(true);
    segmentSummary->setStyleSheet("background:#F4FAF7; border:1px solid #D3E7DD; border-radius:8px; padding:8px; font-weight:600;");
    mainLayout->addWidget(segmentSummary);

    QTableWidget *details = new QTableWidget(rows.size(), 8);
    details->setHorizontalHeaderLabels({"Client", "Récence (j)", "Fréquence", "Montant (kg)", "R", "F", "M", "Segment"});
    details->setEditTriggers(QAbstractItemView::NoEditTriggers);
    details->setAlternatingRowColors(true);
    details->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    details->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    details->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    details->verticalHeader()->setVisible(false);

    for (int i = 0; i < rows.size(); ++i) {
        const RfmRow &row = rows[i];
        details->setItem(i, 0, new QTableWidgetItem(QString("%1 - %2").arg(row.id).arg(row.name)));
        details->setItem(i, 1, new QTableWidgetItem(row.recencyDays >= 0 ? QString::number(row.recencyDays) : QString("N/A")));
        details->setItem(i, 2, new QTableWidgetItem(QString::number(row.frequency)));
        details->setItem(i, 3, new QTableWidgetItem(QString::number(row.monetary, 'f', 2)));
        details->setItem(i, 4, new QTableWidgetItem(QString::number(row.rScore)));
        details->setItem(i, 5, new QTableWidgetItem(QString::number(row.fScore)));
        details->setItem(i, 6, new QTableWidgetItem(QString::number(row.mScore)));
        details->setItem(i, 7, new QTableWidgetItem(row.segment));
    }

    mainLayout->addWidget(details, 1);
    dlg.exec();
}

void MainWindow::updateCiterneStatistics()
{
    if (!citernesTable || !citerneStatsTotalValue || !citerneStatsDonutChartLabel) {
        return;
    }

    auto qualityToScore = [](const QString &value) {
        bool ok = false;
        const double numeric = value.toDouble(&ok);
        if (ok) {
            return numeric * 100.0;
        }

        const QString v = value.trimmed().toUpper();
        if (v == "EXTRA") return 95.0;
        if (v == "PREMIUM") return 90.0;
        if (v == "A") return 85.0;
        if (v == "B") return 75.0;
        if (v == "C") return 65.0;
        if (v == "FAIBLE") return 50.0;
        return 70.0;
    };

    int total = 0;
    double totalCapacity = 0.0;
    double totalVolume = 0.0;
    double totalTemp = 0.0;
    double totalQuality = 0.0;

    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        if (citernesTable->isRowHidden(r)) {
            continue;
        }

        const double capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text().toDouble() : 0.0;
        const double volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text().toDouble() : 0.0;
        const double temperature = citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toDouble() : 0.0;
        const QString qualityText = citernesTable->item(r, 4) ? citernesTable->item(r, 4)->text() : QString();

        totalCapacity += capacity;
        totalVolume += volume;
        totalTemp += temperature;
        totalQuality += qualityToScore(qualityText);
        ++total;
    }

    int critical = 0;
    int warning = 0;
    int normal = 0;
    buildCiterneAlerts(lowLevelThreshold, &critical, &warning, &normal);

    const double fillRate = (totalCapacity > 0.0) ? ((totalVolume / totalCapacity) * 100.0) : 0.0;
    const double averageTemp = (total > 0) ? (totalTemp / total) : 0.0;
    const double averageQuality = (total > 0) ? (totalQuality / total) : 0.0;

    citerneStatsTotalValue->setText(QString::number(total));
    citerneStatsCapacityValue->setText(QString::number(totalCapacity, 'f', 0));
    citerneStatsVolumeValue->setText(QString::number(totalVolume, 'f', 0));
    citerneStatsFillRateValue->setText(QString::number(fillRate, 'f', 1));
    citerneStatsTempValue->setText(QString::number(averageTemp, 'f', 1) + " °C");
    citerneStatsCriticalValue->setText(QString::number(critical));
    citerneStatsTopQualityValue->setText(QString::number(averageQuality, 'f', 1) + " / 100");

    if (citerneStatsBreakdownValue) {
        const QString breakdown = QString("- Critique: %1\n- Warning: %2\n- Normal: %3")
                                      .arg(critical)
                                      .arg(warning)
                                      .arg(normal);
        citerneStatsBreakdownValue->setText(breakdown);
    }

    const QList<int> donutValues = {critical, warning, normal};
    const QList<QColor> donutColors = {
        QColor("#FF3A3A"),
        QColor("#FF7A00"),
        QColor("#38C86A")
    };
    const QSize chartSize = citerneStatsDonutChartLabel->size().isValid() ? citerneStatsDonutChartLabel->size() : QSize(280, 240);
    citerneStatsDonutChartLabel->setPixmap(buildDonutChartPixmap(chartSize,
                                                                  donutValues,
                                                                  donutColors,
                                                                  QString::number(total),
                                                                  QString("citernes")));

    if (citerneStatsLegendCriticalValue) {
        citerneStatsLegendCriticalValue->setText(formatLegendLine("Critique", critical, total));
    }
    if (citerneStatsLegendWarningValue) {
        citerneStatsLegendWarningValue->setText(formatLegendLine("Warning", warning, total));
    }
    if (citerneStatsLegendNormalValue) {
        citerneStatsLegendNormalValue->setText(formatLegendLine("Normal", normal, total));
    }
}

// ============================================================================
// SLOTS - Clients CRUD etc.
// ============================================================================
void MainWindow::showAddClientDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Ajouter un client");
    dlg.setModal(true);
    dlg.setMinimumSize(480,360);

    QFormLayout *form = new QFormLayout();
    QLineEdit *name = new QLineEdit();
    QLineEdit *email = new QLineEdit();
    QLineEdit *phone = new QLineEdit();
    QLineEdit *address = new QLineEdit();
    name->setMaxLength(80);
    email->setMaxLength(120);
    phone->setMaxLength(20);
    address->setMaxLength(180);
    phone->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\+?[0-9 ]{0,20}$"), phone));
    QDateEdit *regDate = new QDateEdit(QDate::currentDate());
    regDate->setCalendarPopup(true);
    form->addRow("Nom:", name);
    form->addRow("Email:", email);
    form->addRow("Téléphone:", phone);
    form->addRow("Adresse:", address);
    form->addRow("Inscrit le:", regDate);

    QVBoxLayout *v = new QVBoxLayout(&dlg);
    v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&]() {
        const QString n = name->text().trimmed();
        const QString e = email->text().trimmed();
        const QString p = phone->text().trimmed();
        const QString a = address->text().trimmed();
        static const QRegularExpression emailRx("^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$", QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression phoneRx("^\\+?[0-9 ]{8,20}$");

        if (n.size() < 2) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le nom doit contenir au moins 2 caractères.");
            return;
        }
        if (!emailRx.match(e).hasMatch()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Email invalide.");
            return;
        }
        if (!phoneRx.match(p).hasMatch()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Téléphone invalide (8 à 20 chiffres, espaces autorisés).");
            return;
        }
        if (a.size() < 5) {
            QMessageBox::warning(&dlg, "Saisie invalide", "L'adresse doit contenir au moins 5 caractères.");
            return;
        }
        dlg.accept();
    });
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    v->addWidget(bb);

    if (dlg.exec() == QDialog::Accepted) {
        if (oracleActive && db.isOpen()) {
            Client client;
            client.setName(name->text());
            client.setEmail(email->text());
            client.setPhone(phone->text());
            client.setAddress(address->text());
            client.setCreatedAt(regDate->date());
            client.setStatus("Actif");

            QString errorMessage;
            if (!client.ajouter(db, &errorMessage)) {
                QMessageBox::critical(this, "Oracle", "Ajout client échoué:\n" + errorMessage);
                return;
            }
            loadClientsFromOracle();
            return;
        }

        int newRow = clientsTable->rowCount();
        clientsTable->insertRow(newRow);
        int maxId = 0;
        for (int r=0;r<clientsTable->rowCount();++r) { QTableWidgetItem *it = clientsTable->item(r,0); if (it) maxId = qMax(maxId, it->text().toInt()); }
        int id = maxId + 1;
        clientsTable->setItem(newRow,0, new QTableWidgetItem(QString::number(id)));
        clientsTable->setItem(newRow,1, new QTableWidgetItem(name->text()));
        clientsTable->setItem(newRow,2, new QTableWidgetItem(email->text()));
        clientsTable->setItem(newRow,3, new QTableWidgetItem(phone->text()));
        clientsTable->setItem(newRow,4, new QTableWidgetItem(address->text()));
        clientsTable->setItem(newRow,5, new QTableWidgetItem(regDate->date().toString("yyyy-MM-dd")));
        clientsTable->setItem(newRow,6, new QTableWidgetItem("Actif"));
        clientsTable->setCellWidget(newRow,7, createClientActionsWidget(id));
    }
}

void MainWindow::editSelectedClient()
{
    QObject *s = sender();
    int targetRow = -1;
    if (s && s->property("client_id").isValid()) {
        int clientId = s->property("client_id").toInt();
        targetRow = findClientRowById(clientId);
    } else {
        targetRow = clientsTable->currentRow();
    }
    if (targetRow < 0) { QMessageBox::warning(this,"Avertissement","Sélectionnez un client à éditer."); return; }

    QString name = clientsTable->item(targetRow,1)->text();
    QString email = clientsTable->item(targetRow,2)->text();
    QString phone = clientsTable->item(targetRow,3)->text();
    QString address = clientsTable->item(targetRow,4)->text();
    QDate reg = QDate::fromString(clientsTable->item(targetRow,5)->text(),"yyyy-MM-dd");
    int clientId = clientsTable->item(targetRow,0)->text().toInt();

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Éditer client %1").arg(name));
    QFormLayout *form = new QFormLayout();
    QLineEdit *nameE = new QLineEdit(name);
    QLineEdit *emailE = new QLineEdit(email);
    QLineEdit *phoneE = new QLineEdit(phone);
    QLineEdit *addressE = new QLineEdit(address);
    nameE->setMaxLength(80);
    emailE->setMaxLength(120);
    phoneE->setMaxLength(20);
    addressE->setMaxLength(180);
    phoneE->setValidator(new QRegularExpressionValidator(QRegularExpression("^\\+?[0-9 ]{0,20}$"), phoneE));
    QDateEdit *regE = new QDateEdit(reg); regE->setCalendarPopup(true);
    form->addRow("Nom:", nameE); form->addRow("Email:", emailE); form->addRow("Téléphone:", phoneE); form->addRow("Adresse:", addressE); form->addRow("Inscrit le:", regE);
    QVBoxLayout *v = new QVBoxLayout(&dlg); v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb,&QDialogButtonBox::accepted,&dlg,[&]() {
        const QString n = nameE->text().trimmed();
        const QString e = emailE->text().trimmed();
        const QString p = phoneE->text().trimmed();
        const QString a = addressE->text().trimmed();
        static const QRegularExpression emailRx("^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$", QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression phoneRx("^\\+?[0-9 ]{8,20}$");

        if (n.size() < 2) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le nom doit contenir au moins 2 caractères.");
            return;
        }
        if (!emailRx.match(e).hasMatch()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Email invalide.");
            return;
        }
        if (!phoneRx.match(p).hasMatch()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Téléphone invalide (8 à 20 chiffres, espaces autorisés).");
            return;
        }
        if (a.size() < 5) {
            QMessageBox::warning(&dlg, "Saisie invalide", "L'adresse doit contenir au moins 5 caractères.");
            return;
        }
        dlg.accept();
    });
    connect(bb,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    v->addWidget(bb);
    if (dlg.exec() == QDialog::Accepted) {
        if (oracleActive && db.isOpen()) {
            Client client;
            client.setId(clientId);
            client.setName(nameE->text());
            client.setEmail(emailE->text());
            client.setPhone(phoneE->text());
            client.setAddress(addressE->text());
            client.setCreatedAt(regE->date());
            client.setStatus(clientsTable->item(targetRow,6) ? clientsTable->item(targetRow,6)->text() : "Actif");

            QString errorMessage;
            if (!client.modifier(db, &errorMessage)) {
                QMessageBox::critical(this, "Oracle", "Modification client échouée:\n" + errorMessage);
                return;
            }
            loadClientsFromOracle();
            QMessageBox::information(this,"Succès","Client mis à jour.");
            return;
        }

        clientsTable->item(targetRow,1)->setText(nameE->text());
        clientsTable->item(targetRow,2)->setText(emailE->text());
        clientsTable->item(targetRow,3)->setText(phoneE->text());
        clientsTable->item(targetRow,4)->setText(addressE->text());
        clientsTable->item(targetRow,5)->setText(regE->date().toString("yyyy-MM-dd"));
        QMessageBox::information(this,"Succès","Client mis à jour.");
    }
}

void MainWindow::viewSelectedClient()
{
    QObject *s = sender();
    int targetRow = -1;
    if (s && s->property("client_id").isValid()) {
        int id = s->property("client_id").toInt();
        targetRow = findClientRowById(id);
    } else {
        targetRow = clientsTable->currentRow();
    }
    if (targetRow < 0) { QMessageBox::warning(this,"Avertissement","Sélectionnez un client à afficher."); return; }
    QString info;
    for (int c=0;c<6;++c) { QTableWidgetItem *it = clientsTable->item(targetRow,c); if (it) info += it->text() + "\n"; }
    QMessageBox::information(this,"Détails client", info);
}

void MainWindow::deleteSelectedClient()
{
    QObject *s = sender();
    int targetRow = -1;
    if (s && s->property("client_id").isValid()) {
        int id = s->property("client_id").toInt();
        targetRow = findClientRowById(id);
    } else {
        targetRow = clientsTable->currentRow();
    }
    if (targetRow < 0) { QMessageBox::warning(this,"Avertissement","Sélectionnez un client à supprimer."); return; }
    const int id = clientsTable->item(targetRow,0)->text().toInt();
    QMessageBox::StandardButton rep = QMessageBox::question(this,"Confirmer suppression", QString("Supprimer le client ID %1 ?").arg(id), QMessageBox::Yes|QMessageBox::No);
    if (rep == QMessageBox::Yes) {
        if (oracleActive && db.isOpen()) {
            QString errorMessage;
            if (!Client::supprimer(db, id, &errorMessage)) {
                QMessageBox::critical(this, "Oracle", "Suppression client échouée:\n" + errorMessage);
                return;
            }
            loadClientsFromOracle();
        } else {
            clientsTable->removeRow(targetRow);
        }
        QMessageBox::information(this,"Supprimé","Client supprimé.");
    }
}

void MainWindow::searchClients(const QString &text)
{
    for (int r=0;r<clientsTable->rowCount();++r) {
        bool match=false;
        for (int c=1;c<=4;++c) {
            QTableWidgetItem *it = clientsTable->item(r,c);
            if (it && it->text().contains(text, Qt::CaseInsensitive)) { match=true; break; }
        }
        clientsTable->setRowHidden(r, !match);
    }
}

void MainWindow::sortClients()
{
    clientsTable->sortItems(1, Qt::AscendingOrder);
    QMessageBox::information(this,"Tri","Clients triés par nom.");
}

void MainWindow::exportClients()
{
    if (!clientsTable || clientsTable->rowCount() == 0) {
        QMessageBox::warning(this, "Exporter", "Aucune donnée client à exporter.");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          "Exporter les clients en PDF",
                                                          QDir::homePath() + "/clients.pdf",
                                                          "PDF Files (*.pdf)");
    if (filePath.isEmpty()) {
        return;
    }

    QString html;
    html += "<h2>Liste des clients</h2>";
    html += QString("<p>Généré le %1</p>").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    html += "<table border='1' cellspacing='0' cellpadding='4'>";
    html += "<tr>"
            "<th>ID</th><th>Nom</th><th>Email</th><th>Téléphone</th>"
            "<th>Adresse</th><th>Inscrit le</th><th>Statut</th>"
            "</tr>";

    for (int r = 0; r < clientsTable->rowCount(); ++r) {
        if (clientsTable->isRowHidden(r)) {
            continue;
        }

        html += "<tr>";
        for (int c = 0; c < 7; ++c) {
            const QTableWidgetItem *item = clientsTable->item(r, c);
            html += QString("<td>%1</td>").arg(item ? item->text().toHtmlEscaped() : QString());
        }
        html += "</tr>";
    }
    html += "</table>";

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(96);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);

    QMessageBox::information(this, "Exporter", "Export PDF clients terminé:\n" + filePath);
}

// ============================================================================
// CITERNE SLOTS (basic versions)
// ============================================================================
void MainWindow::showAddCiterneDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Ajouter une Citerne");
    dlg.setModal(true);
    dlg.setMinimumSize(480,360);

    QFormLayout *form = new QFormLayout();
    QLineEdit *cap = new QLineEdit(); QLineEdit *vol = new QLineEdit();
    QLineEdit *qual = new QLineEdit(); QLineEdit *temp = new QLineEdit();
    auto *numVal = new QDoubleValidator(0.0, 1000000000.0, 2, &dlg);
    numVal->setNotation(QDoubleValidator::StandardNotation);
    auto *tempVal = new QDoubleValidator(-50.0, 200.0, 2, &dlg);
    tempVal->setNotation(QDoubleValidator::StandardNotation);
    cap->setValidator(numVal);
    vol->setValidator(numVal);
    temp->setValidator(tempVal);
    qual->setMaxLength(40);
    QDateEdit *last = new QDateEdit(QDate::currentDate()); last->setCalendarPopup(true);
    form->addRow("Capacité (L):", cap); form->addRow("Volume (L):", vol); form->addRow("Qualité:", qual); form->addRow("Temp (°C):", temp); form->addRow("Dernier remplissage:", last);
    QVBoxLayout *v = new QVBoxLayout(&dlg); v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb,&QDialogButtonBox::accepted,&dlg,[&]() {
        bool okCap = false;
        bool okVol = false;
        bool okTemp = false;
        const double capV = cap->text().trimmed().toDouble(&okCap);
        const double volV = vol->text().trimmed().toDouble(&okVol);
        temp->text().trimmed().toDouble(&okTemp);
        if (!okCap || capV <= 0.0) {
            QMessageBox::warning(&dlg, "Saisie invalide", "La capacité doit être un nombre > 0.");
            return;
        }
        if (!okVol || volV < 0.0) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le volume doit être un nombre >= 0.");
            return;
        }
        if (volV > capV) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le volume ne peut pas dépasser la capacité.");
            return;
        }
        if (!okTemp) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Température invalide.");
            return;
        }
        if (qual->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "La qualité est obligatoire.");
            return;
        }
        dlg.accept();
    });
    connect(bb,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    v->addWidget(bb);

    if (dlg.exec() == QDialog::Accepted) {
        if (oracleActive && db.isOpen()) {
            Citerne citerne;
            citerne.setCapaciteL(cap->text().toDouble());
            citerne.setVolumeL(vol->text().toDouble());
            citerne.setQualite(qual->text());
            citerne.setTemperatureC(temp->text().toDouble());
            citerne.setDernierRemplissage(last->date());

            QString errorMessage;
            if (!citerne.ajouter(db, &errorMessage)) {
                QMessageBox::critical(this, "Oracle", "Ajout citerne échoué:\n" + errorMessage);
                return;
            }
            loadCiternesFromOracle();
            return;
        }

        int r = citernesTable->rowCount(); citernesTable->insertRow(r);
        int maxId=0; for (int i=0;i<citernesTable->rowCount();++i){ QTableWidgetItem *it=citernesTable->item(i,0); if(it) maxId=qMax(maxId,it->text().toInt()); }
        int id = maxId+1;
        citernesTable->setItem(r,0,new QTableWidgetItem(QString::number(id)));
        citernesTable->setItem(r,1,new QTableWidgetItem(cap->text()));
        citernesTable->setItem(r,2,new QTableWidgetItem(vol->text()));
        int pct = 0; if (!cap->text().isEmpty()) pct = int( (vol->text().toDouble()/cap->text().toDouble())*100.0 );
        citernesTable->setCellWidget(r,3, createProgressWidget(pct));
        citernesTable->setItem(r,4,new QTableWidgetItem(qual->text()));
        citernesTable->setItem(r,5,new QTableWidgetItem(temp->text()));
        citernesTable->setItem(r,6,new QTableWidgetItem(last->date().toString("yyyy-MM-dd")));
        citernesTable->setCellWidget(r,7, createCiterneActionsWidget(id));
    }
}

void MainWindow::editSelectedCiterne()
{
    QObject *s = sender();
    int row = -1;
    if (s && s->property("citerne_id").isValid()) {
        row = findCiterneRowById(s->property("citerne_id").toInt());
    } else {
        row = citernesTable->currentRow();
    }
    if (row<0) { QMessageBox::warning(this,"Avertissement","Sélectionnez une citerne."); return; }

    const int id = citernesTable->item(row,0)->text().toInt();
    const QString cap0 = citernesTable->item(row,1)->text();
    const QString vol0 = citernesTable->item(row,2)->text();
    const QString qual0 = citernesTable->item(row,4)->text();
    const QString temp0 = citernesTable->item(row,5)->text();
    const QDate date0 = QDate::fromString(citernesTable->item(row,6)->text(), "yyyy-MM-dd");

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Éditer citerne %1").arg(id));
    QFormLayout *form = new QFormLayout();
    QLineEdit *cap = new QLineEdit(cap0);
    QLineEdit *vol = new QLineEdit(vol0);
    QLineEdit *qual = new QLineEdit(qual0);
    QLineEdit *temp = new QLineEdit(temp0);
    auto *numVal = new QDoubleValidator(0.0, 1000000000.0, 2, &dlg);
    numVal->setNotation(QDoubleValidator::StandardNotation);
    auto *tempVal = new QDoubleValidator(-50.0, 200.0, 2, &dlg);
    tempVal->setNotation(QDoubleValidator::StandardNotation);
    cap->setValidator(numVal);
    vol->setValidator(numVal);
    temp->setValidator(tempVal);
    qual->setMaxLength(40);
    QDateEdit *last = new QDateEdit(date0.isValid() ? date0 : QDate::currentDate());
    last->setCalendarPopup(true);
    form->addRow("Capacité (L):", cap);
    form->addRow("Volume (L):", vol);
    form->addRow("Qualité:", qual);
    form->addRow("Temp (°C):", temp);
    form->addRow("Dernier remplissage:", last);

    QVBoxLayout *v = new QVBoxLayout(&dlg);
    v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb,&QDialogButtonBox::accepted,&dlg,[&]() {
        bool okCap = false;
        bool okVol = false;
        bool okTemp = false;
        const double capV = cap->text().trimmed().toDouble(&okCap);
        const double volV = vol->text().trimmed().toDouble(&okVol);
        temp->text().trimmed().toDouble(&okTemp);
        if (!okCap || capV <= 0.0) {
            QMessageBox::warning(&dlg, "Saisie invalide", "La capacité doit être un nombre > 0.");
            return;
        }
        if (!okVol || volV < 0.0) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le volume doit être un nombre >= 0.");
            return;
        }
        if (volV > capV) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le volume ne peut pas dépasser la capacité.");
            return;
        }
        if (!okTemp) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Température invalide.");
            return;
        }
        if (qual->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "La qualité est obligatoire.");
            return;
        }
        dlg.accept();
    });
    connect(bb,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    v->addWidget(bb);

    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    if (oracleActive && db.isOpen()) {
        Citerne citerne;
        citerne.setId(id);
        citerne.setCapaciteL(cap->text().toDouble());
        citerne.setVolumeL(vol->text().toDouble());
        citerne.setQualite(qual->text());
        citerne.setTemperatureC(temp->text().toDouble());
        citerne.setDernierRemplissage(last->date());

        QString errorMessage;
        if (!citerne.modifier(db, &errorMessage)) {
            QMessageBox::critical(this, "Oracle", "Modification citerne échouée:\n" + errorMessage);
            return;
        }
        loadCiternesFromOracle();
        return;
    }

    const double capV = cap->text().toDouble();
    const double volV = vol->text().toDouble();
    citernesTable->item(row,1)->setText(QString::number(capV,'f',2));
    citernesTable->item(row,2)->setText(QString::number(volV,'f',2));
    citernesTable->setCellWidget(row,3, createProgressWidget(capV > 0 ? int((volV / capV) * 100.0) : 0));
    citernesTable->item(row,4)->setText(qual->text());
    citernesTable->item(row,5)->setText(QString::number(temp->text().toDouble(),'f',1));
    citernesTable->item(row,6)->setText(last->date().toString("yyyy-MM-dd"));
}

void MainWindow::deleteSelectedCiterne()
{
    int row = citernesTable->currentRow();
    if (row<0) { QMessageBox::warning(this,"Avertissement","Sélectionnez une citerne."); return; }
    QMessageBox::StandardButton rep = QMessageBox::question(this,"Confirmer","Supprimer la citerne ?", QMessageBox::Yes|QMessageBox::No);
    if (rep==QMessageBox::Yes) {
        if (oracleActive && db.isOpen()) {
            const int id = citernesTable->item(row,0)->text().toInt();
            QString errorMessage;
            if (!Citerne::supprimer(db, id, &errorMessage)) {
                QMessageBox::critical(this, "Oracle", "Suppression citerne échouée:\n" + errorMessage);
                return;
            }
            loadCiternesFromOracle();
        } else {
            citernesTable->removeRow(row);
        }
        QMessageBox::information(this,"Supprimé","Citerne supprimée.");
    }
}

void MainWindow::onFillButtonClicked()
{
    QObject *s=sender();
    if(!s) return;
    int id = s->property("citerne_id").toInt();
    int row = findCiterneRowById(id);
    if (row<0) return;
    bool ok=false;
    double add = QInputDialog::getDouble(this,"Remplir","Volume à ajouter (L):",100.0,0.0,1e9,2,&ok);
    if(!ok) return;
    if (add <= 0.0) {
        QMessageBox::warning(this, "Saisie invalide", "Le volume à ajouter doit être supérieur à 0.");
        return;
    }
    double cap = citernesTable->item(row,1)->text().toDouble();
    double vol = citernesTable->item(row,2)->text().toDouble();
    vol += add; if (vol>cap) vol=cap;

    if (oracleActive && db.isOpen()) {
        QString errorMessage;
        if (!Citerne::mettreAJourVolume(db, id, vol, &errorMessage)) {
            QMessageBox::critical(this, "Oracle", "Remplissage échoué:\n" + errorMessage);
            return;
        }
    }

    citernesTable->item(row,2)->setText(QString::number(vol,'f',2));
    int pct = (cap>0)?int((vol/cap)*100.0):0;
    citernesTable->setCellWidget(row,3, createProgressWidget(pct));
}

void MainWindow::onDrainButtonClicked()
{
    QObject *s=sender();
    if(!s) return;
    int id = s->property("citerne_id").toInt();
    int row = findCiterneRowById(id);
    if (row<0) return;
    bool ok=false;
    double rem = QInputDialog::getDouble(this,"Vider","Volume à retirer (L):",100.0,0.0,1e9,2,&ok);
    if(!ok) return;
    if (rem <= 0.0) {
        QMessageBox::warning(this, "Saisie invalide", "Le volume à retirer doit être supérieur à 0.");
        return;
    }
    double cap = citernesTable->item(row,1)->text().toDouble();
    double vol = citernesTable->item(row,2)->text().toDouble();
    vol -= rem; if(vol<0) vol=0;

    if (oracleActive && db.isOpen()) {
        QString errorMessage;
        if (!Citerne::mettreAJourVolume(db, id, vol, &errorMessage)) {
            QMessageBox::critical(this, "Oracle", "Vidage échoué:\n" + errorMessage);
            return;
        }
    }

    citernesTable->item(row,2)->setText(QString::number(vol,'f',2));
    int pct = (cap>0)?int((vol/cap)*100.0):0;
    citernesTable->setCellWidget(row,3, createProgressWidget(pct));
}

void MainWindow::searchCiternes(const QString &text)
{
    for (int r=0;r<citernesTable->rowCount();++r) {
        bool match=false;
        for (int c=0;c<citernesTable->columnCount()-1;++c) {
            QTableWidgetItem *it = citernesTable->item(r,c);
            if (it && it->text().contains(text, Qt::CaseInsensitive)) { match=true; break; }
        }
        citernesTable->setRowHidden(r, !match);
    }
}

void MainWindow::sortCiternes()
{
    citernesTable->sortItems(2, Qt::DescendingOrder);
    QMessageBox::information(this,"Tri","Citernes triées par volume.");
}

void MainWindow::showCiterneStatistics()
{
    updateCiterneStatistics();
    if (citerneStatisticsPage) {
        stackedWidget->setCurrentWidget(citerneStatisticsPage);
        if (moduleTabs) {
            moduleTabs->setCurrentIndex(1);
        }
    }
}

void MainWindow::exportCiternes()
{
    if (!citernesTable || citernesTable->rowCount() == 0) {
        QMessageBox::warning(this, "Exporter", "Aucune donnée citerne à exporter.");
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          "Exporter les citernes en PDF",
                                                          QDir::homePath() + "/citernes.pdf",
                                                          "PDF Files (*.pdf)");
    if (filePath.isEmpty()) {
        return;
    }

    QString html;
    html += "<h2>Liste des citernes</h2>";
    html += QString("<p>Généré le %1</p>").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    html += "<table border='1' cellspacing='0' cellpadding='4'>";
    html += "<tr>"
            "<th>ID</th><th>Capacité (L)</th><th>Volume (L)</th><th>Remplissage (%)</th>"
            "<th>Qualité</th><th>Temp (°C)</th><th>Dernier remplissage</th>"
            "</tr>";

    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        if (citernesTable->isRowHidden(r)) {
            continue;
        }

        const QString capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text() : QString();
        const QString volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text() : QString();
        const double capVal = capacity.toDouble();
        const double volVal = volume.toDouble();
        const int pct = (capVal > 0.0) ? int((volVal / capVal) * 100.0) : 0;

        html += "<tr>";
        html += QString("<td>%1</td>").arg(citernesTable->item(r, 0) ? citernesTable->item(r, 0)->text().toHtmlEscaped() : QString());
        html += QString("<td>%1</td>").arg(capacity.toHtmlEscaped());
        html += QString("<td>%1</td>").arg(volume.toHtmlEscaped());
        html += QString("<td>%1</td>").arg(QString::number(pct));
        html += QString("<td>%1</td>").arg(citernesTable->item(r, 4) ? citernesTable->item(r, 4)->text().toHtmlEscaped() : QString());
        html += QString("<td>%1</td>").arg(citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toHtmlEscaped() : QString());
        html += QString("<td>%1</td>").arg(citernesTable->item(r, 6) ? citernesTable->item(r, 6)->text().toHtmlEscaped() : QString());
        html += "</tr>";
    }
    html += "</table>";

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(96);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&writer);

    QMessageBox::information(this, "Exporter", "Export PDF citernes terminé:\n" + filePath);
}

// ============================================================================
// NAVIGATION / STATISTICS
// ============================================================================
void MainWindow::showStatisticsView()
{
    updateClientStatistics();
    if (statisticsPage) {
        stackedWidget->setCurrentWidget(statisticsPage);
        if (moduleTabs) {
            moduleTabs->setCurrentIndex(0);
        }
        return;
    }
    stackedWidget->setCurrentIndex(stackedWidget->count()-1);
}

void MainWindow::showMainListView()
{
    switchToClients();
}

void MainWindow::showMainCiterneListView()
{
    switchToCiternes();
}

// ============================================================================
// ADVANCED CITERNES FEATURES
// ============================================================================

void MainWindow::viewCiterneDetails()
{
    int row = citernesTable->currentRow();
    if (row < 0) { 
        QMessageBox::warning(this, "Attention", "Sélectionnez une citerne pour voir les détails.");
        return; 
    }

    QString id = citernesTable->item(row, 0)->text();
    QString capacity = citernesTable->item(row, 1)->text();
    QString volume = citernesTable->item(row, 2)->text();
    QString quality = citernesTable->item(row, 4)->text();
    QString temp = citernesTable->item(row, 5)->text();
    
    QString details = QString(
        "━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "DÉTAILS DE LA CITERNE #%1\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "Capacité: %2 L\n"
        "Volume actuel: %3 L\n"
        "Taux remplissage: %4%\n"
        "Qualité: %5\n"
        "Température: %6 °C\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "Statut: ACTIF\n"
        "Dernière mise à jour: 2026-02-11 10:30"
    ).arg(id, capacity, volume, 
          QString::number(int((volume.toDouble()/capacity.toDouble())*100)), 
          quality, temp);
    
    QMessageBox::information(this, "Détails Citerne", details);
}

// ============================================================================
// QUALITY SIMULATOR
// ============================================================================

void MainWindow::openBlendingSimulator()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Simulateur Qualité");
    dlg.setMinimumSize(820, 560);

    QVBoxLayout *mainLay = new QVBoxLayout(&dlg);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(10);

    QFrame *hero = new QFrame();
    hero->setStyleSheet("QFrame{background:#0F5C53; border:1px solid #1F7B6E; border-radius:12px;}"
                        "QLabel{color:white; background:transparent;}");
    QVBoxLayout *heroLay = new QVBoxLayout(hero);
    heroLay->setContentsMargins(14, 12, 14, 12);
    QLabel *heroTitle = new QLabel("<b>Simulateur de qualité des mélanges</b>");
    QLabel *heroSub = new QLabel("Sélectionnez plusieurs citernes pour estimer la qualité globale et le volume obtenu.");
    heroSub->setWordWrap(true);
    heroLay->addWidget(heroTitle);
    heroLay->addWidget(heroSub);
    mainLay->addWidget(hero);

    QHBoxLayout *topCtrl = new QHBoxLayout();
    QPushButton *selectAllBtn = new QPushButton("Tout sélectionner");
    selectAllBtn->setProperty("role", "secondary");
    QPushButton *clearBtn = new QPushButton("Tout désélectionner");
    clearBtn->setProperty("role", "secondary");
    QLabel *summaryLbl = new QLabel("Aucune citerne sélectionnée");
    summaryLbl->setStyleSheet("font-weight:700; color:#114E47;");
    topCtrl->addWidget(selectAllBtn);
    topCtrl->addWidget(clearBtn);
    topCtrl->addStretch();
    topCtrl->addWidget(summaryLbl);
    mainLay->addLayout(topCtrl);

    QTableWidget *selectionTable = new QTableWidget();
    selectionTable->setColumnCount(6);
    selectionTable->setHorizontalHeaderLabels({"Sel.", "Citerne", "Volume (L)", "Remplissage", "Qualité", "Temp (°C)"});
    selectionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    selectionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    selectionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    selectionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    selectionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    selectionTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    selectionTable->verticalHeader()->setVisible(false);
    selectionTable->setSelectionMode(QAbstractItemView::NoSelection);
    selectionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    selectionTable->setAlternatingRowColors(true);
    selectionTable->setShowGrid(false);

    QList<QCheckBox*> checkboxes;
    selectionTable->setRowCount(citernesTable->rowCount());
    for (int i = 0; i < citernesTable->rowCount(); ++i) {
        const QString id = citernesTable->item(i, 0) ? citernesTable->item(i, 0)->text() : QString::number(i + 1);
        const QString capText = citernesTable->item(i, 1) ? citernesTable->item(i, 1)->text() : "0";
        const QString volText = citernesTable->item(i, 2) ? citernesTable->item(i, 2)->text() : "0";
        const QString qualityText = citernesTable->item(i, 4) ? citernesTable->item(i, 4)->text() : "N/A";
        const QString tempText = citernesTable->item(i, 5) ? citernesTable->item(i, 5)->text() : "0";

        bool capOk = false;
        const double cap = capText.toDouble(&capOk);
        const double vol = volText.toDouble();
        const double fill = (capOk && cap > 0.0) ? ((vol / cap) * 100.0) : 0.0;

        QWidget *checkHost = new QWidget();
        QHBoxLayout *checkLay = new QHBoxLayout(checkHost);
        checkLay->setContentsMargins(0, 0, 0, 0);
        checkLay->setAlignment(Qt::AlignCenter);
        QCheckBox *cb = new QCheckBox();
        checkLay->addWidget(cb);
        checkboxes.append(cb);

        selectionTable->setCellWidget(i, 0, checkHost);
        selectionTable->setItem(i, 1, new QTableWidgetItem(QString("#%1").arg(id)));
        selectionTable->setItem(i, 2, new QTableWidgetItem(QString::number(vol, 'f', 1)));
        selectionTable->setItem(i, 3, new QTableWidgetItem(QString::number(fill, 'f', 1) + "%"));
        selectionTable->setItem(i, 4, new QTableWidgetItem(qualityText));
        selectionTable->setItem(i, 5, new QTableWidgetItem(tempText));
    }

    mainLay->addWidget(selectionTable, 1);

    auto updateSummary = [&]() {
        int count = 0;
        double totalVol = 0.0;
        for (int i = 0; i < checkboxes.size(); ++i) {
            if (!checkboxes[i]->isChecked()) {
                continue;
            }
            ++count;
            const QTableWidgetItem *volItem = selectionTable->item(i, 2);
            totalVol += volItem ? volItem->text().toDouble() : 0.0;
        }

        if (count == 0) {
            summaryLbl->setText("Aucune citerne sélectionnée");
            return;
        }

        summaryLbl->setText(QString("Sélection: %1 citerne(s) | Volume total: %2 L")
                            .arg(count)
                            .arg(QString::number(totalVol, 'f', 1)));
    };

    for (QCheckBox *cb : checkboxes) {
        connect(cb, &QCheckBox::toggled, &dlg, updateSummary);
    }

    connect(selectAllBtn, &QPushButton::clicked, &dlg, [&]() {
        for (QCheckBox *cb : checkboxes) {
            cb->setChecked(true);
        }
    });
    connect(clearBtn, &QPushButton::clicked, &dlg, [&]() {
        for (QCheckBox *cb : checkboxes) {
            cb->setChecked(false);
        }
    });

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setText("Lancer la simulation");
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLay->addWidget(box);

    updateSummary();

    if (dlg.exec() == QDialog::Accepted) {
        qualitySelectedRows.clear();
        for (int i = 0; i < checkboxes.size(); ++i) {
            if (checkboxes[i]->isChecked()) {
                qualitySelectedRows.append(i);
            }
        }

        if (qualitySelectedRows.isEmpty()) {
            QMessageBox::warning(this, "Simulateur Qualité", "Sélectionnez au moins une citerne.");
            return;
        }
        calculateBlendingResult();
    }
}

void MainWindow::calculateBlendingResult()
{
    auto qualityToScore = [](const QString &value) {
        bool ok = false;
        const double numeric = value.toDouble(&ok);
        if (ok) {
            return numeric * 100.0;
        }

        const QString v = value.trimmed().toUpper();
        if (v == "EXTRA") return 95.0;
        if (v == "PREMIUM") return 90.0;
        if (v == "A") return 85.0;
        if (v == "B") return 75.0;
        if (v == "C") return 65.0;
        if (v == "FAIBLE") return 50.0;
        return 70.0;
    };

    double qualitySum = 0.0;
    double totalVolume = 0.0;

    for (int i = 0; i < qualitySelectedRows.size(); ++i) {
        const int row = qualitySelectedRows[i];
        const QString qualityText = citernesTable->item(row, 4) ? citernesTable->item(row, 4)->text() : QString();
        const double volume = citernesTable->item(row, 2) ? citernesTable->item(row, 2)->text().toDouble() : 0.0;
        const double score = qualityToScore(qualityText);

        qualitySum += score;
        totalVolume += volume;
    }

    const double qualityScore = qualitySum / qualitySelectedRows.size();
    QString qualityClass = "Faible";
    if (qualityScore >= 90.0) qualityClass = "Excellente";
    else if (qualityScore >= 80.0) qualityClass = "Élevée";
    else if (qualityScore >= 70.0) qualityClass = "Moyenne";

    QDialog resultDlg(this);
    resultDlg.setWindowTitle("Résultat - Simulateur Qualité");
    resultDlg.setMinimumSize(520, 360);

    QVBoxLayout *vl = new QVBoxLayout(&resultDlg);

    QFrame *hero = new QFrame();
    hero->setStyleSheet("QFrame{background:#0F5C53; border:1px solid #1F7B6E; border-radius:12px;}"
                        "QLabel{color:white; background:transparent;}");
    QVBoxLayout *heroLay = new QVBoxLayout(hero);
    heroLay->setContentsMargins(14, 12, 14, 12);
    QLabel *title = new QLabel("<b>Simulation terminée</b>");
    QLabel *subtitle = new QLabel(QString("Classe qualité obtenue: <b>%1</b>").arg(qualityClass));
    heroLay->addWidget(title);
    heroLay->addWidget(subtitle);
    vl->addWidget(hero);

    QGridLayout *cards = new QGridLayout();
    auto makeCard = [&](const QString &label, const QString &value) {
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame{background:white; border:1px solid #D8E2DE; border-radius:10px;}"
                            "QLabel{background:transparent;}");
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(12, 10, 12, 10);
        QLabel *l = new QLabel(label);
        l->setStyleSheet("color:#35514C; font-weight:700;");
        QLabel *v = new QLabel(value);
        v->setStyleSheet("color:#0C4F47; font-size:24px; font-weight:900;");
        cl->addWidget(l);
        cl->addWidget(v);
        return card;
    };

    cards->addWidget(makeCard("Citernes sélectionnées", QString::number(qualitySelectedRows.size())), 0, 0);
    cards->addWidget(makeCard("Volume total", QString::number(totalVolume, 'f', 1) + " L"), 0, 1);
    cards->addWidget(makeCard("Indice qualité", QString::number(qualityScore, 'f', 1) + " / 100"), 1, 0, 1, 2);
    vl->addLayout(cards);

    QProgressBar *scoreBar = new QProgressBar();
    scoreBar->setRange(0, 100);
    scoreBar->setValue(qBound(0, int(qRound(qualityScore)), 100));
    scoreBar->setFormat("Score qualité: %v / 100");
    scoreBar->setMinimumHeight(24);
    vl->addWidget(scoreBar);

    QLabel *reco = new QLabel();
    reco->setWordWrap(true);
    if (qualityScore >= 90.0) {
        reco->setText("✅ Lot premium: prêt pour un conditionnement haut de gamme.");
    } else if (qualityScore >= 80.0) {
        reco->setText("🟢 Lot stable: qualité élevée, distribution standard recommandée.");
    } else if (qualityScore >= 70.0) {
        reco->setText("🟡 Lot moyen: prévoir un contrôle laboratoire complémentaire.");
    } else {
        reco->setText("🔴 Lot faible: correction de mélange recommandée avant libération.");
    }
    reco->setStyleSheet("padding:8px; border:1px solid #D8E2DE; border-radius:8px; background:#FBFCFA;");
    vl->addWidget(reco);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, &resultDlg, &QDialog::accept);
    vl->addWidget(box);

    resultDlg.exec();
    notificationHistory.append(QString("Qualité: %1/100 (%2), volume estimé %3 L")
                               .arg(qualityScore, 0, 'f', 1)
                               .arg(qualityClass)
                               .arg(int(totalVolume)));
}

void MainWindow::performBlending()
{
    calculateBlendingResult();
}

// ============================================================================
// NOTIFICATIONS & ALERTS
// ============================================================================

QStringList MainWindow::buildCiterneAlerts(double thresholdPercent, int *criticalCount, int *warningCount, int *normalCount) const
{
    int critical = 0;
    int warning = 0;
    int normal = 0;
    QStringList alerts;

    auto qualityToScore = [](const QString &value) {
        bool ok = false;
        const double numeric = value.toDouble(&ok);
        if (ok) {
            return numeric * 100.0;
        }

        const QString v = value.trimmed().toUpper();
        if (v == "EXTRA") return 95.0;
        if (v == "PREMIUM") return 90.0;
        if (v == "A") return 85.0;
        if (v == "B") return 75.0;
        if (v == "C") return 65.0;
        if (v == "FAIBLE") return 50.0;
        return 70.0;
    };

    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        const int id = citernesTable->item(r, 0) ? citernesTable->item(r, 0)->text().toInt() : (r + 1);
        const double capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text().toDouble() : 0.0;
        const double volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text().toDouble() : 0.0;
        const QString qualityText = citernesTable->item(r, 4) ? citernesTable->item(r, 4)->text() : QString();
        const double temp = citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toDouble() : 20.0;

        if (capacity <= 0.0) {
            ++critical;
            alerts << QString("🔴 Citerne #%1: capacité invalide").arg(id);
            continue;
        }

        const double fillPercent = (volume / capacity) * 100.0;
        const double qualityScore = qualityToScore(qualityText);

        QStringList reasons;
        QString severity = "normal";

        if (fillPercent < qMax(5.0, thresholdPercent * 0.6)) {
            severity = "critical";
            reasons << QString("niveau critique (%1%)").arg(fillPercent, 0, 'f', 1);
        } else if (fillPercent < thresholdPercent) {
            if (severity != "critical") severity = "warning";
            reasons << QString("niveau bas (%1%)").arg(fillPercent, 0, 'f', 1);
        }

        if (temp < 5.0 || temp > 32.0) {
            severity = "critical";
            reasons << QString("température critique (%1°C)").arg(temp, 0, 'f', 1);
        } else if (temp < 8.0 || temp > 28.0) {
            if (severity != "critical") severity = "warning";
            reasons << QString("température instable (%1°C)").arg(temp, 0, 'f', 1);
        }

        if (qualityScore < 60.0) {
            severity = "critical";
            reasons << QString("qualité faible (%1/100)").arg(qualityScore, 0, 'f', 1);
        } else if (qualityScore < 70.0) {
            if (severity != "critical") severity = "warning";
            reasons << QString("qualité à surveiller (%1/100)").arg(qualityScore, 0, 'f', 1);
        }

        if (severity == "critical") {
            ++critical;
            alerts << QString("🔴 Citerne #%1: %2").arg(id).arg(reasons.join(", "));
        } else if (severity == "warning") {
            ++warning;
            alerts << QString("🟡 Citerne #%1: %2").arg(id).arg(reasons.join(", "));
        } else {
            ++normal;
            alerts << QString("🟢 Citerne #%1: statut normal (%2%)").arg(id).arg(fillPercent, 0, 'f', 1);
        }
    }

    if (criticalCount) *criticalCount = critical;
    if (warningCount) *warningCount = warning;
    if (normalCount) *normalCount = normal;
    return alerts;
}

void MainWindow::showNotifications()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Alertes et Notifications");
    dlg.setMinimumSize(860, 620);

    QVBoxLayout *mainLay = new QVBoxLayout(&dlg);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(10);

    QFrame *hero = new QFrame();
    hero->setStyleSheet("QFrame{background:#0F5C53; border:1px solid #1F7B6E; border-radius:12px;}"
                        "QLabel{color:white; background:transparent;}");
    QVBoxLayout *heroLay = new QVBoxLayout(hero);
    heroLay->setContentsMargins(14, 12, 14, 12);
    QLabel *heroTitle = new QLabel("<b>Centre d'alertes et notifications</b>");
    QLabel *heroSub = new QLabel("Surveillance en direct: niveaux, température, qualité et alertes prédictives.");
    heroSub->setWordWrap(true);
    heroLay->addWidget(heroTitle);
    heroLay->addWidget(heroSub);
    mainLay->addWidget(hero);

    QHBoxLayout *thresholdLayout = new QHBoxLayout();
    QLabel *thresholdLabel = new QLabel("Seuil niveau bas (%)");
    thresholdLabel->setStyleSheet("font-weight:700;");
    QDoubleSpinBox *thresholdSpin = new QDoubleSpinBox();
    thresholdSpin->setValue(lowLevelThreshold);
    thresholdSpin->setRange(0.0, 100.0);
    thresholdSpin->setDecimals(0);
    thresholdSpin->setSingleStep(1.0);
    thresholdSpin->setFixedWidth(90);

    QPushButton *refreshBtn = new QPushButton("Analyser");
    refreshBtn->setProperty("role", "accent");
    QPushButton *defaultBtn = new QPushButton("Seuil défaut (30%)");
    defaultBtn->setProperty("role", "secondary");

    thresholdLayout->addWidget(thresholdLabel);
    thresholdLayout->addWidget(thresholdSpin);
    thresholdLayout->addWidget(refreshBtn);
    thresholdLayout->addWidget(defaultBtn);
    thresholdLayout->addStretch();
    mainLay->addLayout(thresholdLayout);

    QHBoxLayout *kpiRow = new QHBoxLayout();
    auto makeKpi = [&](const QString &title, const QString &color, QLabel **valueLabel) {
        QFrame *card = new QFrame();
        card->setStyleSheet(QString("QFrame{background:white; border:1px solid #D8E2DE; border-left:4px solid %1; border-radius:10px;}"
                                    "QLabel{background:transparent;}").arg(color));
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(10, 8, 10, 8);
        QLabel *t = new QLabel(title);
        t->setStyleSheet("font-weight:700; color:#39534F;");
        QLabel *v = new QLabel("0");
        v->setStyleSheet("font-size:24px; font-weight:900; color:#0E4A44;");
        cl->addWidget(t);
        cl->addWidget(v);
        *valueLabel = v;
        return card;
    };

    QLabel *criticalValue = nullptr;
    QLabel *warningValue = nullptr;
    QLabel *normalValue = nullptr;
    kpiRow->addWidget(makeKpi("Critiques", "#FF3A3A", &criticalValue), 1);
    kpiRow->addWidget(makeKpi("A surveiller", "#FF7A00", &warningValue), 1);
    kpiRow->addWidget(makeKpi("Normales", "#38C86A", &normalValue), 1);
    mainLay->addLayout(kpiRow);

    QHBoxLayout *listsRow = new QHBoxLayout();

    QGroupBox *alertsBox = new QGroupBox("Alertes actuelles");
    QVBoxLayout *alertsLay = new QVBoxLayout(alertsBox);
    QListWidget *alertsList = new QListWidget();
    alertsList->setSelectionMode(QAbstractItemView::NoSelection);
    alertsList->setAlternatingRowColors(true);
    alertsLay->addWidget(alertsList);

    QGroupBox *historyBox = new QGroupBox("Historique");
    QVBoxLayout *historyLay = new QVBoxLayout(historyBox);
    QListWidget *historyList = new QListWidget();
    historyList->setSelectionMode(QAbstractItemView::NoSelection);
    historyList->setAlternatingRowColors(true);
    const int maxItems = 30;
    const int start = qMax(0, notificationHistory.size() - maxItems);
    for (int i = start; i < notificationHistory.size(); ++i) {
        historyList->addItem(notificationHistory[i]);
    }
    historyLay->addWidget(historyList);

    listsRow->addWidget(alertsBox, 3);
    listsRow->addWidget(historyBox, 2);
    mainLay->addLayout(listsRow, 1);

    auto refreshAlerts = [&]() {
        int critical = 0;
        int warning = 0;
        int normal = 0;
        const QStringList alerts = buildCiterneAlerts(thresholdSpin->value(), &critical, &warning, &normal);

        alertsList->clear();
        for (const QString &alert : alerts) {
            QListWidgetItem *item = new QListWidgetItem(alert);
            if (alert.startsWith("🔴")) {
                item->setForeground(QBrush(QColor("#9D1C1C")));
            } else if (alert.startsWith("🟡")) {
                item->setForeground(QBrush(QColor("#8B5A16")));
            } else {
                item->setForeground(QBrush(QColor("#1E6B2A")));
            }
            alertsList->addItem(item);
        }

        if (criticalValue) criticalValue->setText(QString::number(critical));
        if (warningValue) warningValue->setText(QString::number(warning));
        if (normalValue) normalValue->setText(QString::number(normal));
    };

    connect(refreshBtn, &QPushButton::clicked, &dlg, refreshAlerts);
    connect(defaultBtn, &QPushButton::clicked, &dlg, [&]() {
        thresholdSpin->setValue(30.0);
        refreshAlerts();
    });

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Ok)->setText("Enregistrer seuil");
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLay->addWidget(box);

    refreshAlerts();

    if (dlg.exec() == QDialog::Accepted) {
        lowLevelThreshold = thresholdSpin->value();
        checkLowLevelAlerts();
        checkPredictiveAlerts();
        const QString stamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        notificationHistory.append(QString("[%1] Seuil alertes mis à %2%").arg(stamp).arg(lowLevelThreshold, 0, 'f', 0));
    }
}

// ============================================================================
// PREDICTIVE MAINTENANCE
// ============================================================================

void MainWindow::showEquipmentStatus()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Maintenance Prédictive - État des Équipements");
    dlg.setMinimumSize(860, 620);

    QVBoxLayout *mainLay = new QVBoxLayout(&dlg);
    mainLay->setContentsMargins(14, 14, 14, 14);
    mainLay->setSpacing(10);

    QFrame *hero = new QFrame();
    hero->setStyleSheet("QFrame{background:#0F5C53; border:1px solid #1F7B6E; border-radius:12px;}"
                        "QLabel{color:white; background:transparent;}");
    QVBoxLayout *heroLay = new QVBoxLayout(hero);
    heroLay->setContentsMargins(14, 12, 14, 12);
    QLabel *heroTitle = new QLabel("<b>Maintenance prédictive</b>");
    QLabel *heroSub = new QLabel("Évalue le risque, l'état de santé et la durée de vie restante de chaque citerne.");
    heroSub->setWordWrap(true);
    heroLay->addWidget(heroTitle);
    heroLay->addWidget(heroSub);
    mainLay->addWidget(hero);

    QScrollArea *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    QWidget *content = new QWidget();
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setSpacing(10);

    QStringList globalAlerts;

    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        const int id = citernesTable->item(r, 0) ? citernesTable->item(r, 0)->text().toInt() : (r + 1);
        const double capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text().toDouble() : 0.0;
        const double volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text().toDouble() : 0.0;
        const QString quality = citernesTable->item(r, 4) ? citernesTable->item(r, 4)->text().trimmed() : QString();
        const double temperature = citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toDouble() : 20.0;

        const double fillPercent = (capacity > 0.0) ? ((volume / capacity) * 100.0) : 0.0;

        int riskScore = 5;
        QStringList anomalies;

        if (fillPercent < 20.0) {
            riskScore += 35;
            anomalies << QString("niveau critique (%1%)").arg(fillPercent, 0, 'f', 1);
        } else if (fillPercent < 35.0) {
            riskScore += 20;
            anomalies << QString("niveau faible (%1%)").arg(fillPercent, 0, 'f', 1);
        }

        if (temperature < 8.0 || temperature > 26.0) {
            riskScore += 15;
            anomalies << QString("température hors plage (%1°C)").arg(temperature, 0, 'f', 1);
        }
        if (temperature < 5.0 || temperature > 30.0) {
            riskScore += 15;
        }

        bool qualityNumericOk = false;
        double qualityScore = quality.toDouble(&qualityNumericOk) * 100.0;
        if (!qualityNumericOk) {
            const QString q = quality.toUpper();
            if (q == "EXTRA") qualityScore = 95.0;
            else if (q == "PREMIUM") qualityScore = 90.0;
            else if (q == "A") qualityScore = 85.0;
            else if (q == "B") qualityScore = 75.0;
            else if (q == "C") qualityScore = 65.0;
            else qualityScore = 70.0;
        }

        if (qualityScore < 65.0) {
            riskScore += 20;
            anomalies << QString("qualité dégradée (%1/100)").arg(qualityScore, 0, 'f', 1);
        } else if (qualityScore < 75.0) {
            riskScore += 10;
        }

        riskScore = qBound(0, riskScore, 100);
        const int healthScore = 100 - riskScore;
        const int rulDays = qMax(7, 120 - riskScore);

        QString riskLevel = "Faible";
        if (riskScore >= 70) riskLevel = "Élevé";
        else if (riskScore >= 40) riskLevel = "Moyen";

        QString recommendation = "Surveillance standard.";
        if (riskScore >= 70) {
            recommendation = "Maintenance immédiate recommandée.";
        } else if (riskScore >= 40) {
            recommendation = "Inspection préventive sous 7 jours.";
        }

        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame{background:white; border:1px solid #D8E2DE; border-radius:10px;}"
                            "QLabel{background:transparent;}");
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(12, 10, 12, 10);

        QLabel *title = new QLabel(QString("<b>Citerne #%1</b>").arg(id));
        title->setStyleSheet("color:#114E47; font-size:15px;");
        cl->addWidget(title);

        QLabel *meta = new QLabel(QString("Risque: <b>%1</b> | RUL estimé: <b>%2 jours</b>").arg(riskLevel).arg(rulDays));
        meta->setStyleSheet("color:#39534F;");
        cl->addWidget(meta);

        QProgressBar *healthBar = new QProgressBar();
        healthBar->setRange(0, 100);
        healthBar->setValue(qBound(0, healthScore, 100));
        healthBar->setFormat(QString("Santé équipement: %1%%").arg(healthScore));
        healthBar->setMinimumHeight(22);
        cl->addWidget(healthBar);

        QLabel *detail = new QLabel(QString("Remplissage: %1% | Temp: %2°C | Qualité: %3/100")
                                    .arg(fillPercent, 0, 'f', 1)
                                    .arg(temperature, 0, 'f', 1)
                                    .arg(qualityScore, 0, 'f', 1));
        detail->setStyleSheet("color:#21423D;");
        cl->addWidget(detail);

        QLabel *recoLbl = new QLabel(QString("Action recommandée: %1").arg(recommendation));
        recoLbl->setWordWrap(true);
        recoLbl->setStyleSheet("padding:6px; border:1px solid #D8E2DE; border-radius:8px; background:#FBFCFA;");
        cl->addWidget(recoLbl);

        if (!anomalies.isEmpty()) {
            QLabel *anomaliesLbl = new QLabel("Anomalies: " + anomalies.join("; "));
            anomaliesLbl->setWordWrap(true);
            anomaliesLbl->setStyleSheet("color: #8A4B08; font-weight: 600;");
            cl->addWidget(anomaliesLbl);
            globalAlerts << QString("Citerne #%1: %2").arg(id).arg(anomalies.join(", "));
        }

        contentLay->addWidget(card);
    }

    QLabel *predictionLbl = new QLabel();
    predictionLbl->setWordWrap(true);
    if (globalAlerts.isEmpty()) {
        predictionLbl->setText("\n✓ <b>Aucune anomalie critique détectée.</b>\nMaintenance préventive standard.");
    } else {
        predictionLbl->setText("\n🔍 <b>Prédictions Anomalies:</b>\n• " + globalAlerts.join("\n• "));
    }
    predictionLbl->setStyleSheet("padding:8px; border:1px solid #D8E2DE; border-radius:8px; background:#FBFCFA;");
    contentLay->addWidget(predictionLbl);
    contentLay->addStretch();

    scroll->setWidget(content);
    mainLay->addWidget(scroll, 1);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    mainLay->addWidget(box);
    
    dlg.exec();
}

void MainWindow::checkPredictiveAlerts()
{
    QStringList alerts;
    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        const int id = citernesTable->item(r, 0) ? citernesTable->item(r, 0)->text().toInt() : (r + 1);
        const double capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text().toDouble() : 0.0;
        const double volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text().toDouble() : 0.0;
        const double temp = citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toDouble() : 20.0;

        const double fillPercent = (capacity > 0.0) ? ((volume / capacity) * 100.0) : 0.0;
        int risk = 0;
        if (fillPercent < 20.0) risk += 40;
        else if (fillPercent < 35.0) risk += 20;
        if (temp < 5.0 || temp > 30.0) risk += 35;
        else if (temp < 8.0 || temp > 26.0) risk += 20;

        if (risk >= 40) {
            const QString level = (risk >= 70) ? "ÉLEVÉ" : "MOYEN";
            alerts << QString("Citerne #%1: risque %2 (%3)").arg(id).arg(level).arg(risk);
        }
    }

    if (alerts.isEmpty()) {
        QMessageBox::information(this, "Maintenance prédictive", "Aucune alerte prédictive détectée.");
        return;
    }

    notificationHistory.append(alerts);
    QMessageBox::warning(this, "Maintenance prédictive", "Alertes détectées:\n- " + alerts.join("\n- "));
}

void MainWindow::checkLowLevelAlerts()
{
    for (int i = 0; i < citernesTable->rowCount(); ++i) {
        const QString volume = citernesTable->item(i, 2) ? citernesTable->item(i, 2)->text() : QString();
        const QString capacity = citernesTable->item(i, 1) ? citernesTable->item(i, 1)->text() : QString();
        const int id = citernesTable->item(i, 0) ? citernesTable->item(i, 0)->text().toInt() : (i + 1);
        const double capacityValue = capacity.toDouble();
        if (capacityValue <= 0.0) {
            continue;
        }

        const double fillPercent = (volume.toDouble() / capacityValue) * 100.0;
        if (fillPercent < lowLevelThreshold) {
            notificationHistory.append(QString("Alerte: Citerne %1 niveau bas (%2%)").arg(id).arg((int)fillPercent));
        }
    }
}

void MainWindow::detectAnomalies()
{
    QStringList anomalies;

    for (int r = 0; r < citernesTable->rowCount(); ++r) {
        const int id = citernesTable->item(r, 0) ? citernesTable->item(r, 0)->text().toInt() : (r + 1);
        const double capacity = citernesTable->item(r, 1) ? citernesTable->item(r, 1)->text().toDouble() : 0.0;
        const double volume = citernesTable->item(r, 2) ? citernesTable->item(r, 2)->text().toDouble() : 0.0;
        const double temp = citernesTable->item(r, 5) ? citernesTable->item(r, 5)->text().toDouble() : 20.0;

        if (capacity <= 0.0) {
            anomalies << QString("Citerne #%1: capacité invalide").arg(id);
            continue;
        }

        const double fillPercent = (volume / capacity) * 100.0;
        if (fillPercent < 10.0) {
            anomalies << QString("Citerne #%1: niveau critique (%2%)").arg(id).arg(fillPercent, 0, 'f', 1);
        } else if (fillPercent > 98.0) {
            anomalies << QString("Citerne #%1: quasi-surcharge (%2%)").arg(id).arg(fillPercent, 0, 'f', 1);
        }

        if (temp < 5.0 || temp > 30.0) {
            anomalies << QString("Citerne #%1: température anormale (%2°C)").arg(id).arg(temp, 0, 'f', 1);
        }
    }

    if (anomalies.isEmpty()) {
        QMessageBox::information(this, "Détection Anomalies", "Aucune anomalie détectée.");
        return;
    }

    notificationHistory.append(anomalies);
    QMessageBox::warning(this, "Détection Anomalies", "Anomalies détectées:\n- " + anomalies.join("\n- "));
}

void MainWindow::configureThresholds()
{
    // Implementation for threshold configuration
    QMessageBox::information(this, "Configuration", "Configuration des seuils...");
}

void MainWindow::viewFillingHistory()
{
    if (notificationHistory.isEmpty()) {
        QMessageBox::information(this, "Historique", "Aucun événement enregistré.");
        return;
    }

    QMessageBox::information(this, "Historique", notificationHistory.join("\n"));
}

bool MainWindow::promptAndTestOracleConnection()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Connexion Oracle (ODBC)");
    dlg.setModal(true);
    dlg.setMinimumSize(430, 240);

    QFormLayout *form = new QFormLayout();
    QLineEdit *dsnEdit = new QLineEdit("projetqt");
    QLineEdit *userEdit = new QLineEdit("awss");
    QLineEdit *passEdit = new QLineEdit("123");
    passEdit->setEchoMode(QLineEdit::Password);

    form->addRow("DSN ODBC:", dsnEdit);
    form->addRow("Utilisateur:", userEdit);
    form->addRow("Mot de passe:", passEdit);

    QLabel *hint = new QLabel("Saisis le DSN ODBC Oracle puis clique sur OK pour tester la connexion.");
    hint->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addWidget(hint);
    layout->addLayout(form);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, [&]() {
        if (dsnEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "Le DSN est obligatoire.");
            return;
        }
        if (userEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Saisie invalide", "L'utilisateur est obligatoire.");
            return;
        }
        dlg.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted) {
        return false;
    }

    const QString dsn = dsnEdit->text().trimmed();
    const QString user = userEdit->text().trimmed();
    const QString pass = passEdit->text();

    QString lastError;
    Connection *cx = Connection::instance();
    const bool connected = cx && cx->openOdbcConnection(dsn, user, pass, lastError);
    if (connected) {
        db = cx->database();
        QMessageBox::information(this,
                                 "Oracle",
                                 "Connexion Oracle/ODBC réussie.\n"
                                 "DSN utilisé: " + dsn +
                                 "\nTest SELECT 1 FROM DUAL = 1");
        return true;
    }

    QMessageBox::critical(this,
                          "Oracle",
                          "Échec de connexion Oracle via ODBC.\n"
                          "Vérifie le DSN, l'utilisateur/mot de passe et le driver ODBC Oracle.\n\n"
                          "Dernière erreur: " + lastError +
                          "\n\nDrivers Qt disponibles: " + QSqlDatabase::drivers().join(", "));
    return false;
}

void MainWindow::updateDatabaseStatusLabel()
{
    if (!dbStatusLabel) {
        return;
    }

    if (db.isValid() && db.isOpen()) {
        const QString target = db.databaseName();
        dbStatusLabel->setText("Base Oracle: connectée\n" + target + "\nUtilisateur: " + db.userName());
        dbStatusLabel->setStyleSheet("padding:8px; border-radius:8px; background:#2E574B; color:#FFFFFF;");
    } else {
        dbStatusLabel->setText("Base Oracle: non connectée\nMode démonstration actif");
        dbStatusLabel->setStyleSheet("padding:8px; border-radius:8px; background:#5C2B2B; color:#FFFFFF;");
    }
}
