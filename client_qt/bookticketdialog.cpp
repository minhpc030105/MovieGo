#include "bookticketdialog.h"
#include "responsecodes.h"
#include <QByteArray>
#include <QVBoxLayout>
#include <QGridLayout>
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
    
    seatGridLayout = new QGridLayout();
    seatGridLayout->addWidget(new QLabel("", seatPage), 0, 0);
    for (int column = 1; column <= 10; ++column) {
        QLabel *columnLabel = new QLabel(QString::number(column), seatPage);
        columnLabel->setAlignment(Qt::AlignCenter);
        seatGridLayout->addWidget(columnLabel, 0, column);
    }
    const QString rows = "ABCDE";
    for (int row = 0; row < rows.size(); ++row) {
        QLabel *rowLabel = new QLabel(QString(rows.at(row)), seatPage);
        rowLabel->setAlignment(Qt::AlignCenter);
        seatGridLayout->addWidget(rowLabel, row + 1, 0);
        for (int column = 1; column <= 10; ++column) {
            QString seatLabel = QString("%1-%2").arg(rows.at(row)).arg(column);
            QPushButton *seatButton = new QPushButton(QString::number(column), seatPage);
            seatButton->setProperty("seatLabel", seatLabel);
            seatButton->setProperty("seatState", "BOOKED");
            seatButton->setEnabled(false);
            seatButton->setMinimumSize(40, 36);
            seatButton->setStyleSheet("background-color: #f44336; color: white;");
            seatGridLayout->addWidget(seatButton, row + 1, column);
            seatButtons.insert(seatLabel, seatButton);
            connect(seatButton, &QPushButton::clicked, this, [this, seatLabel]() {
                toggleSeatSelection(seatLabel);
            });
        }
    }

    selectedSeatsLabel = new QLabel("Selected seats: none", seatPage);
    selectedSeatsLabel->setAlignment(Qt::AlignCenter);
    
    QHBoxLayout *seatBtnLayout = new QHBoxLayout();
    backFromSeatBtn = new QPushButton("← Back", seatPage);
    selectSeatBtn = new QPushButton("Confirm Booking", seatPage);
    backFromSeatBtn->setMinimumHeight(40);
    selectSeatBtn->setMinimumHeight(40);
    selectSeatBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    seatBtnLayout->addWidget(backFromSeatBtn);
    seatBtnLayout->addWidget(selectSeatBtn);
    
    seatLayout->addWidget(seatLabel);
    seatLayout->addLayout(seatGridLayout);
    seatLayout->addWidget(selectedSeatsLabel);
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
    
    MovieResponse reply;
    int result = receive_response(sockfd, &reply);
    if (showQtResponseCodeMessage(this, result)) return;
    filmListWidget->clear();
    
    if (result != FIND_FILM_SUCCESS && result != NO_FILMS) {
        QMessageBox::warning(this, "Error", "Failed to load films!");
        return;
    }
    if (result == NO_FILMS) {
        QMessageBox::information(this, "Info", "No films available!");
        return;
    }
    
    for (char *row_text; (row_text = next_response_line(&reply)) != NULL;) {
        filmListWidget->addItem(QString::fromUtf8(row_text));
    }
}

void BookTicketDialog::loadCinemas(const QString &filmId)
{
    char message[2048];
    sprintf(message, "SHOW_CINEMA_BY_FILM\r\n%s\r\n", filmId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    MovieResponse reply;
    int result = receive_response(sockfd, &reply);
    if (showQtResponseCodeMessage(this, result)) return;
    cinemaListWidget->clear();
    
    if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
        QMessageBox::warning(this, "Error", "Failed to load cinemas!");
        return;
    }
    if (result == NO_CINEMAS) {
        QMessageBox::information(this, "Info", "No cinemas available for this film!");
        return;
    }
    
    for (char *row_text; (row_text = next_response_line(&reply)) != NULL;) {
        cinemaListWidget->addItem(QString::fromUtf8(row_text));
    }
}

