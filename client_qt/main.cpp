#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

extern "C" {
    #include "../lib/socket/socket.h"
    #include "../lib/messages/message.h"
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 1024

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    if(argc != 3){
        QMessageBox::critical(nullptr, "Error", 
            "Usage: ./client_qt <ip address> <port number>");
        return 1;
    }

    // Initialize socket connection
    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        QMessageBox::critical(nullptr, "Error", 
            "Failed to create socket!");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        QMessageBox::critical(nullptr, "Error", 
            "Failed to connect to server!");
        close(sockfd);
        return 1;
    }

    // Receive welcome message from server
    char conn_mess[BUFSIZE];
    memset(conn_mess, 0, BUFSIZE);

    if (recv(sockfd, conn_mess, sizeof(conn_mess), 0) > 0) {
        QMessageBox::information(nullptr, "Connection", 
            QString::fromUtf8(conn_mess));
    }

    // Create and show main window
    MainWindow window(sockfd);
    window.show();

    int result = app.exec();
    
    // Close socket when application exits
    close(sockfd);
    
    return result;
}
