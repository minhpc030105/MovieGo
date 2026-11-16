#ifndef BOOKTICKETDIALOG_H
#define BOOKTICKETDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

class BookTicketDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BookTicketDialog(int sockfd, QWidget *parent = nullptr);
    ~BookTicketDialog();

private slots:
    void onSelectFilm();
    void onSelectCinema();
    void onSelectShowtime();
    void onBookTicket();
    void onBackToStart();

private:
    void setupUI();
    void loadFilms();
    void loadCinemas(const QString &filmId);
    void loadShowtimes(const QString &filmId, const QString &cinemaId);
    void loadSeats(const QString &showtimeId);
    
    int sockfd;
    
    QStackedWidget *stackedWidget;
    
    // Step 1: Select Film
    QWidget *filmPage;
    QListWidget *filmListWidget;
    QPushButton *selectFilmBtn;
    
    // Step 2: Select Cinema
    QWidget *cinemaPage;
    QListWidget *cinemaListWidget;
    QPushButton *selectCinemaBtn;
    QPushButton *backFromCinemaBtn;
    
    // Step 3: Select Showtime
    QWidget *showtimePage;
    QListWidget *showtimeListWidget;
    QPushButton *selectShowtimeBtn;
    QPushButton *backFromShowtimeBtn;
    
    // Step 4: Select Seat
    QWidget *seatPage;
    QListWidget *seatListWidget;
    QPushButton *selectSeatBtn;
    QPushButton *backFromSeatBtn;
    
    // Step 5: Confirmation
    QWidget *confirmPage;
    QLabel *confirmLabel;
    QPushButton *confirmBookBtn;
    QPushButton *backFromConfirmBtn;
    
    QString selectedFilmId;
    QString selectedCinemaId;
    QString selectedShowtimeId;
    QString selectedSeatId;
};

#endif
