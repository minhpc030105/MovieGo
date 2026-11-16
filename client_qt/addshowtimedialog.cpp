#include "addshowtimedialog.h"
#include "responsecodes.h"
#include <QDebug>

extern "C" {
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

AddShowTimeDialog::AddShowTimeDialog(int sockfd, QWidget *parent)
    : QDialog(parent), sockfd(sockfd)
{
    setWindowTitle("Add Show Time - MovieGo");
    setModal(true);
    setMinimumSize(900, 700);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    titleLabel = new QLabel("ADD SHOW TIME", this);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #5bc0de; margin: 15px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Form section
    QGroupBox *formGroup = new QGroupBox("Show Time Information", this);
    formGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; margin-top: 10px; } "
                            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QFormLayout *formLayout = new QFormLayout(formGroup);
    
    // Film selection
    filmCombo = new QComboBox(this);
    filmCombo->setMinimumHeight(35);
    filmCombo->setStyleSheet("QComboBox { padding: 5px; font-size: 13px; }");
    formLayout->addRow("Select Film:", filmCombo);
    
    // Cinema selection
    cinemaCombo = new QComboBox(this);
    cinemaCombo->setMinimumHeight(35);
    cinemaCombo->setStyleSheet("QComboBox { padding: 5px; font-size: 13px; }");
    cinemaCombo->setEnabled(false);
    formLayout->addRow("Select Cinema:", cinemaCombo);
    
    // Room selection
    roomCombo = new QComboBox(this);
    roomCombo->setMinimumHeight(35);
    roomCombo->setStyleSheet("QComboBox { padding: 5px; font-size: 13px; }");
    roomCombo->setEnabled(false);
    formLayout->addRow("Select Room:", roomCombo);
    
    // DateTime picker
    datetimeEdit = new QDateTimeEdit(this);
    datetimeEdit->setDateTime(QDateTime::currentDateTime().addDays(1));
    datetimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    datetimeEdit->setCalendarPopup(true);
    datetimeEdit->setMinimumHeight(35);
    datetimeEdit->setStyleSheet("QDateTimeEdit { padding: 5px; font-size: 13px; }");
    formLayout->addRow("Start Time:", datetimeEdit);
    
    formLayout->setSpacing(15);
    formLayout->setContentsMargins(20, 20, 20, 20);
    
    // Current showtimes section
    QGroupBox *showtimeGroup = new QGroupBox("Current Showtimes in Selected Room", this);
    showtimeGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; margin-top: 10px; } "
                                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");
    QVBoxLayout *showtimeLayout = new QVBoxLayout(showtimeGroup);
    
    showtimeTable = new QTableWidget(this);
    showtimeTable->setColumnCount(3);
    showtimeTable->setHorizontalHeaderLabels({"Film", "Start Time", "End Time"});
    showtimeTable->horizontalHeader()->setStretchLastSection(true);
    showtimeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    showtimeTable->setAlternatingRowColors(true);
    showtimeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    showtimeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    showtimeTable->setStyleSheet("QTableWidget { font-size: 12px; }");
    showtimeTable->setMinimumHeight(200);
    
    showtimeLayout->addWidget(showtimeTable);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    addBtn = new QPushButton("Add Show Time", this);
    addBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 12px 25px; "
                         "font-weight: bold; font-size: 14px; border-radius: 5px;");
    addBtn->setMinimumHeight(45);
    addBtn->setEnabled(false);
    
    refreshBtn = new QPushButton("Refresh", this);
    refreshBtn->setStyleSheet("background-color: #5bc0de; color: white; padding: 12px 25px; "
                             "font-weight: bold; font-size: 14px; border-radius: 5px;");
    refreshBtn->setMinimumHeight(45);
    
    cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setStyleSheet("background-color: #777; color: white; padding: 12px 25px; "
                            "font-weight: bold; font-size: 14px; border-radius: 5px;");
    cancelBtn->setMinimumHeight(45);
    
    buttonLayout->addWidget(addBtn);
    buttonLayout->addWidget(refreshBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn);
    
    // Add all to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(formGroup);
    mainLayout->addWidget(showtimeGroup);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(buttonLayout);
    
    mainLayout->setContentsMargins(20, 15, 20, 15);
    mainLayout->setSpacing(15);
    
    // Connect signals
    connect(filmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &AddShowTimeDialog::onFilmSelected);
    connect(cinemaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &AddShowTimeDialog::onCinemaSelected);
    connect(roomCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &AddShowTimeDialog::onRoomSelected);
    connect(addBtn, &QPushButton::clicked, this, &AddShowTimeDialog::onAddShowTimeClicked);
    connect(refreshBtn, &QPushButton::clicked, this, &AddShowTimeDialog::onRefreshClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    // Load initial data
    loadFilms();
}

