#include "mainwindow.h"
#include "connexion.h"
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
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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
    if (db.isValid() && db.isOpen()) {
        db.close();
    }
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
    moduleTabs->addTab("Facturation");
    contentLayout->addWidget(moduleTabs, 0);

    stackedWidget = new QStackedWidget();
    contentLayout->addWidget(stackedWidget, 1);

    hroot->addWidget(content, 1);

    setCentralWidget(central);

    // Create pages and add to stack
    createClientsPage();
    createCiternesPage();
    createReceptionPage();
    createFacturationPage();
    createStatisticsPage();

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
    btnFacturation = new QPushButton("Facturation");

    QList<QPushButton*> buttons = {btnClients, btnCiternes, btnReception, btnFacturation};
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
    connect(btnFacturation, &QPushButton::clicked, this, &MainWindow::switchToFacturation);

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

void MainWindow::switchToFacturation()
{
    stackedWidget->setCurrentWidget(facturationPage);
    if (btnFacturation) btnFacturation->setChecked(true);
    if (moduleTabs) moduleTabs->setCurrentIndex(3);
}

// ============================================================================
// CLIENTS PAGE - full clients UI (table + controls)
// ============================================================================
void MainWindow::createClientsPage()
{
    clientsPage = new QWidget();
    QVBoxLayout *mainLay = new QVBoxLayout(clientsPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    // Header
    QWidget *hdr = createHeaderWidget("Gestion des Clients");
    mainLay->addWidget(hdr);

    // Controls + table
    QWidget *content = new QWidget();
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
    sortButton->setFixedSize(80,34);
    connect(sortButton, &QPushButton::clicked, this, &MainWindow::sortClients);

    exportButton = new QPushButton("Exporter");
    exportButton->setFixedSize(100,34);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportClients);

    statsButton = new QPushButton("Statistiques");
    statsButton->setFixedSize(110,34);
    connect(statsButton, &QPushButton::clicked, this, &MainWindow::showStatisticsView);

    ctrl->addWidget(searchBox);
    ctrl->addWidget(sortButton);
    ctrl->addWidget(exportButton);
    ctrl->addWidget(statsButton);
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
    clientsTable->setObjectName("clientsTable");
    contentLay->addWidget(clientsTable);

    // Action buttons
    QHBoxLayout *actions = new QHBoxLayout();
    addButton = new QPushButton("+ Ajouter");
    addButton->setFixedSize(120,36);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::showAddClientDialog);

    editButton = new QPushButton("✎ Éditer");
    editButton->setFixedSize(100,36);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedClient);

    viewButton = new QPushButton("👁 Voir");
    viewButton->setFixedSize(100,36);
    connect(viewButton, &QPushButton::clicked, this, &MainWindow::viewSelectedClient);

    deleteButton = new QPushButton("🗑 Supprimer");
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
// CITERNE PAGE - reuse simple table + actions (you can expand later)
// ============================================================================
void MainWindow::createCiternesPage()
{
    citernesPage = new QWidget();
    QVBoxLayout *mainLay = new QVBoxLayout(citernesPage);
    mainLay->setContentsMargins(12,12,12,12);
    mainLay->setSpacing(8);

    QWidget *hdr = createHeaderWidget("Gestion Avancée des Citernes");
    mainLay->addWidget(hdr);

    // Content
    QWidget *content = new QWidget();
    QVBoxLayout *contentLay = new QVBoxLayout(content);
    contentLay->setContentsMargins(8,8,8,8);
    contentLay->setSpacing(10);

    // Search row for citernes (only search input)
    QHBoxLayout *ctrl = new QHBoxLayout();
    searchBoxCiternes = new QLineEdit();
    searchBoxCiternes->setPlaceholderText("Rechercher par ID / Qualité ...");
    searchBoxCiternes->setFixedHeight(34);
    connect(searchBoxCiternes, &QLineEdit::textChanged, this, &MainWindow::searchCiternes);

    ctrl->addWidget(searchBoxCiternes);
    ctrl->addStretch();
    contentLay->addLayout(ctrl);
    
    // Advanced features row
    QHBoxLayout *advCtrl = new QHBoxLayout();
    blendingBtn = new QPushButton("🧪 Simulateur Qualité");
    blendingBtn->setFixedSize(180,34);
    connect(blendingBtn, &QPushButton::clicked, this, &MainWindow::openBlendingSimulator);
    
    alertsBtn = new QPushButton("⚠️ Alertes & Notifications");
    alertsBtn->setFixedSize(180,34);
    connect(alertsBtn, &QPushButton::clicked, this, &MainWindow::showNotifications);
    
    QPushButton *maintBtn = new QPushButton("🔧 Maintenance Prédictive");
    maintBtn->setFixedSize(180,34);
    connect(maintBtn, &QPushButton::clicked, this, &MainWindow::showEquipmentStatus);
    
    advCtrl->addWidget(blendingBtn);
    advCtrl->addWidget(alertsBtn);
    advCtrl->addWidget(maintBtn);
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
    citernesTable->setObjectName("citernesTable");
    contentLay->addWidget(citernesTable);

    // action buttons below table just like clients
    QHBoxLayout *actions = new QHBoxLayout();
    addCiterneBtn = new QPushButton("+ Ajouter");
    addCiterneBtn->setFixedSize(120,36);
    connect(addCiterneBtn, &QPushButton::clicked, this, &MainWindow::showAddCiterneDialog);

    editCiterneBtn = new QPushButton("✎ Éditer");
    editCiterneBtn->setFixedSize(100,36);
    connect(editCiterneBtn, &QPushButton::clicked, this, &MainWindow::editSelectedCiterne);

    detailsCiterneBtn = new QPushButton("👁 Voir");
    detailsCiterneBtn->setFixedSize(100,36);
    connect(detailsCiterneBtn, &QPushButton::clicked, this, &MainWindow::viewCiterneDetails);

    deleteCiterneBtn = new QPushButton("🗑 Supprimer");
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
// RECEPTION and FACTURATION pages (simple placeholders to extend later)
// ============================================================================
void MainWindow::createReceptionPage()
{
    receptionPage = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(receptionPage);
    l->addWidget(createHeaderWidget("Réception"));
    QLabel *lbl = new QLabel("Module Réception - à implémenter");
    lbl->setWordWrap(true);
    l->addWidget(lbl);
    l->addStretch();
    stackedWidget->addWidget(receptionPage);
}

void MainWindow::createFacturationPage()
{
    facturationPage = new QWidget();
    QVBoxLayout *l = new QVBoxLayout(facturationPage);
    l->addWidget(createHeaderWidget("Facturation"));
    QLabel *lbl = new QLabel("Module Facturation - à implémenter");
    lbl->setWordWrap(true);
    l->addWidget(lbl);
    l->addStretch();
    stackedWidget->addWidget(facturationPage);
}

// ============================================================================
// STATISTICS page (simple placeholder)
// ============================================================================
void MainWindow::createStatisticsPage()
{
    QWidget *stats = new QWidget();
    QVBoxLayout *m = new QVBoxLayout(stats);
    m->addWidget(createHeaderWidget("Statistiques"));
    QLabel *lbl = new QLabel("Statistiques générales (placeholder).");
    m->addWidget(lbl);
    m->addStretch();
    // We add statisticsPage to stack so showStatisticsView can show it
    stackedWidget->addWidget(stats);
}

// ============================================================================
// HEADER CREATOR (for pages)
// ============================================================================
QWidget* MainWindow::createHeaderWidget(const QString &title)
{
    QWidget *header = new QWidget();
    header->setObjectName("pageHeader");
    header->setFixedHeight(56);
    QHBoxLayout *h = new QHBoxLayout(header);
    h->setContentsMargins(16,8,16,8);
    QLabel *lbl = new QLabel(title);
    lbl->setStyleSheet("font-size:18px; font-weight:700; color:#FFFFFF;");
    h->addWidget(lbl);
    h->addStretch();
    header->setStyleSheet("background-color:#0A5F58;");
    return header;
}

// ============================================================================
// STYLES
// ============================================================================
void MainWindow::applyStyles()
{
    QString qss = R"(
        QWidget { background: #F7F8FA; font-family: 'Segoe UI', Roboto, Arial; color: #2B2B2B; }
        #sideBar { background: #1B2F2A; }
        #sideBar QLabel { background: transparent; }
        #sideBrand { color: #EAF5F0; font-size: 22px; font-weight: 700; letter-spacing: 0.5px; }
        #sideSubtitle { color: #9CB5AD; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; }
        #dbStatusLabel { background: #24433A; color: #EAF5F0; font-size: 11px; }
        QPushButton[nav="true"] { text-align: left; padding: 10px 12px; border-radius: 10px; color: #EAF5F0; background: transparent; }
        QPushButton[nav="true"]:hover { background: #24433A; }
        QPushButton[nav="true"]:checked { background: #2E574B; color: #FFFFFF; font-weight: 600; }
        #pageHeader { background-color:#0A5F58; }
        #moduleTabs { background: transparent; border: none; }
        QTabBar { border: none; }
        QTabBar::tab { background: #E8F1EE; color: #24433A; padding: 8px 14px; margin-right: 6px; border: none; border-top-left-radius: 8px; border-top-right-radius: 8px; }
        QTabBar::tab:selected { background: #0A5F58; color: #FFFFFF; border: none; }
This branch is up to date with citerne.        #searchBox, #searchBoxCiternes { border:1px solid #A3CAD3; border-radius:6px; padding:6px; }
        QPushButton { border-radius:6px; padding:6px 10px; }
        QTableWidget { background: white; border: 1px solid #E8E8E8; }
        QHeaderView::section { background: #0A5F58; color: white; padding:8px; }
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
        "BEGIN EXECUTE IMMEDIATE 'CREATE TABLE clients_app ("
        "id NUMBER PRIMARY KEY, "
        "nom VARCHAR2(120) NOT NULL, "
        "email VARCHAR2(180), "
        "telephone VARCHAR2(40), "
        "adresse VARCHAR2(240), "
        "inscrit_le DATE DEFAULT SYSDATE NOT NULL, "
        "statut VARCHAR2(30))'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE TABLE citernes_app ("
        "id NUMBER PRIMARY KEY, "
        "capacite_l NUMBER(12,2) NOT NULL, "
        "volume_l NUMBER(12,2) DEFAULT 0 NOT NULL, "
        "qualite VARCHAR2(60), "
        "temperature_c NUMBER(5,2), "
        "dernier_remplissage DATE, "
        "CONSTRAINT ck_citernes_app_cap CHECK (capacite_l > 0), "
        "CONSTRAINT ck_citernes_app_vol CHECK (volume_l >= 0 AND volume_l <= capacite_l))'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE SEQUENCE seq_clients_app START WITH 1 INCREMENT BY 1 NOCACHE NOCYCLE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "BEGIN EXECUTE IMMEDIATE 'CREATE SEQUENCE seq_citernes_app START WITH 1 INCREMENT BY 1 NOCACHE NOCYCLE'; "
        "EXCEPTION WHEN OTHERS THEN IF SQLCODE != -955 THEN RAISE; END IF; END;",

        "CREATE OR REPLACE TRIGGER trg_clients_app_bi "
        "BEFORE INSERT ON clients_app "
        "FOR EACH ROW "
        "WHEN (NEW.id IS NULL) "
        "BEGIN "
        "SELECT seq_clients_app.NEXTVAL INTO :NEW.id FROM dual; "
        "END;",

        "CREATE OR REPLACE TRIGGER trg_citernes_app_bi "
        "BEFORE INSERT ON citernes_app "
        "FOR EACH ROW "
        "WHEN (NEW.id IS NULL) "
        "BEGIN "
        "SELECT seq_citernes_app.NEXTVAL INTO :NEW.id FROM dual; "
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

    QSqlQuery q(db);
    if (!q.exec("SELECT id, nom, email, telephone, adresse, TO_CHAR(inscrit_le,'YYYY-MM-DD'), statut FROM clients_app ORDER BY id")) {
        QMessageBox::critical(this, "Oracle", "Chargement clients échoué:\n" + q.lastError().text());
        return;
    }

    clientsTable->setRowCount(0);
    int row = 0;
    while (q.next()) {
        const int id = q.value(0).toInt();
        clientsTable->insertRow(row);
        clientsTable->setItem(row,0, new QTableWidgetItem(QString::number(id)));
        clientsTable->setItem(row,1, new QTableWidgetItem(q.value(1).toString()));
        clientsTable->setItem(row,2, new QTableWidgetItem(q.value(2).toString()));
        clientsTable->setItem(row,3, new QTableWidgetItem(q.value(3).toString()));
        clientsTable->setItem(row,4, new QTableWidgetItem(q.value(4).toString()));
        clientsTable->setItem(row,5, new QTableWidgetItem(q.value(5).toString()));
        clientsTable->setItem(row,6, new QTableWidgetItem(q.value(6).toString()));
        clientsTable->setCellWidget(row,7, createClientActionsWidget(id));
        ++row;
    }
    clientsTable->resizeColumnsToContents();
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
    QHBoxLayout *hl = new QHBoxLayout(w);
    hl->setContentsMargins(2,2,2,2);
    QPushButton *view = new QPushButton("👁");
    view->setFixedSize(28,28);
    view->setProperty("client_id", id);
    connect(view, &QPushButton::clicked, this, &MainWindow::viewSelectedClient);
    QPushButton *edit = new QPushButton("✎");
    edit->setFixedSize(28,28);
    edit->setProperty("client_id", id);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editSelectedClient);
    QPushButton *del = new QPushButton("🗑");
    del->setFixedSize(28,28);
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
    QHBoxLayout *al = new QHBoxLayout(aw);
    al->setContentsMargins(2,2,2,2);
    QPushButton *fill = new QPushButton("➕");
    fill->setFixedSize(28,28);
    fill->setProperty("citerne_id", id);
    connect(fill, &QPushButton::clicked, this, &MainWindow::onFillButtonClicked);
    QPushButton *drain = new QPushButton("➖");
    drain->setFixedSize(28,28);
    drain->setProperty("citerne_id", id);
    connect(drain, &QPushButton::clicked, this, &MainWindow::onDrainButtonClicked);
    QPushButton *edit = new QPushButton("✎");
    edit->setFixedSize(28,28);
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
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    v->addWidget(bb);

    if (dlg.exec() == QDialog::Accepted) {
        if (oracleActive && db.isOpen()) {
            QSqlQuery q(db);
            q.prepare("INSERT INTO clients_app (nom,email,telephone,adresse,inscrit_le,statut) "
                      "VALUES (:nom,:email,:telephone,:adresse,TO_DATE(:inscrit,'YYYY-MM-DD'),'Actif')");
            q.bindValue(":nom", name->text());
            q.bindValue(":email", email->text());
            q.bindValue(":telephone", phone->text());
            q.bindValue(":adresse", address->text());
            q.bindValue(":inscrit", regDate->date().toString("yyyy-MM-dd"));
            if (!q.exec()) {
                QMessageBox::critical(this, "Oracle", "Ajout client échoué:\n" + q.lastError().text());
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

    QDialog dlg(this);
    dlg.setWindowTitle(QString("Éditer client %1").arg(name));
    QFormLayout *form = new QFormLayout();
    QLineEdit *nameE = new QLineEdit(name);
    QLineEdit *emailE = new QLineEdit(email);
    QLineEdit *phoneE = new QLineEdit(phone);
    QLineEdit *addressE = new QLineEdit(address);
    QDateEdit *regE = new QDateEdit(reg); regE->setCalendarPopup(true);
    form->addRow("Nom:", nameE); form->addRow("Email:", emailE); form->addRow("Téléphone:", phoneE); form->addRow("Adresse:", addressE); form->addRow("Inscrit le:", regE);
    QVBoxLayout *v = new QVBoxLayout(&dlg); v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb,&QDialogButtonBox::accepted,&dlg,&QDialog::accept); connect(bb,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    v->addWidget(bb);
    if (dlg.exec() == QDialog::Accepted) {
        if (oracleActive && db.isOpen()) {
            QSqlQuery q(db);
            q.prepare("UPDATE clients_app SET nom=:nom,email=:email,telephone=:telephone,adresse=:adresse,"
                      "inscrit_le=TO_DATE(:inscrit,'YYYY-MM-DD') WHERE id=:id");
            q.bindValue(":nom", nameE->text());
            q.bindValue(":email", emailE->text());
            q.bindValue(":telephone", phoneE->text());
            q.bindValue(":adresse", addressE->text());
            q.bindValue(":inscrit", regE->date().toString("yyyy-MM-dd"));
            q.bindValue(":id", id);
            if (!q.exec()) {
                QMessageBox::critical(this, "Oracle", "Modification client échouée:\n" + q.lastError().text());
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
            QSqlQuery q(db);
            q.prepare("DELETE FROM clients_app WHERE id=:id");
            q.bindValue(":id", id);
            if (!q.exec()) {
                QMessageBox::critical(this, "Oracle", "Suppression client échouée:\n" + q.lastError().text());
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
    QMessageBox::information(this,"Exporter","Export CSV / PDF - non implémenté.");
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
    QDateEdit *last = new QDateEdit(QDate::currentDate()); last->setCalendarPopup(true);
    form->addRow("Capacité (L):", cap); form->addRow("Volume (L):", vol); form->addRow("Qualité:", qual); form->addRow("Temp (°C):", temp); form->addRow("Dernier remplissage:", last);
    QVBoxLayout *v = new QVBoxLayout(&dlg); v->addLayout(form);
    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); connect(bb,&QDialogButtonBox::accepted,&dlg,&QDialog::accept); connect(bb,&QDialogButtonBox::rejected,&dlg,&QDialog::reject); v->addWidget(bb);

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
    connect(bb,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);
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

void MainWindow::exportCiternes()
{
    QMessageBox::information(this,"Exporter","Export citerne - non implémenté.");
}

// ============================================================================
// NAVIGATION / STATISTICS
// ============================================================================
void MainWindow::showStatisticsView()
{
    // Show the last added page (statistics placeholder)
    // created by createStatisticsPage()
    stackedWidget->setCurrentIndex(stackedWidget->count()-1);
}

void MainWindow::showMainListView()
{
    switchToClients();
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
    dlg.setMinimumWidth(500);
    
    QVBoxLayout *vl = new QVBoxLayout(&dlg);
    
    QLabel *titleLbl = new QLabel("<b>Sélectionnez les citernes à inclure dans l'évaluation:</b>");
    vl->addWidget(titleLbl);
    
    // Sélection simple: une coche par citerne
    QList<QCheckBox*> checkboxes;
    
    for (int i = 0; i < citernesTable->rowCount(); ++i) {
        QString citerneId = citernesTable->item(i, 0)->text();
        QHBoxLayout *hbl = new QHBoxLayout();
        
        const QString qualityText = citernesTable->item(i, 4) ? citernesTable->item(i, 4)->text() : "N/A";
        QCheckBox *cb = new QCheckBox(QString("Citerne %1 (Qualité: %2)").arg(citerneId, qualityText));
        hbl->addWidget(cb);
        checkboxes.append(cb);

        hbl->addStretch();
        vl->addLayout(hbl);
    }
    
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    vl->addWidget(box);
    
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

    QString result = QString(
        "━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "RÉSULTAT QUALITÉ\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "Nombre de citernes: %1\n"
          "Volume total: %2 L\n"
        "Indice qualité: %3 / 100\n"
        "Classe qualité: %4\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "✓ Évaluation qualité terminée"
    ).arg(qualitySelectedRows.size())
      .arg(int(totalVolume))
     .arg(qualityScore, 0, 'f', 1)
     .arg(qualityClass);
    
    QMessageBox::information(this, "Résultat Qualité", result);
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

void MainWindow::showNotifications()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Alertes et Notifications");
    dlg.setMinimumSize(500, 400);
    
    QVBoxLayout *vl = new QVBoxLayout(&dlg);
    
    // Threshold config
    QHBoxLayout *thresholdLayout = new QHBoxLayout();
    thresholdLayout->addWidget(new QLabel("Seuil bas (%):"));
    QDoubleSpinBox *thresholdSpin = new QDoubleSpinBox();
    thresholdSpin->setValue(lowLevelThreshold);
    thresholdSpin->setRange(0, 100);
    thresholdLayout->addWidget(thresholdSpin);
    thresholdLayout->addStretch();
    vl->addLayout(thresholdLayout);
    
    // Alerts list
    QLabel *alertsLbl = new QLabel("<b>Alertes actuelles:</b>");
    vl->addWidget(alertsLbl);
    
    QListWidget *alertsList = new QListWidget();
    alertsList->addItem("⚠️ Citerne #3: Niveau bas (24%)");
    alertsList->addItem("🔴 Citerne #4: Température critique (22.5°C)");
    alertsList->addItem("🟡 Citerne #1: Variation volume (5.2% en 2h)");
    alertsList->addItem("✓ Citerne #2: Statut normal");
    vl->addWidget(alertsList);
    
    // History
    QLabel *historyLbl = new QLabel("<b>Historique remplissages:</b>");
    vl->addWidget(historyLbl);
    
    QListWidget *historyList = new QListWidget();
    historyList->addItem("2026-02-11 10:30 - Citerne #2: +200L");
    historyList->addItem("2026-02-11 09:15 - Citerne #1: -150L");
    historyList->addItem("2026-02-11 08:00 - Citerne #3: +350L");
    vl->addWidget(historyList);
    
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    vl->addWidget(box);
    
    dlg.exec();
    lowLevelThreshold = thresholdSpin->value();
}

// ============================================================================
// PREDICTIVE MAINTENANCE
// ============================================================================

void MainWindow::showEquipmentStatus()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Maintenance Prédictive - État des Équipements");
    dlg.setMinimumSize(600, 500);
    
    QVBoxLayout *vl = new QVBoxLayout(&dlg);
    
    QLabel *titleLbl = new QLabel("<b>Moniteurs de Santé - Prédiction Maintenance</b>");
    vl->addWidget(titleLbl);
    
    // Equipment monitoring
    for (int i = 1; i <= 4; ++i) {
        QGroupBox *gb = new QGroupBox(QString("Citerne #%1").arg(i));
        QVBoxLayout *gvl = new QVBoxLayout(gb);
        
        // Simulate health metrics
        int healthScore = 75 + (i * 5);
        
        QLabel *statusLbl = new QLabel(QString(
            "État général: %1%\n"
            "Température: 22.3°C (Normal)\n"
            "Pression: 1.2 bar (Normal)\n"
            "Niveau huile moteur: 85%\n"
            "RUL (Remaining Useful Life): ~%2 jours"
        ).arg(healthScore).arg(400 - (i * 30)));
        
        gvl->addWidget(statusLbl);
        vl->addWidget(gb);
    }
    
    QLabel *predictionLbl = new QLabel(
        "\n🔍 <b>Prédictions Anomalies:</b>\n"
        "• Citerne #3: Possible fuite détectée (variation volume 3% en 24h)\n"
        "• Citerne #4: Capteur température instable (à calibrer)\n"
        "• Maintenance préventive recommandée dans 15 jours"
    );
    vl->addWidget(predictionLbl);
    
    vl->addStretch();
    
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    vl->addWidget(box);
    
    dlg.exec();
}

void MainWindow::checkPredictiveAlerts()
{
    // Implementation for checking predictive alerts
    QMessageBox::information(this, "Vérification", "Vérification des alertes prédictives en cours...");
}

void MainWindow::checkLowLevelAlerts()
{
    // Implementation for checking low level alerts
    for (int i = 0; i < citernesTable->rowCount(); ++i) {
        QString volume = citernesTable->item(i, 2)->text();
        QString capacity = citernesTable->item(i, 1)->text();
        double fillPercent = (volume.toDouble() / capacity.toDouble()) * 100.0;
        
        if (fillPercent < lowLevelThreshold) {
            notificationHistory.append(QString("Alerte: Citerne %1 niveau bas (%2%)").arg(i+1).arg((int)fillPercent));
        }
    }
}

void MainWindow::detectAnomalies()
{
    // Implementation for detecting anomalies
    QMessageBox::information(this, "Détection Anomalies", "Analyse des anomalies en cours...");
}

void MainWindow::configureThresholds()
{
    // Implementation for threshold configuration
    QMessageBox::information(this, "Configuration", "Configuration des seuils...");
}

void MainWindow::viewFillingHistory()
{
    // Implementation for viewing filling history
    QMessageBox::information(this, "Historique", "Historique des remplissages affichés.");
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
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
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
