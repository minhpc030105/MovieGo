#ifndef BROWSEFILMDIALOG_H
#define BROWSEFILMDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>

class BrowseFilmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowseFilmDialog(int sockfd, QWidget *parent = nullptr);
    ~BrowseFilmDialog();

private slots:
    void onBrowseByCategory();
    void onBrowseByCinema();

private:
    void setupUI();
    void displayFilmList(const QString &title);
    
    int sockfd;
    
    QTabWidget *tabWidget;
    
    // Category tab
    QWidget *categoryTab;
    QListWidget *categoryListWidget;
    QListWidget *filmByCategoryListWidget;
    QPushButton *browseCategoryBtn;
    
    // Cinema tab
    QWidget *cinemaTab;
    QListWidget *cinemaListWidget;
    QListWidget *filmByCinemaListWidget;
    QPushButton *browseCinemaBtn;
};

#endif
