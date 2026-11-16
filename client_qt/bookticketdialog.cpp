#include "bookticketdialog.h"
#include "responsecodes.h"
#include <QVBoxLayout>
#include <QMessageBox>

extern "C" {
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
    #include "../client/headers/menu.h"
}

BookTicketDialog::BookTicketDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd)
{
    setWindowTitle("Book Ticket");
    setMinimumSize(600, 500);
    setupUI();
    loadFilms();
}

BookTicketDialog::~BookTicketDialog()
{
}

void BookTicketDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    stackedWidget = new QStackedWidget(this);
    
    // ========== STEP 1: SELECT FILM ==========
    filmPage = new QWidget();
    QVBoxLayout *filmLayout = new QVBoxLayout(filmPage);
    
    QLabel *filmLabel = new QLabel("Step 1: Select a Film", filmPage);
    filmLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    filmLabel->setAlignment(Qt::AlignCenter);
    
    filmListWidget = new QListWidget(filmPage);
    selectFilmBtn = new QPushButton("Next →", filmPage);
    selectFilmBtn->setMinimumHeight(40);
    
    filmLayout->addWidget(filmLabel);
    filmLayout->addWidget(filmListWidget);
    filmLayout->addWidget(selectFilmBtn);
    
    // ========== STEP 2: SELECT CINEMA ==========
    cinemaPage = new QWidget();
    QVBoxLayout *cinemaLayout = new QVBoxLayout(cinemaPage);
    
    QLabel *cinemaLabel = new QLabel("Step 2: Select a Cinema", cinemaPage);
    cinemaLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    cinemaLabel->setAlignment(Qt::AlignCenter);
    
    cinemaListWidget = new QListWidget(cinemaPage);
    
    QHBoxLayout *cinemaBtnLayout = new QHBoxLayout();
    backFromCinemaBtn = new QPushButton("← Back", cinemaPage);
    selectCinemaBtn = new QPushButton("Next →", cinemaPage);
    backFromCinemaBtn->setMinimumHeight(40);
    selectCinemaBtn->setMinimumHeight(40);
    cinemaBtnLayout->addWidget(backFromCinemaBtn);
    cinemaBtnLayout->addWidget(selectCinemaBtn);
    
    cinemaLayout->addWidget(cinemaLabel);
    cinemaLayout->addWidget(cinemaListWidget);
    cinemaLayout->addLayout(cinemaBtnLayout);
    
    // ========== STEP 3: SELECT SHOWTIME ==========
    showtimePage = new QWidget();
    QVBoxLayout *showtimeLayout = new QVBoxLayout(showtimePage);
    
    QLabel *showtimeLabel = new QLabel("Step 3: Select a Showtime", showtimePage);
    showtimeLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    showtimeLabel->setAlignment(Qt::AlignCenter);
    
    showtimeListWidget = new QListWidget(showtimePage);
    
    QHBoxLayout *showtimeBtnLayout = new QHBoxLayout();
    backFromShowtimeBtn = new QPushButton("← Back", showtimePage);
    selectShowtimeBtn = new QPushButton("Next →", showtimePage);
    backFromShowtimeBtn->setMinimumHeight(40);
    selectShowtimeBtn->setMinimumHeight(40);
    showtimeBtnLayout->addWidget(backFromShowtimeBtn);
    showtimeBtnLayout->addWidget(selectShowtimeBtn);
    
    showtimeLayout->addWidget(showtimeLabel);
    showtimeLayout->addWidget(showtimeListWidget);
    showtimeLayout->addLayout(showtimeBtnLayout);
    
    // ========== STEP 4: SELECT SEAT ==========
    seatPage = new QWidget();
    QVBoxLayout *seatLayout = new QVBoxLayout(seatPage);
    
    QLabel *seatLabel = new QLabel("Step 4: Select a Seat", seatPage);
    seatLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    seatLabel->setAlignment(Qt::AlignCenter);
    
    seatListWidget = new QListWidget(seatPage);
    
    QHBoxLayout *seatBtnLayout = new QHBoxLayout();
    backFromSeatBtn = new QPushButton("← Back", seatPage);
    selectSeatBtn = new QPushButton("Confirm Booking", seatPage);
    backFromSeatBtn->setMinimumHeight(40);
    selectSeatBtn->setMinimumHeight(40);
    selectSeatBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    seatBtnLayout->addWidget(backFromSeatBtn);
    seatBtnLayout->addWidget(selectSeatBtn);
    
    seatLayout->addWidget(seatLabel);
    seatLayout->addWidget(seatListWidget);
    seatLayout->addLayout(seatBtnLayout);
    
    // Add pages to stacked widget
    stackedWidget->addWidget(filmPage);
    stackedWidget->addWidget(cinemaPage);
    stackedWidget->addWidget(showtimePage);
    stackedWidget->addWidget(seatPage);
    
    mainLayout->addWidget(stackedWidget);
    
    // Connect signals
    connect(selectFilmBtn, &QPushButton::clicked, this, &BookTicketDialog::onSelectFilm);
    connect(backFromCinemaBtn, &QPushButton::clicked, this, &BookTicketDialog::onBackToStart);
    connect(selectCinemaBtn, &QPushButton::clicked, this, &BookTicketDialog::onSelectCinema);
    connect(backFromShowtimeBtn, &QPushButton::clicked, [this]() { stackedWidget->setCurrentWidget(cinemaPage); });
    connect(selectShowtimeBtn, &QPushButton::clicked, this, &BookTicketDialog::onSelectShowtime);
    connect(backFromSeatBtn, &QPushButton::clicked, [this]() { stackedWidget->setCurrentWidget(showtimePage); });
    connect(selectSeatBtn, &QPushButton::clicked, this, &BookTicketDialog::onBookTicket);
}

