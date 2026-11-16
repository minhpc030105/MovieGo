#include "logindialog.h"
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

LoginDialog::LoginDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd), loginResult(-1)
{
    setWindowTitle("Login");
    setModal(true);
    setMinimumWidth(400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Form layout
    QFormLayout *formLayout = new QFormLayout();
    
    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    
    formLayout->addRow("Username:", usernameEdit);
    formLayout->addRow("Password:", passwordEdit);
    
    statusLabel = new QLabel("", this);
    statusLabel->setStyleSheet("color: red;");
    statusLabel->setAlignment(Qt::AlignCenter);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    loginBtn = new QPushButton("Login", this);
    cancelBtn = new QPushButton("Cancel", this);
    
    loginBtn->setDefault(true);
    
    buttonLayout->addWidget(loginBtn);
    buttonLayout->addWidget(cancelBtn);
    
    // Add to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
}

void LoginDialog::onLoginClicked()
{
    QString user = usernameEdit->text().trimmed();
    QString pass = passwordEdit->text();
    
    if (user.isEmpty() || pass.isEmpty()) {
        statusLabel->setText("Please fill in all fields!");
        return;
    }
    
    // Send login request to server
    char message[255];
    QByteArray userBytes = user.toUtf8();
    QByteArray passBytes = pass.toUtf8();
    
    makeLoginMessage(userBytes.data(), passBytes.data(), message);
    sendMessage(sockfd, message);
    
    // Receive response
    int result = recvResult(sockfd);
    loginResult = result;
    
    if (result == LOGIN_SUCCESS_USER || result == LOGIN_SUCCESS_MANAGER || result == LOGIN_SUCCESS_ADMIN) {
        username = user;
        accept();
    } else if (result == LOGIN_FAIL) {
        statusLabel->setText(LOGIN_FAIL_MESSAGE);
        QMessageBox::warning(this, "Login Failed", LOGIN_FAIL_MESSAGE);
    } else if (result == LOGIN_ALREADY) {
        statusLabel->setText(LOGIN_ALREADY_MESSAGE);
        QMessageBox::warning(this, "Login Failed", LOGIN_ALREADY_MESSAGE);
    } else {
        statusLabel->setText("Connection error!");
        QMessageBox::critical(this, "Error", "Failed to communicate with server!");
    }
}

void LoginDialog::onCancelClicked()
{
    reject();
}
