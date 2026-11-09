#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include "../client/headers/menu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class LoginDialog;
class RegisterDialog;
class ChangePasswordDialog;
class BrowseFilmDialog;
class SearchFilmDialog;
class BookTicketDialog;
class ViewTicketsDialog;
class AdminDialog;
class ManagerDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(int sockfd, QWidget *parent = nullptr);
    ~MainWindow();
    
    void showWelcomeScreen();
    void showUserScreen();
    void setUsername(const QString &username);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLogoutClicked();
    void onChangePasswordClicked();
    void onExitClicked();
    void onBrowseFilmClicked();
    void onSearchFilmClicked();
    void onBookTicketClicked();
    void onViewTicketsClicked();

private:
    QStackedWidget *stackedWidget;
    QWidget *welcomePage;
    QWidget *userPage;
    
    QPushButton *loginBtn;
    QPushButton *registerBtn;
    QPushButton *exitBtn;
    
    QPushButton *logoutBtn;
    QPushButton *changePasswordBtn;
    QPushButton *browseFilmBtn;
    QPushButton *searchFilmBtn;
    QPushButton *viewTicketsBtn;
    QPushButton *bookTicketBtn;
    
    QLabel *userLabel;
    QLabel *statusLabel;
    
    int sockfd;
    QString currentUsername;
    bool isLoggedIn;
};

#endif
