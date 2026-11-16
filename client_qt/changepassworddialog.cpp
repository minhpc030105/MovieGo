#include "changepassworddialog.h"
#include "responsecodes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

extern "C" {
    #include "../client/headers/menu.h"
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

ChangePasswordDialog::ChangePasswordDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd)
{
    setWindowTitle("Change Password");
    setModal(true);
    setMinimumWidth(400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Form layout
    QFormLayout *formLayout = new QFormLayout();
    
    oldPasswordEdit = new QLineEdit(this);
    newPasswordEdit = new QLineEdit(this);
    confirmPasswordEdit = new QLineEdit(this);
    
    oldPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    
    formLayout->addRow("Old Password:", oldPasswordEdit);
    formLayout->addRow("New Password:", newPasswordEdit);
    formLayout->addRow("Confirm New Password:", confirmPasswordEdit);
    
    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: red;");
    statusLabel->setAlignment(Qt::AlignCenter);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    changeBtn = new QPushButton("Change", this);
    cancelBtn = new QPushButton("Cancel", this);
    
    changeBtn->setDefault(true);
    
    buttonLayout->addWidget(changeBtn);
    buttonLayout->addWidget(cancelBtn);
    
    // Add to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(changeBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onChangeClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &ChangePasswordDialog::onCancelClicked);
}

void ChangePasswordDialog::onChangeClicked()
{
    QString oldPassword = oldPasswordEdit->text();
    QString newPassword = newPasswordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();
    
    if (oldPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
        statusLabel->setText("Please fill in all fields!");
        return;
    }
    
    if (newPassword != confirmPassword) {
        statusLabel->setText("New passwords do not match!");
        QMessageBox::warning(this, "Error", "New passwords do not match!");
        return;
    }
    
    // Send change password request to server
    char message[255];
    char username[255] = "";
    QByteArray oldPassBytes = oldPassword.toUtf8();
    QByteArray newPassBytes = newPassword.toUtf8();
    
    makeChangePasswordMessage(username, oldPassBytes.data(), newPassBytes.data(), message);
    sendMessage(sockfd, message);
    
    // Receive response
    int result = recvResult(sockfd);
    
    if (result == CHANGE_PASSWORD_SUCCESS) {
        QMessageBox::information(this, "Success", CHANGE_PASSWORD_SUCCESS_MESSAGE);
        accept();
    } else if (result == CHANGE_PASSWORD_FAIL) {
        statusLabel->setText(CHANGE_PASSWORD_FAIL_MESSAGE);
        QMessageBox::warning(this, "Change Password Failed", CHANGE_PASSWORD_FAIL_MESSAGE);
    } else {
        statusLabel->setText("Connection error!");
        QMessageBox::critical(this, "Error", "Failed to communicate with server!");
    }
}

void ChangePasswordDialog::onCancelClicked()
{
    reject();
}
