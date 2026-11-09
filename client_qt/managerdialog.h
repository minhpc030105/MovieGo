#ifndef MANAGERDIALOG_H
#define MANAGERDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QMessageBox>
#include <QFormLayout>

class ManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ManagerDialog(int sockfd, const QString &username, QWidget *parent = nullptr);

private slots:
    void onAddFilmClicked();
    void onAddShowTimeClicked();
    void onBrowseFilmsClicked();
    void onLogoutClicked();

private:
    void showAddFilmDialog();
    
    int sockfd;
    QString username;
    
    QLabel *titleLabel;
    QLabel *userLabel;
    QPushButton *addFilmBtn;
    QPushButton *addShowTimeBtn;
    QPushButton *browseFilmsBtn;
    QPushButton *logoutBtn;
};

#endif