void AddShowTimeDialog::loadFilms()
{
    filmCombo->clear();
    filmCombo->addItem("-- Select a Film --", "");
    
    char message[2048];
    memset(message, 0, sizeof(message));
    sprintf(message, "SHOW_FILMS\r\n");
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
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
        QString line = QString::fromUtf8(message).trimmed();
        
        if (line == "END" || line.isEmpty()) {
            break;
        }
        
        QStringList parts = line.split(". ");
        if (parts.size() >= 2) {
            QString id = parts[0].trimmed();
            QString title = parts[1].trimmed();
            
            filmCombo->addItem(title, id);
        }
    }
    
    if (filmCombo->count() == 1) {
        QMessageBox::warning(this, "No Films", "No films available. Please add films first.");
    }
}

void AddShowTimeDialog::loadCinemas()
{
    cinemaCombo->clear();
    cinemaCombo->addItem("-- Select a Cinema --", "");
    
    char message[2048];
    memset(message, 0, sizeof(message));
    sprintf(message, "SHOW_CINEMAS\r\n");
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
        QMessageBox::warning(this, "Error", "Failed to load cinemas!");
        return;
    }
    if (result == NO_CINEMAS) {
        QMessageBox::information(this, "Info", "No cinemas available!");
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        QString line = QString::fromUtf8(message).trimmed();
        
        if (line == "END" || line.isEmpty()) {
            break;
        }
        
        QStringList parts = line.split(". ");
        if (parts.size() >= 2) {
            QString id = parts[0].trimmed();
            QString name = parts[1].trimmed();
            
            cinemaCombo->addItem(name, id);
        }
    }
    
    cinemaCombo->setEnabled(cinemaCombo->count() > 1);
}

void AddShowTimeDialog::loadRooms(const QString &cinemaId)
{
    roomCombo->clear();
    roomCombo->addItem("-- Select a Room --", "");
    clearShowtimes();
    
    if (cinemaId.isEmpty()) {
        roomCombo->setEnabled(false);
        return;
    }
    
    char message[2048];
    memset(message, 0, sizeof(message));
    sprintf(message, "SHOW_ROOMS_BY_CINEMA\r\n%s\r\n", cinemaId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result == INVALID_CINEMA) {
        QMessageBox::warning(this, "Error", "Invalid cinema ID!");
        return;
    }
    if (result == NO_ROOMS || result == FAILED_ROOMS) {
        QMessageBox::information(this, "Info", "No rooms available in this cinema!");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        QMessageBox::warning(this, "Error", "Failed to load rooms!");
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        QString line = QString::fromUtf8(message).trimmed();
        
        if (line == "END" || line.isEmpty()) {
            break;
        }
        
        QStringList parts = line.split(" | ");
        if (parts.size() >= 2) {
            QString idPart = parts[0].trimmed();
            QString namePart = parts[1].trimmed();
            
            if (idPart.contains(": ")) {
                QString id = idPart.split(": ")[1].trimmed();
                roomCombo->addItem(namePart, id);
            }
        }
    }
    
    roomCombo->setEnabled(roomCombo->count() > 1);
}

void AddShowTimeDialog::loadShowtimes(const QString &roomId)
{
    clearShowtimes();
    
    if (roomId.isEmpty()) {
        return;
    }
    
    char message[2048];
    memset(message, 0, sizeof(message));
    sprintf(message, "SHOW_SHOWTIMES_BY_ROOM\r\n%s\r\n", roomId.toUtf8().constData());
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result == SHOW_TIME_FAIL) {
        int row = showtimeTable->rowCount();
        showtimeTable->insertRow(row);
        
        QTableWidgetItem *item = new QTableWidgetItem("No showtimes in this room yet.");
        item->setTextAlignment(Qt::AlignCenter);
        item->setForeground(QBrush(Qt::gray));
        showtimeTable->setItem(row, 0, item);
        showtimeTable->setSpan(row, 0, 1, 3);
        return;
    }
    if (result != SHOW_SHOWTIME_SUCCESS) {
        return;
    }
    
    while (true) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        QString line = QString::fromUtf8(message).trimmed();
        
        if (line == "END" || line.isEmpty()) {
            break;
        }
        
        int row = showtimeTable->rowCount();
        showtimeTable->insertRow(row);
        
        QStringList parts = line.split(" | ");
        if (parts.size() >= 3) {
            QString title = parts[1].replace("Title: ", "").trimmed();
            QString times = parts[2].trimmed();
            
            QStringList timeParts = times.split(" - ");
            QString start = timeParts.size() > 0 ? timeParts[0].trimmed() : "";
            QString endPart = timeParts.size() > 1 ? timeParts[1] : "";
            QString end = endPart.split(" (").size() > 0 ? endPart.split(" (")[0].trimmed() : "";
            
            showtimeTable->setItem(row, 0, new QTableWidgetItem(title));
            showtimeTable->setItem(row, 1, new QTableWidgetItem(start));
            showtimeTable->setItem(row, 2, new QTableWidgetItem(end));
        }
    }
}

