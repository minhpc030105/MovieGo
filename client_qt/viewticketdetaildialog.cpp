#include "viewticketdetaildialog.h"
#include "responsecodes.h"

extern "C" {
    #include "../lib/socket/socket.h"
    
}

ViewTicketDetailDialog::ViewTicketDetailDialog(int sockfd, const QString &ticketId, QWidget *parent)
    : QDialog(parent), sockfd(sockfd), ticketId(ticketId)
{
    setWindowTitle("Ticket Detail");
    setMinimumSize(600, 450);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("=== TICKET DETAIL ===", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Create group box for ticket info
    QGroupBox *infoGroup = new QGroupBox("Ticket Information", this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    // Info labels
    customerLabel = new QLabel("", this);
    filmLabel = new QLabel("", this);
    cinemaLabel = new QLabel("", this);
    roomLabel = new QLabel("", this);
    showtimeLabel = new QLabel("", this);
    bookedLabel = new QLabel("", this);
    
    // Style labels
    QString labelStyle = "font-size: 13px; padding: 8px; margin: 3px;";
    customerLabel->setStyleSheet(labelStyle);
    filmLabel->setStyleSheet(labelStyle);
    cinemaLabel->setStyleSheet(labelStyle);
    roomLabel->setStyleSheet(labelStyle);
    showtimeLabel->setStyleSheet(labelStyle);
    bookedLabel->setStyleSheet(labelStyle);
    
    customerLabel->setWordWrap(true);
    filmLabel->setWordWrap(true);
    cinemaLabel->setWordWrap(true);
    roomLabel->setWordWrap(true);
    showtimeLabel->setWordWrap(true);
    bookedLabel->setWordWrap(true);
    
    // Add to info layout
    infoLayout->addWidget(customerLabel);
    infoLayout->addWidget(filmLabel);
    infoLayout->addWidget(cinemaLabel);
    infoLayout->addWidget(roomLabel);
    infoLayout->addWidget(showtimeLabel);
    infoLayout->addWidget(bookedLabel);
    infoLayout->addStretch();
    
    // Close button
    closeBtn = new QPushButton("Close", this);
    closeBtn->setMinimumHeight(35);
    
    // Add to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(closeBtn);
    
    // Connect signals
    connect(closeBtn, &QPushButton::clicked, this, &ViewTicketDetailDialog::onCloseClicked);
    
    // Load ticket detail
    loadTicketDetail();
}

ViewTicketDetailDialog::~ViewTicketDetailDialog()
{
}

void ViewTicketDetailDialog::loadTicketDetail()
{
    // Send VIEW_TICKET_DETAIL request
    char message[512];
    sprintf(message, "VIEW_TICKET_DETAIL\r\n%s\r\n", ticketId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    // Receive result code
    int result = recvResult(sockfd);
    
    if (result == TICKET_NOT_FOUND) {
        QMessageBox::warning(this, "Error", "Ticket not found!");
        customerLabel->setText("Ticket not found!");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        QMessageBox::warning(this, "Error", "Failed to load ticket detail!");
        return;
    }
    
    // Receive response data
    QString customer, film, cinema, room, showtime, booked;
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        
        QString response = QString::fromUtf8(message);
        
        if (response == "END") {
            break;
        }
        
        // Parse different lines
        if (response.contains("TICKET DETAIL")) {
            // Skip header
            continue;
        } else if (response.contains("Customer:")) {
            customer = response;
        } else if (response.contains("Film:")) {
            film = response;
        } else if (response.contains("Cinema:")) {
            cinema = response;
        } else if (response.contains("Room:")) {
            room = response;
        } else if (response.contains("Showtime:")) {
            showtime = response;
        } else if (response.contains("Booked at:")) {
            booked = response;
        }
    }
    
    // Display information
    customerLabel->setText(customer.isEmpty() ? "Customer: N/A" : customer);
    filmLabel->setText(film.isEmpty() ? "Film: N/A" : film);
    cinemaLabel->setText(cinema.isEmpty() ? "Cinema: N/A" : cinema);
    roomLabel->setText(room.isEmpty() ? "Room: N/A" : room);
    showtimeLabel->setText(showtime.isEmpty() ? "Showtime: N/A" : showtime);
    bookedLabel->setText(booked.isEmpty() ? "Booked at: N/A" : booked);
}

void ViewTicketDetailDialog::onCloseClicked()
{
    accept();
}