void BookTicketDialog::loadFilms()
{
    char message[2048];
    sprintf(message, "SHOW_FILMS\r\n");
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    filmListWidget->clear();
    
    if (result != FIND_FILM_SUCCESS && result != NO_FILMS) {
        QMessageBox::warning(this, "Error", "Failed to load films!");
        return;
    }
    if (result == NO_FILMS) {
        QMessageBox::information(this, "Info", "No films available!");
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) break;
        filmListWidget->addItem(QString::fromUtf8(message));
    }
}

void BookTicketDialog::loadCinemas(const QString &filmId)
{
    char message[2048];
    sprintf(message, "SHOW_CINEMA_BY_FILM\r\n%s\r\n", filmId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    cinemaListWidget->clear();
    
    if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
        QMessageBox::warning(this, "Error", "Failed to load cinemas!");
        return;
    }
    if (result == NO_CINEMAS) {
        QMessageBox::information(this, "Info", "No cinemas available for this film!");
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) break;
        cinemaListWidget->addItem(QString::fromUtf8(message));
    }
}

void BookTicketDialog::loadShowtimes(const QString &filmId, const QString &cinemaId)
{
    char message[2048];
    sprintf(message, "SHOW_TIME_BY_FILM_CINEMA\r\n%s\r\n%s\r\n", 
            filmId.toUtf8().constData(), 
            cinemaId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    showtimeListWidget->clear();
    
    if (result != BROWSE_TIME_SUCCESS && result != NO_SHOWTIMES) {
        QMessageBox::warning(this, "Error", "Failed to load showtimes!");
        return;
    }
    if (result == NO_SHOWTIMES) {
        QMessageBox::information(this, "Info", "No showtimes available!");
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) break;
        showtimeListWidget->addItem(QString::fromUtf8(message));
    }
}

void BookTicketDialog::loadSeats(const QString &showtimeId)
{
    char message[2048];
    sprintf(message, "SHOW_SEATS\r\n%s\r\n", showtimeId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    seatListWidget->clear();
    
    if (result == INVALID_REQUEST || result == INVALID_SHOWTIME) {
        QMessageBox::warning(this, "Error", "Invalid showtime!");
        return;
    }
    if (result == NO_SEATS_AVAILABLE) {
        QMessageBox::information(this, "Info", "No seats available!");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        QMessageBox::warning(this, "Error", "Failed to load seats!");
        return;
    }
    
    bool hasSeats = false;
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) break;
        
        QString msg = QString::fromUtf8(message);
        if (msg.startsWith("SEAT|")) {
            hasSeats = true;
            QStringList parts = msg.split("|");
            if (parts.size() >= 3) {
                QString display = QString("ID: %1  -  Seat: %2")
                    .arg(parts[1].trimmed())
                    .arg(parts[2].trimmed());
                seatListWidget->addItem(display);
            }
        } else {
            seatListWidget->addItem(msg);
        }
    }
    
    if (!hasSeats) {
        seatListWidget->addItem("No available seats for this showtime!");
        selectSeatBtn->setEnabled(false);
    } else {
        selectSeatBtn->setEnabled(true);
    }
}

void BookTicketDialog::onSelectFilm()
{
    QListWidgetItem *item = filmListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Warning", "Please select a film!");
        return;
    }
    
    QString selectedText = item->text();
    QStringList parts = selectedText.split(".");
    if (parts.isEmpty()) return;
    
    selectedFilmId = parts[0].trimmed();
    
    loadCinemas(selectedFilmId);
    stackedWidget->setCurrentWidget(cinemaPage);
}

void BookTicketDialog::onSelectCinema()
{
    QListWidgetItem *item = cinemaListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Warning", "Please select a cinema!");
        return;
    }
    
    QString selectedText = item->text();
    QStringList parts = selectedText.split(".");
    if (parts.isEmpty()) return;
    
    selectedCinemaId = parts[0].trimmed();
    
    loadShowtimes(selectedFilmId, selectedCinemaId);
    stackedWidget->setCurrentWidget(showtimePage);
}

void BookTicketDialog::onSelectShowtime()
{
    QListWidgetItem *item = showtimeListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Warning", "Please select a showtime!");
        return;
    }
    
    QString selectedText = item->text();
    QStringList parts = selectedText.split(".");
    if (parts.isEmpty()) return;
    
    selectedShowtimeId = parts[0].trimmed();
    
    loadSeats(selectedShowtimeId);
    stackedWidget->setCurrentWidget(seatPage);
}

void BookTicketDialog::onBookTicket()
{
    QListWidgetItem *item = seatListWidget->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Warning", "Please select a seat!");
        return;
    }
    
    QString selectedText = item->text();
    
    QStringList parts = selectedText.split("-");
    if (parts.isEmpty()) return;
    
    QString idPart = parts[0].trimmed();
    QStringList idParts = idPart.split(":");
    if (idParts.size() < 2) return;
    
    selectedSeatId = idParts[1].trimmed();
    
    // Send booking request
    char message[2048];
    sprintf(message, "BOOK_TICKET\r\n%s\r\n%s\r\n", 
            selectedShowtimeId.toUtf8().constData(),
            selectedSeatId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    
    if (result == BOOK_TICKET_SUCCESS) {
        QMessageBox::information(this, "Success", "Ticket booked successfully!");
        accept();
    } else {
        QMessageBox::critical(this, "Failed", "Booking failed! Seat may already be booked.");
    }
}

void BookTicketDialog::onBackToStart()
{
    stackedWidget->setCurrentWidget(filmPage);
}
