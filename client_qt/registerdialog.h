#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(int sockfd, QWidget *parent = nullptr);

private slots:
    void onRegisterClicked();
    void onCancelClicked();

private:
    QLineEdit *fullnameEdit;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;
    QPushButton *registerBtn;
    QPushButton *cancelBtn;
    QLabel *statusLabel;
    
    int sockfd;
};

#endif // REGISTERDIALOG_H
