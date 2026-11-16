#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(int sockfd, QWidget *parent = nullptr);
    QString getUsername() const { return username; }
    int getLoginResult() const { return loginResult; }

private slots:
    void onLoginClicked();
    void onCancelClicked();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginBtn;
    QPushButton *cancelBtn;
    QLabel *statusLabel;
    
    int sockfd;
    QString username;
    int loginResult;
};

#endif
