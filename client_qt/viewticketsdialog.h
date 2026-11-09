#ifndef VIEWTICKETSDIALOG_H
#define VIEWTICKETSDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QString>

class ViewTicketsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ViewTicketsDialog(int sockfd, const QString &username, QWidget *parent = nullptr);
    ~ViewTicketsDialog();

private slots:
    void onRefreshClicked();
    void onCloseClicked();
    void onTicketDoubleClicked(int row, int column);

private:
    void loadTickets();
    
    int sockfd;
    QString username;
    
    QTableWidget *ticketsTable;
    QPushButton *refreshBtn;
    QPushButton *closeBtn;
    QLabel *statusLabel;
};

#endif // VIEWTICKETSDIALOG_H
