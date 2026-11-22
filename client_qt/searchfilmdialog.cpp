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
    QByteArray titleBytes = title.toUtf8();
    if (titleBytes.size() > 255) {
        QMessageBox::warning(this, "Search", "Film title must be at most 255 UTF-8 bytes.");
        return;
    }
    makeSearchFilmByTitleMessage(titleBytes.data(), message);
    sendMessage(sockfd, message);
    
    MovieResponse reply;
    int result = receive_response(sockfd, &reply);
    if (showQtResponseCodeMessage(this, result)) return;
    resultListWidget->clear();
    
    if (result == FIND_FILM_SUCCESS) {
        for (char *row_text; (row_text = next_response_line(&reply)) != NULL;) {
            resultListWidget->addItem(QString::fromUtf8(row_text));
        }
        
        if (resultListWidget->count() == 0) {
            resultListWidget->addItem("No films found!");
        }
    } else {
        resultListWidget->addItem("No films found!");
    }
}
