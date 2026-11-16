#ifndef SEARCHFILMDIALOG_H
#define SEARCHFILMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>

class SearchFilmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchFilmDialog(int sockfd, QWidget *parent = nullptr);
    ~SearchFilmDialog();

private slots:
    void onSearchClicked();

private:
    void setupUI();
    
    int sockfd;
    
    QLineEdit *titleLineEdit;
    QPushButton *searchBtn;
    QListWidget *resultListWidget;
};

#endif // SEARCHFILMDIALOG_H
