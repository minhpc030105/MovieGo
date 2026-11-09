#include "admindialog.h"
#include <QHeaderView>
#include <QInputDialog>
#include <QDebug>
#include "responsecodes.h"

extern "C" {
    #include "../client/headers/menu.h"
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

AdminDialog::AdminDialog(int sockfd, const QString &username, QWidget *parent)
    : QDialog(parent), sockfd(sockfd), username(username)
{
    setWindowTitle("Admin Panel - MovieGo");
    setModal(true);
    setMinimumSize(800, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title and user info
    titleLabel = new QLabel("ADMIN PANEL", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin: 10px; color: #d9534f;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    userLabel = new QLabel("Logged in as: " + username, this);
    userLabel->setStyleSheet("font-size: 14px; margin: 5px; color: green;");
    userLabel->setAlignment(Qt::AlignCenter);
    
    // User table
    userTable = new QTableWidget(this);
    userTable->setColumnCount(4);
    userTable->setHorizontalHeaderLabels({"ID", "Name", "Username", "Role"});
    userTable->horizontalHeader()->setStretchLastSection(true);
    userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable->setSelectionMode(QAbstractItemView::SingleSelection);
    userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable->setAlternatingRowColors(true);
    
    // Set column widths
    userTable->setColumnWidth(0, 50);
    userTable->setColumnWidth(1, 200);
    userTable->setColumnWidth(2, 150);
    
    // Action buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    
    showAllUsersBtn = new QPushButton("Load All Users", this);
    showAllUsersBtn->setStyleSheet("background-color: #5bc0de; color: white; padding: 10px; font-weight: bold;");
    
    deleteUserBtn = new QPushButton("Delete User", this);
    deleteUserBtn->setStyleSheet("background-color: #d9534f; color: white; padding: 10px; font-weight: bold;");
    
    changeRoleBtn = new QPushButton("Change Role", this);
    changeRoleBtn->setStyleSheet("background-color: #f0ad4e; color: white; padding: 10px; font-weight: bold;");
    
    refreshBtn = new QPushButton("Refresh", this);
    refreshBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 10px; font-weight: bold;");
    
    actionLayout->addWidget(showAllUsersBtn);
    actionLayout->addWidget(deleteUserBtn);
    actionLayout->addWidget(changeRoleBtn);
    actionLayout->addWidget(refreshBtn);
    
    // Logout button
    logoutBtn = new QPushButton("Logout", this);
    logoutBtn->setStyleSheet("background-color: #777; color: white; padding: 10px; font-weight: bold;");
    logoutBtn->setMinimumHeight(40);
    
    // Add to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(userLabel);
    mainLayout->addWidget(userTable);
    mainLayout->addLayout(actionLayout);
    mainLayout->addWidget(logoutBtn);
    
    // Connect signals
    connect(showAllUsersBtn, &QPushButton::clicked, this, &AdminDialog::onShowAllUsersClicked);
    connect(deleteUserBtn, &QPushButton::clicked, this, &AdminDialog::onDeleteUserClicked);
    connect(changeRoleBtn, &QPushButton::clicked, this, &AdminDialog::onChangeUserRoleClicked);
    connect(refreshBtn, &QPushButton::clicked, this, &AdminDialog::onRefreshClicked);
    connect(logoutBtn, &QPushButton::clicked, this, &AdminDialog::onLogoutClicked);
    
    // Load users on startup
    loadAllUsers();
}

void AdminDialog::loadAllUsers()
{
    clearUserTable();
    
    // Send SHOW_ALL_USERS request
    char message[1024];
    strcpy(message, "SHOW_ALL_USERS");
    sendMessage(sockfd, message);
    
    // Receive result code
    int result = recvResult(sockfd);
    
    if (result == SHOW_ALL_USERS_SUCCESS) {
        // Receive user data - multiple messages until "END"
        QList<QStringList> userData;
        
        while (true) {
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            
            QString line = QString::fromUtf8(message).trimmed();
            
            if (line == "END") {
                break;
            }
            
            if (!line.isEmpty()) {
                // Parse format: "id|name|username|role"
                QStringList parts = line.split('|');
                if (parts.size() >= 4) {
                    // Filter out current admin account
                    QString usernameInList = parts[2].trimmed();
                    if (usernameInList != username) {
                        userData.append(parts);
                    }
                }
            }
        }
        
        userTable->setRowCount(userData.size());
        
        for (int i = 0; i < userData.size(); i++) {
            QStringList fields = userData[i];
            userTable->setItem(i, 0, new QTableWidgetItem(fields[0])); 
            userTable->setItem(i, 1, new QTableWidgetItem(fields[1])); 
            userTable->setItem(i, 2, new QTableWidgetItem(fields[2])); 
            userTable->setItem(i, 3, new QTableWidgetItem(fields[3])); 
        }
        
        if (userData.size() > 0) {
            QMessageBox::information(this, "Success", QString("Loaded %1 users (excluding your account)").arg(userData.size()));
        } else {
            QMessageBox::information(this, "Info", "No other users found");
        }
    } else {
        QMessageBox::critical(this, "Error", "Failed to load users from server!");
    }
}

void AdminDialog::clearUserTable()
{
    userTable->clearContents();
    userTable->setRowCount(0);
}

void AdminDialog::onShowAllUsersClicked()
{
    loadAllUsers();
}

void AdminDialog::onDeleteUserClicked()
{
    QList<QTableWidgetItem*> selectedItems = userTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a user to delete!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QString userId = userTable->item(row, 0)->text();
    QString username = userTable->item(row, 2)->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, 
        "Confirm Delete", 
        QString("Are you sure you want to delete user:\n\nID: %1\nUsername: %2").arg(userId, username),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Send DELETE_USER request
        char message[255];
        sprintf(message, "DELETE_USER\r\n%s", userId.toUtf8().constData());
        sendMessage(sockfd, message);
        
        // Receive result
        int result = recvResult(sockfd);
        
        if (result == DELETE_USER_SUCCESS) {
            QMessageBox::information(this, "Success", "User deleted successfully!");
            loadAllUsers();
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete user!");
        }
    }
}

void AdminDialog::onChangeUserRoleClicked()
{
    QList<QTableWidgetItem*> selectedItems = userTable->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a user to change role!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QString userId = userTable->item(row, 0)->text();
    QString username = userTable->item(row, 2)->text();
    QString currentRole = userTable->item(row, 3)->text();
    
    // Create dialog to select new role
    QStringList roles;
    roles << "Admin (0)" << "Manager (1)" << "User (2)";
    
    bool ok;
    QString selectedRole = QInputDialog::getItem(
        this,
        "Change User Role",
        QString("Change role for user: %1\nCurrent role: %2\n\nSelect new role:").arg(username, currentRole),
        roles,
        2, // Default to User
        false,
        &ok
    );
    
    if (ok && !selectedRole.isEmpty()) {
        // Extract role ID from selection
        QString roleId;
        if (selectedRole.contains("(0)")) roleId = "0";
        else if (selectedRole.contains("(1)")) roleId = "1";
        else if (selectedRole.contains("(2)")) roleId = "2";
        
        // Send CHANGE_USER_ROLE request
        char message[255];
        sprintf(message, "CHANGE_USER_ROLE\r\n%s\r\n%s", 
                userId.toUtf8().constData(), 
                roleId.toUtf8().constData());
        sendMessage(sockfd, message);
        
        // Receive result
        int result = recvResult(sockfd);
        
        if (result == UPDATE_USER_ROLE_SUCCESS) {
            QMessageBox::information(this, "Success", "User role changed successfully!");
            loadAllUsers();
        } else {
            QMessageBox::critical(this, "Error", "Failed to change user role!");
        }
    }
}

void AdminDialog::onRefreshClicked()
{
    loadAllUsers();
}

void AdminDialog::onLogoutClicked()
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
