#ifndef CHANGEPASSWORDDIALOG_H
#define CHANGEPASSWORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ChangePasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangePasswordDialog(int sockfd, QWidget *parent = nullptr);

private slots:
    void onChangeClicked();
    void onCancelClicked();

private:
    QLineEdit *oldPasswordEdit;
    QLineEdit *newPasswordEdit;
    QLineEdit *confirmPasswordEdit;
    QPushButton *changeBtn;
    QPushButton *cancelBtn;
    QLabel *statusLabel;
    
    int sockfd;
};

#endif
