#ifndef ADMINDIALOG_H
#define ADMINDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>

class AdminDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdminDialog(int sockfd, const QString &username, QWidget *parent = nullptr);

private slots:
    void onShowAllUsersClicked();
    void onDeleteUserClicked();
    void onChangeUserRoleClicked();
    void onRefreshClicked();
    void onLogoutClicked();

private:
    void loadAllUsers();
    void clearUserTable();
    
    int sockfd;
    QString username;
    
    QLabel *titleLabel;
    QLabel *userLabel;
    QTableWidget *userTable;
    QPushButton *showAllUsersBtn;
    QPushButton *deleteUserBtn;
    QPushButton *changeRoleBtn;
    QPushButton *refreshBtn;
    QPushButton *logoutBtn;
};

#endif
