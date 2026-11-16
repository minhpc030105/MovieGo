#ifndef VIEWTICKETDETAILDIALOG_H
#define VIEWTICKETDETAILDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

class ViewTicketDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewTicketDetailDialog(int sockfd, const QString &ticketId, QWidget *parent = nullptr);
    ~ViewTicketDetailDialog();

private slots:
    void onCloseClicked();

private:
    void loadTicketDetail();
    
    int sockfd;
    QString ticketId;
    
    QLabel *customerLabel;
    QLabel *filmLabel;
    QLabel *cinemaLabel;
    QLabel *roomLabel;
    QLabel *showtimeLabel;
    QLabel *bookedLabel;
    QPushButton *closeBtn;
};

#endif // VIEWTICKETDETAILDIALOG_H
