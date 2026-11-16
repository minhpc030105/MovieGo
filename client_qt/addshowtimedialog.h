#ifndef ADDSHOWTIMEDIALOG_H
#define ADDSHOWTIMEDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QGroupBox>
#include <QDateTime>

class AddShowTimeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddShowTimeDialog(int sockfd, QWidget *parent = nullptr);

private slots:
    void onFilmSelected(int index);
    void onCinemaSelected(int index);
    void onRoomSelected(int index);
    void onAddShowTimeClicked();
    void onRefreshClicked();

private:
    void loadFilms();
    void loadCinemas();
    void loadRooms(const QString &cinemaId);
    void loadShowtimes(const QString &roomId);
    void clearShowtimes();
    
    int sockfd;
    
    QLabel *titleLabel;
    QComboBox *filmCombo;
    QComboBox *cinemaCombo;
    QComboBox *roomCombo;
    QDateTimeEdit *datetimeEdit;
    QTableWidget *showtimeTable;
    QPushButton *addBtn;
    QPushButton *refreshBtn;
    QPushButton *cancelBtn;
    
    QString selectedFilmId;
    QString selectedCinemaId;
    QString selectedRoomId;
};

#endif