void BookTicketDialog::loadShowtimes(const QString &filmId, const QString &cinemaId)
{
    char message[2048];
    sprintf(message, "SHOW_TIME_BY_FILM_CINEMA\r\n%s\r\n%s\r\n", 
            filmId.toUtf8().constData(), 
            cinemaId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    MovieResponse reply;
    int result = receive_response(sockfd, &reply);
    if (showQtResponseCodeMessage(this, result)) return;
    showtimeListWidget->clear();
    
    if (result != BROWSE_TIME_SUCCESS && result != NO_SHOWTIMES) {
        QMessageBox::warning(this, "Error", "Failed to load showtimes!");
        return;
    }
    if (result == NO_SHOWTIMES) {
        QMessageBox::information(this, "Info", "No showtimes available!");
        return;
    }
    
    for (char *row_text; (row_text = next_response_line(&reply)) != NULL;) {
        showtimeListWidget->addItem(QString::fromUtf8(row_text));
    }
}

void BookTicketDialog::loadSeats(const QString &showtimeId)
{
    resetSeatGrid();
    char message[2048];
    sprintf(message, "SHOW_SEATS\r\n%s\r\n", showtimeId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    MovieResponse reply;
    int result = receive_response(sockfd, &reply);
    if (showQtResponseCodeMessage(this, result)) return;
    
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
    bool malformedSeat = false;
    
    for (char *row_text; (row_text = next_response_line(&reply)) != NULL;) {
        QString msg = QString::fromUtf8(row_text);
        QStringList parts = msg.split("|");
        bool validId = false;
        if (parts.size() != 3 || parts[0] != "SEAT") {
            malformedSeat = true;
            continue;
        }
        parts[1].trimmed().toULongLong(&validId);
        QStringList seatParts = parts[2].trimmed().split("-");
        if (!validId || seatParts.size() != 2) {
            malformedSeat = true;
            continue;
        }

        int row = QString("ABCDE").indexOf(seatParts[0].trimmed());
        bool validColumn = false;
        int column = seatParts[1].trimmed().toInt(&validColumn);
        QString seatLabel = QString("%1-%2").arg(seatParts[0].trimmed()).arg(column);
        if (row < 0 || !validColumn || column < 1 || column > 10 ||
            !seatButtons.contains(seatLabel)) {
            continue;
        }

        QPushButton *button = seatButtons.value(seatLabel);
        button->setProperty("seatId", parts[1].trimmed());
        button->setProperty("seatState", "AVAILABLE");
        button->setEnabled(true);
        button->setStyleSheet("background-color: #eeeeee; color: black;");
        hasSeats = true;
    }

    if (malformedSeat) {
        resetSeatGrid();
        QMessageBox::warning(this, "Error", "The server returned invalid seat data.");
        return;
    }
    
    if (!hasSeats) {
        selectSeatBtn->setEnabled(false);
    } else {
        selectSeatBtn->setEnabled(true);
    }
}

void BookTicketDialog::resetSeatGrid()
{
    selectedSeatIds.clear();
    selectedSeatLabels.clear();
    for (QPushButton *button : seatButtons) {
        button->setProperty("seatId", QString());
        button->setProperty("seatState", "BOOKED");
        button->setEnabled(false);
        button->setStyleSheet("background-color: #f44336; color: white;");
    }
    updateSelectedSeatsLabel();
    selectSeatBtn->setEnabled(true);
}

void BookTicketDialog::updateSelectedSeatsLabel()
{
    selectedSeatsLabel->setText(selectedSeatLabels.isEmpty()
        ? "Selected seats: none"
        : "Selected seats: " + selectedSeatLabels.join(", "));
}

void BookTicketDialog::toggleSeatSelection(const QString &seatLabel)
{
    QPushButton *button = seatButtons.value(seatLabel, nullptr);
    if (!button) return;

    QString state = button->property("seatState").toString();
    if (state == "AVAILABLE") {
        if (selectedSeatIds.size() >= 10) {
            QMessageBox::warning(this, "Seat Limit",
                                 "You can select up to 10 seats.");
            return;
        }
        selectedSeatIds.append(button->property("seatId").toString());
        selectedSeatLabels.append(seatLabel);
        button->setProperty("seatState", "SELECTED");
        button->setStyleSheet("background-color: #4caf50; color: white;");
    } else if (state == "SELECTED") {
        int selectedIndex = selectedSeatLabels.indexOf(seatLabel);
        if (selectedIndex >= 0) {
            selectedSeatLabels.removeAt(selectedIndex);
            selectedSeatIds.removeAt(selectedIndex);
        }
        button->setProperty("seatState", "AVAILABLE");
        button->setStyleSheet("background-color: #eeeeee; color: black;");
    }
    updateSelectedSeatsLabel();
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
    if (selectedSeatIds.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a seat!");
        return;
    }

    if (selectedSeatIds.size() > 1) {
        QMessageBox::information(this, "Booking Not Available",
                                 "The server does not support booking multiple seats yet. "
                                 "Your selected seats are kept for the future booking flow.");
        return;
    }
    
    // Send booking request
    char message[2048];
    QByteArray showtimeId = selectedShowtimeId.toUtf8();
    QByteArray seatId = selectedSeatIds.first().toUtf8();
    makeBookTicketMessage(showtimeId.data(), seatId.data(), message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    
    if (result == BOOK_TICKET_SUCCESS) {
        QMessageBox::information(this, "Success", "Ticket booked successfully!");
        accept();
    } else if (showQtResponseCodeMessage(this, result)) {
        return;
    } else {
        QMessageBox::critical(this, "Failed", "Booking failed! Seat may already be booked.");
    }
}

void BookTicketDialog::onBackToStart()
{
    stackedWidget->setCurrentWidget(filmPage);
}
