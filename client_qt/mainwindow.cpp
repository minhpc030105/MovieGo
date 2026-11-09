#include "mainwindow.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "changepassworddialog.h"
#include "browsefilmdialog.h"
#include "searchfilmdialog.h"
#include "bookticketdialog.h"
#include "viewticketsdialog.h"
#include "admindialog.h"
#include "managerdialog.h"
#include "responsecodes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

extern "C" {
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

MainWindow::MainWindow(int sockfd, QWidget *parent)
    : QMainWindow(parent), sockfd(sockfd), isLoggedIn(false)
{
    setWindowTitle("MovieGo Client");
    setMinimumSize(500, 400);
    
    // Create stacked widget for different pages
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);
    
    // Create welcome page
    welcomePage = new QWidget();
    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomePage);
    
    QLabel *titleLabel = new QLabel("Welcome to MovieGo", welcomePage);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin: 20px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    statusLabel = new QLabel("", welcomePage);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: blue; margin: 10px;");
    
    loginBtn = new QPushButton("Login", welcomePage);
    registerBtn = new QPushButton("Register", welcomePage);
    exitBtn = new QPushButton("Exit", welcomePage);
    
    loginBtn->setMinimumHeight(40);
    registerBtn->setMinimumHeight(40);
    exitBtn->setMinimumHeight(40);
    
    welcomeLayout->addWidget(titleLabel);
    welcomeLayout->addWidget(statusLabel);
    welcomeLayout->addStretch();
    welcomeLayout->addWidget(loginBtn);
    welcomeLayout->addWidget(registerBtn);
    welcomeLayout->addWidget(exitBtn);
    welcomeLayout->addStretch();
    
    welcomeLayout->setContentsMargins(50, 20, 50, 20);
    welcomeLayout->setSpacing(10);
    
    // Create user page
    userPage = new QWidget();
    QVBoxLayout *userLayout = new QVBoxLayout(userPage);
    
    QLabel *userTitleLabel = new QLabel("MovieGo System", userPage);
    userTitleLabel->setStyleSheet("font-size: 20px; font-weight: bold; margin: 20px;");
    userTitleLabel->setAlignment(Qt::AlignCenter);
    
    userLabel = new QLabel("", userPage);
    userLabel->setAlignment(Qt::AlignCenter);
    userLabel->setStyleSheet("font-size: 16px; color: green; margin: 10px;");
    
    browseFilmBtn = new QPushButton("Browse Film Catalogs", userPage);
    searchFilmBtn = new QPushButton("Search Film", userPage);
    bookTicketBtn = new QPushButton("Book Ticket", userPage);
    viewTicketsBtn = new QPushButton("View My Tickets", userPage);
    changePasswordBtn = new QPushButton("Change Password", userPage);
    logoutBtn = new QPushButton("Logout", userPage);
    
    browseFilmBtn->setMinimumHeight(40);
    searchFilmBtn->setMinimumHeight(40);
    bookTicketBtn->setMinimumHeight(40);
    viewTicketsBtn->setMinimumHeight(40);
    changePasswordBtn->setMinimumHeight(40);
    logoutBtn->setMinimumHeight(40);
    
    userLayout->addWidget(userTitleLabel);
    userLayout->addWidget(userLabel);
    userLayout->addStretch();
    userLayout->addWidget(browseFilmBtn);
    userLayout->addWidget(searchFilmBtn);
    userLayout->addWidget(bookTicketBtn);
    userLayout->addWidget(viewTicketsBtn);
    userLayout->addWidget(changePasswordBtn);
    userLayout->addWidget(logoutBtn);
    userLayout->addStretch();
    
    userLayout->setContentsMargins(50, 20, 50, 20);
    userLayout->setSpacing(10);
    
    // Add pages to stacked widget
    stackedWidget->addWidget(welcomePage);
    stackedWidget->addWidget(userPage);
    
    // Connect signals
    connect(loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(exitBtn, &QPushButton::clicked, this, &MainWindow::onExitClicked);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    connect(browseFilmBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFilmClicked);
    connect(searchFilmBtn, &QPushButton::clicked, this, &MainWindow::onSearchFilmClicked);
    connect(bookTicketBtn, &QPushButton::clicked, this, &MainWindow::onBookTicketClicked);
    connect(viewTicketsBtn, &QPushButton::clicked, this, &MainWindow::onViewTicketsClicked);
    
    // Show welcome page
    showWelcomeScreen();
}

MainWindow::~MainWindow()
{
}

void MainWindow::showWelcomeScreen()
{
    stackedWidget->setCurrentWidget(welcomePage);
    isLoggedIn = false;
}

void MainWindow::showUserScreen()
{
    stackedWidget->setCurrentWidget(userPage);
    isLoggedIn = true;
    userLabel->setText("Welcome, " + currentUsername + "!");
}

void MainWindow::setUsername(const QString &username)
{
    currentUsername = username;
}

void MainWindow::onLoginClicked()
{
    LoginDialog dialog(sockfd, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString username = dialog.getUsername();
        int loginResult = dialog.getLoginResult();
        setUsername(username);
        
        if (loginResult == LOGIN_SUCCESS_ADMIN) {
            // Show admin dialog
            AdminDialog adminDialog(sockfd, username, this);
            adminDialog.exec();
            
            // After admin dialog closes, return to welcome screen
            statusLabel->setText("");
            showWelcomeScreen();
        } else if (loginResult == LOGIN_SUCCESS_MANAGER) {
            // Show manager dialog
            ManagerDialog managerDialog(sockfd, username, this);
            managerDialog.exec();
            
            // After manager dialog closes, return to welcome screen
            statusLabel->setText("");
            showWelcomeScreen();
        } else if (loginResult == LOGIN_SUCCESS_USER) {
            // Show normal user screen
            showUserScreen();
        }
    }
}

void MainWindow::onRegisterClicked()
{
    RegisterDialog dialog(sockfd, this);
    if (dialog.exec() == QDialog::Accepted) {
        statusLabel->setText("Registration successful! Please login.");
        QMessageBox::information(this, "Success", "Registration successful! Please login.");
    }
}

void MainWindow::onLogoutClicked()
{
    // Send logout message to server
    char message[255];
    makeLogoutMessage(message);
    sendMessage(sockfd, message);
    
    // Receive response
    int result = recvResult(sockfd);
    if (result == LOGOUT_SUCCESS) {
        QMessageBox::information(this, "Logout", "Logged out successfully!");
        statusLabel->setText("");
        showWelcomeScreen();
    }
}

void MainWindow::onChangePasswordClicked()
{
    ChangePasswordDialog dialog(sockfd, this);
    if (dialog.exec() == QDialog::Accepted) {
        QMessageBox::information(this, "Success", "Password changed successfully!");
    }
}

void MainWindow::onExitClicked()
{
    close();
}

void MainWindow::onBrowseFilmClicked()
{
    BrowseFilmDialog dialog(sockfd, this);
    dialog.exec();
}

void MainWindow::onSearchFilmClicked()
{
    SearchFilmDialog dialog(sockfd, this);
    dialog.exec();
}

void MainWindow::onBookTicketClicked()
{
    BookTicketDialog dialog(sockfd, this);
    dialog.exec();
}

void MainWindow::onViewTicketsClicked()
{
    ViewTicketsDialog dialog(sockfd, currentUsername, this);
    dialog.exec();
}