void AddShowTimeDialog::clearShowtimes()
{
    showtimeTable->setRowCount(0);
}

void AddShowTimeDialog::onFilmSelected(int index)
{
    if (index > 0) {
        selectedFilmId = filmCombo->currentData().toString();
        loadCinemas();
    } else {
        selectedFilmId.clear();
        cinemaCombo->clear();
        cinemaCombo->setEnabled(false);
        roomCombo->clear();
        roomCombo->setEnabled(false);
        clearShowtimes();
        addBtn->setEnabled(false);
    }
}

void AddShowTimeDialog::onCinemaSelected(int index)
{
    if (index > 0) {
        selectedCinemaId = cinemaCombo->currentData().toString();
        loadRooms(selectedCinemaId);
    } else {
        selectedCinemaId.clear();
        roomCombo->clear();
        roomCombo->setEnabled(false);
        clearShowtimes();
        addBtn->setEnabled(false);
    }
}

void AddShowTimeDialog::onRoomSelected(int index)
{
    if (index > 0) {
        selectedRoomId = roomCombo->currentData().toString();
        loadShowtimes(selectedRoomId);
        addBtn->setEnabled(true);
    } else {
        selectedRoomId.clear();
        clearShowtimes();
        addBtn->setEnabled(false);
    }
}

void AddShowTimeDialog::onAddShowTimeClicked()
{
    if (selectedFilmId.isEmpty() || selectedCinemaId.isEmpty() || selectedRoomId.isEmpty()) {
        QMessageBox::warning(this, "Invalid Selection", 
                           "Please select film, cinema, and room!");
        return;
    }
    
    QString datetime = datetimeEdit->dateTime().toString("yyyy-MM-dd HH:mm:ss");
    
    // Debug: Show what we're sending
    qDebug() << "Adding showtime:";
    qDebug() << "  Film ID:" << selectedFilmId;
    qDebug() << "  Cinema ID:" << selectedCinemaId;
    qDebug() << "  Room ID:" << selectedRoomId;
    qDebug() << "  DateTime:" << datetime;
    
    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirm Add Show Time",
        QString("Are you sure you want to add this show time?\n\n"
                "Film: %1\n"
                "Cinema: %2\n"
                "Room: %3\n"
                "Start Time: %4")
            .arg(filmCombo->currentText())
            .arg(cinemaCombo->currentText())
            .arg(roomCombo->currentText())
            .arg(datetime),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Send request to server
    char message[2048];
    memset(message, 0, sizeof(message));
    sprintf(message, "ADD_SHOWTIME\r\n%s\r\n%s\r\n%s\r\n%s\r\n", 
            selectedFilmId.toUtf8().constData(),
            selectedCinemaId.toUtf8().constData(),
            selectedRoomId.toUtf8().constData(),
            datetime.toUtf8().constData());
    
    qDebug() << "Sending message:" << message;
    
    sendMessage(sockfd, message);
    
    // Receive response
    int result = recvResult(sockfd);
    
    qDebug() << "Server response code:" << result;
    
    if (result == ADD_SHOWTIME_SUCCESS) {
        QMessageBox::information(this, "Success", 
                               "Show time added successfully!");
        // Refresh showtimes table
        qDebug() << "Refreshing showtimes for room:" << selectedRoomId;
        loadShowtimes(selectedRoomId);
    } else {
        QString errorMsg;
        switch(result) {
            case INVALID_FILM_ID:
                errorMsg = "Invalid film ID!";
                break;
            case DB_QUERY_ERROR:
                errorMsg = "Database error!";
                break;
            case INVALID_CINEMA_ID:
                errorMsg = "Invalid cinema ID!";
                break;
            case INVALID_ROOM_ID:
                errorMsg = "Invalid room ID or room is not active!";
                break;
            case ROOM_NOT_BELONG_CINEMA:
                errorMsg = "Room does not belong to the selected cinema!";
                break;
            case INVALID_DATETIME_FORMAT:
                errorMsg = "Invalid datetime format!";
                break;
            case SCHEDULING_CONFLICT:
                errorMsg = "Scheduling conflict! There is another showtime in this room at that time.";
                break;
            default:
                errorMsg = "Failed to add showtime!";
        }
        QMessageBox::critical(this, "Error", errorMsg);
    }
}

void AddShowTimeDialog::onRefreshClicked()
{
    // Save current selections
    QString currentFilm = selectedFilmId;
    QString currentCinema = selectedCinemaId;
    QString currentRoom = selectedRoomId;
    
    // Reload films
    loadFilms();
    
    // Restore selections if possible
    if (!currentFilm.isEmpty()) {
        int filmIndex = filmCombo->findData(currentFilm);
        if (filmIndex >= 0) {
            filmCombo->setCurrentIndex(filmIndex);
        }
    }
    
    QMessageBox::information(this, "Refreshed", "Data has been refreshed!");
}
