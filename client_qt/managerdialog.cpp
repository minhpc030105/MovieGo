#include "managerdialog.h"
#include "responsecodes.h"
#include "browsefilmdialog.h"
#include "addshowtimedialog.h"

extern "C" {
    #include "../client/headers/menu.h"
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

ManagerDialog::ManagerDialog(int sockfd, const QString &username, QWidget *parent)
    : QDialog(parent), sockfd(sockfd), username(username)
{
    setWindowTitle("Manager Panel - MovieGo");
    setModal(true);
    setMinimumSize(500, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title and user info
    titleLabel = new QLabel("MANAGER PANEL", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin: 10px; color: #5bc0de;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    userLabel = new QLabel("Logged in as: " + username, this);
    userLabel->setStyleSheet("font-size: 14px; margin: 5px; color: green;");
    userLabel->setAlignment(Qt::AlignCenter);
    
    // Action buttons
    addFilmBtn = new QPushButton("Add New Film", this);
    addFilmBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 15px; font-weight: bold; font-size: 14px;");
    addFilmBtn->setMinimumHeight(50);
    
    addShowTimeBtn = new QPushButton("Add Show Time", this);
    addShowTimeBtn->setStyleSheet("background-color: #f0ad4e; color: white; padding: 15px; font-weight: bold; font-size: 14px;");
    addShowTimeBtn->setMinimumHeight(50);
    
    browseFilmsBtn = new QPushButton("Browse Films", this);
    browseFilmsBtn->setStyleSheet("background-color: #5bc0de; color: white; padding: 15px; font-weight: bold; font-size: 14px;");
    browseFilmsBtn->setMinimumHeight(50);
    
    logoutBtn = new QPushButton("Logout", this);
    logoutBtn->setStyleSheet("background-color: #777; color: white; padding: 15px; font-weight: bold; font-size: 14px;");
    logoutBtn->setMinimumHeight(50);
    
    // Add to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(userLabel);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(addFilmBtn);
    mainLayout->addWidget(addShowTimeBtn);
    mainLayout->addWidget(browseFilmsBtn);
    mainLayout->addStretch();
    mainLayout->addWidget(logoutBtn);
    
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(15);
    
    // Connect signals
    connect(addFilmBtn, &QPushButton::clicked, this, &ManagerDialog::onAddFilmClicked);
    connect(addShowTimeBtn, &QPushButton::clicked, this, &ManagerDialog::onAddShowTimeClicked);
    connect(browseFilmsBtn, &QPushButton::clicked, this, &ManagerDialog::onBrowseFilmsClicked);
    connect(logoutBtn, &QPushButton::clicked, this, &ManagerDialog::onLogoutClicked);
}

void ManagerDialog::onAddFilmClicked()
{
    showAddFilmDialog();
}

void ManagerDialog::onAddShowTimeClicked()
{
    AddShowTimeDialog dialog(sockfd, this);
    dialog.exec();
}

void ManagerDialog::onBrowseFilmsClicked()
{
    BrowseFilmDialog dialog(sockfd, this);
    dialog.exec();
}

void ManagerDialog::onLogoutClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Send logout request
        char message[255];
        makeLogoutMessage(message);
        sendMessage(sockfd, message);
        
        accept();
    }
}

void ManagerDialog::showAddFilmDialog()
{
    QDialog *addDialog = new QDialog(this);
    addDialog->setWindowTitle("Add New Film");
    addDialog->setMinimumSize(500, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(addDialog);
    
    QLabel *titleLabel = new QLabel("Add New Film", addDialog);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Form layout
    QFormLayout *formLayout = new QFormLayout();
    
    QLineEdit *titleEdit = new QLineEdit(addDialog);
    titleEdit->setPlaceholderText("Enter film title");
    
    QComboBox *categoryCombo = new QComboBox(addDialog);
    
    // Load categories from server
    char message[1024];
    memset(message, 0, sizeof(message));
    sprintf(message, "SHOW_CATEGORIES\r\n");
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result == SHOW_CATEGORY_SUCCESS) {
        while (true) {
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            QString line = QString::fromUtf8(message).trimmed();
            if (line == "END" || line.isEmpty()) break;
            
            // Parse: "1. Action" format
            QStringList parts = line.split(". ");
            if (parts.size() >= 2) {
                QString id = parts[0].trimmed();
                QString name = parts[1].trimmed();
                categoryCombo->addItem(name, id);
            }
        }
    }
    
    if (categoryCombo->count() == 0) {
        categoryCombo->addItem("No categories available", "-1");
    }
    
    QLineEdit *durationEdit = new QLineEdit(addDialog);
    durationEdit->setPlaceholderText("Duration in minutes");
    
    formLayout->addRow("Title:", titleEdit);
    formLayout->addRow("Category:", categoryCombo);
    formLayout->addRow("Duration:", durationEdit);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Add Film", addDialog);
    QPushButton *cancelBtn = new QPushButton("Cancel", addDialog);
    
    saveBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 10px;");
    cancelBtn->setStyleSheet("background-color: #777; color: white; padding: 10px;");
    
    buttonLayout->addWidget(saveBtn);
    buttonLayout->addWidget(cancelBtn);
    
    layout->addWidget(titleLabel);
    layout->addLayout(formLayout);
    layout->addStretch();
    layout->addLayout(buttonLayout);
    
    // Connect save button
    connect(saveBtn, &QPushButton::clicked, [=]() {
        QString title = titleEdit->text().trimmed();
        QString categoryId = categoryCombo->currentData().toString();
        QString duration = durationEdit->text().trimmed();
        
        if (title.isEmpty() || duration.isEmpty() || categoryId == "-1") {
            QMessageBox::warning(addDialog, "Invalid Input", "Please fill all fields!");
            return;
        }
        
        bool ok;
        int durationInt = duration.toInt(&ok);
        if (!ok || durationInt <= 0) {
            QMessageBox::warning(addDialog, "Invalid Duration", "Duration must be a positive number!");
            return;
        }
        
        // Send ADD_FILM request
        char msg[1024];
        snprintf(msg, sizeof(msg), "ADD_FILM\r\n%s\r\n%s\r\n%d\r\n", 
                title.toUtf8().constData(),
                categoryId.toUtf8().constData(),
                durationInt);
        sendMessage(sockfd, msg);
        
        int addResult = recvResult(sockfd);
        if (addResult == ADD_FILM_SUCCESS) {
            QMessageBox::information(addDialog, "Success", "Film added successfully!");
            addDialog->accept();
        } else if (addResult == FILM_EXISTS) {
            QMessageBox::warning(addDialog, "Error", "Film already exists!");
        } else {
            QMessageBox::critical(addDialog, "Error", "Failed to add film!");
        }
    });
    
    connect(cancelBtn, &QPushButton::clicked, addDialog, &QDialog::reject);
    
    addDialog->exec();
    delete addDialog;
}
