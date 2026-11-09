#include "viewticketsdialog.h"
#include "responsecodes.h"
#include "viewticketdetaildialog.h"

extern "C" {
    #include "../lib/socket/socket.h"
}

ViewTicketsDialog::ViewTicketsDialog(int sockfd, const QString &username, QWidget *parent)
    : QDialog(parent), sockfd(sockfd), username(username)
{
    setWindowTitle("My Tickets");
    setMinimumSize(900, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("My Booked Tickets", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Status label
    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: blue; margin: 5px;");
    
    // Table
    ticketsTable = new QTableWidget(this);
    ticketsTable->setColumnCount(7);
    ticketsTable->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Film" << "Cinema" << "Room" << "Seat" << "Start Time" << "Booked At");
    
    ticketsTable->horizontalHeader()->setStretchLastSection(true);
    ticketsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ticketsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ticketsTable->setAlternatingRowColors(true);
    
    // Adjust column widths
    ticketsTable->setColumnWidth(0, 50);   // ID
    ticketsTable->setColumnWidth(1, 200);  // Film
    ticketsTable->setColumnWidth(2, 150);  // Cinema
    ticketsTable->setColumnWidth(3, 100);  // Room
    ticketsTable->setColumnWidth(4, 80);   // Seat
    ticketsTable->setColumnWidth(5, 150);  // Start Time
    
    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    refreshBtn = new QPushButton("Refresh", this);
    closeBtn = new QPushButton("Close", this);
    
    refreshBtn->setMinimumHeight(35);
    closeBtn->setMinimumHeight(35);
    
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    
    // Add to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(ticketsTable);
    mainLayout->addLayout(btnLayout);
    
    // Connect signals
    connect(refreshBtn, &QPushButton::clicked, this, &ViewTicketsDialog::onRefreshClicked);
    connect(closeBtn, &QPushButton::clicked, this, &ViewTicketsDialog::onCloseClicked);
    connect(ticketsTable, &QTableWidget::cellDoubleClicked, this, &ViewTicketsDialog::onTicketDoubleClicked);
    
    // Load tickets initially
    loadTickets();
}

ViewTicketsDialog::~ViewTicketsDialog()
{
}

void ViewTicketsDialog::loadTickets()
{
    // Clear table
    ticketsTable->setRowCount(0);
    statusLabel->setText("Loading tickets...");
    
    // Send VIEW_TICKETS request
    char message[512];
    sprintf(message, "VIEW_TICKETS\r\n%s\r\n", username.toUtf8().constData());
    sendMessage(sockfd, message);
    
    // Receive result code
    int result = recvResult(sockfd);
    
    if (result == NO_TICKETS) {
        statusLabel->setText("No tickets found.");
        return;
    }
    if (result == FAILED_RETRIEVE) {
        statusLabel->setText("Failed to retrieve tickets.");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        statusLabel->setText("Error loading tickets!");
        return;
    }
    
    // Receive data
    int rowCount = 0;
    bool firstLine = true;
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        
        QString response = QString::fromUtf8(message);
        
        if (response == "END") {
            break;
        }
        
        // Skip header line
        if (firstLine) {
            firstLine = false;
            continue;
        }
        
        // Parse response
        // Format: ID | Film | Cinema | Room | Seat | Start Time | Booked At
        if (response.contains("|")) {
            QStringList parts = response.split("|");
            if (parts.size() >= 7) {
                ticketsTable->insertRow(rowCount);
                
                for (int i = 0; i < 7 && i < parts.size(); i++) {
                    QTableWidgetItem *item = new QTableWidgetItem(parts[i].trimmed());
                    item->setTextAlignment(Qt::AlignCenter);
                    ticketsTable->setItem(rowCount, i, item);
                }
                
                rowCount++;
            }
        } else if (response.contains("No tickets found")) {
            statusLabel->setText("You haven't booked any tickets yet.");
            return;
        } else if (response.contains("total:")) {
            statusLabel->setText(response);
        }
    }
    
    if (rowCount > 0) {
        statusLabel->setText(QString("Total: %1 ticket(s)").arg(rowCount));
    }
}

void ViewTicketsDialog::onRefreshClicked()
{
    loadTickets();
}

void ViewTicketsDialog::onCloseClicked()
{
    accept();
}

void ViewTicketsDialog::onTicketDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    
    // Get ticket ID from first column
    QTableWidgetItem *idItem = ticketsTable->item(row, 0);
    if (idItem) {
        QString ticketId = idItem->text().trimmed();
        
        // Open detail dialog
        ViewTicketDetailDialog detailDialog(sockfd, ticketId, this);
        detailDialog.exec();
    }
}
