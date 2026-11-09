#include "searchfilmdialog.h"
#include "responsecodes.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

extern "C" {
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
    #include "../client/headers/menu.h"
}

SearchFilmDialog::SearchFilmDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd)
{
    setWindowTitle("Search Film");
    setMinimumSize(600, 400);
    setupUI();
}

SearchFilmDialog::~SearchFilmDialog()
{
}

void SearchFilmDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *titleLabel = new QLabel("Search Film by Title", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *searchLayout = new QHBoxLayout();
    
    QLabel *inputLabel = new QLabel("Film Title:", this);
    titleLineEdit = new QLineEdit(this);
    titleLineEdit->setPlaceholderText("Enter film title...");
    titleLineEdit->setMinimumHeight(30);
    
    searchBtn = new QPushButton("Search", this);
    searchBtn->setMinimumHeight(30);
    searchBtn->setMinimumWidth(100);
    
    searchLayout->addWidget(inputLabel);
    searchLayout->addWidget(titleLineEdit);
    searchLayout->addWidget(searchBtn);
    
    QLabel *resultLabel = new QLabel("Search Results:", this);
    resultLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    
    resultListWidget = new QListWidget(this);
    
    QPushButton *closeBtn = new QPushButton("Close", this);
    closeBtn->setMinimumHeight(35);
    
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(resultLabel);
    mainLayout->addWidget(resultListWidget);
    mainLayout->addWidget(closeBtn);
    
    connect(searchBtn, &QPushButton::clicked, this, &SearchFilmDialog::onSearchClicked);
    connect(titleLineEdit, &QLineEdit::returnPressed, this, &SearchFilmDialog::onSearchClicked);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void SearchFilmDialog::onSearchClicked()
{
    QString title = titleLineEdit->text().trimmed();
    
    if (title.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a film title!");
        return;
    }
    
    char message[1024];
    sprintf(message, "SEARCH_FILM_BY_TITLE\r\n%s\r\n", title.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    resultListWidget->clear();
    
    if (result == FIND_FILM_SUCCESS) {
        while (true) {
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if (strcmp(message, "END") == 0) break;
            resultListWidget->addItem(QString::fromUtf8(message));
        }
        
        if (resultListWidget->count() == 0) {
            resultListWidget->addItem("No films found!");
        }
    } else {
        resultListWidget->addItem("No films found!");
    }
}
