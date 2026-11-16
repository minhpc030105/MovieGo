#include "registerdialog.h"
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

RegisterDialog::RegisterDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd)
{
    setWindowTitle("Register");
    setModal(true);
    setMinimumWidth(400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Form layout
    QFormLayout *formLayout = new QFormLayout();
    
    fullnameEdit = new QLineEdit(this);
    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    confirmPasswordEdit = new QLineEdit(this);
    
    passwordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    
    formLayout->addRow("Full Name:", fullnameEdit);
    formLayout->addRow("Username:", usernameEdit);
    formLayout->addRow("Password:", passwordEdit);
    formLayout->addRow("Confirm Password:", confirmPasswordEdit);
    
    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: red;");
    statusLabel->setAlignment(Qt::AlignCenter);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    registerBtn = new QPushButton("Register", this);
    cancelBtn = new QPushButton("Cancel", this);
    
    registerBtn->setDefault(true);
    
    buttonLayout->addWidget(registerBtn);
    buttonLayout->addWidget(cancelBtn);
    
    // Add to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(registerBtn, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &RegisterDialog::onCancelClicked);
}

void RegisterDialog::onRegisterClicked()
{
    QString fullname = fullnameEdit->text().trimmed();
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();
    
    if (fullname.isEmpty() || username.isEmpty() || password.isEmpty()) {
        statusLabel->setText("Please fill in all fields!");
        return;
    }
    
    if (password != confirmPassword) {
        statusLabel->setText("Passwords do not match!");
        QMessageBox::warning(this, "Error", "Passwords do not match!");
        return;
    }
    
    // Send register request to server
    char message[255];
    QByteArray nameBytes = fullname.toUtf8();
    QByteArray userBytes = username.toUtf8();
    QByteArray passBytes = password.toUtf8();
    
    makeRegisterMessage(nameBytes.data(), userBytes.data(), passBytes.data(), message);
    sendMessage(sockfd, message);
    
    // Receive response
    int result = recvResult(sockfd);
    
    if (result == REGISTER_SUCCESS) {
        QMessageBox::information(this, "Success", REGISTER_SUCCESS_MESSAGE);
        accept();
    } else if (result == REGISTER_FAIL) {
        statusLabel->setText(REGISTER_FAIL_MESSAGE);
        QMessageBox::warning(this, "Registration Failed", REGISTER_FAIL_MESSAGE);
    } else {
        statusLabel->setText("Connection error!");
        QMessageBox::critical(this, "Error", "Failed to communicate with server!");
    }
}

void RegisterDialog::onCancelClicked()
{
    reject();
}
