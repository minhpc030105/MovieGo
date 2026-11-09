#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../lib/socket/socket.h"
#include "../../lib/messages/message.h"
#include "menu.h"
#include "function.h"
#include "../../lib/resultCodes.h"

#define MSG_END "END"

#define MAX_BUFFER 1024

void clearKeyboardBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {

    }
}

void handleLogin(int sockfd, char *username, char *password, char *message, int *login_status){
    viewLogin(username, password);
    makeLoginMessage(username, password, message);
    sendMessage(sockfd, message);
    *login_status = recvResult(sockfd);
    if(*login_status == LOGIN_FAIL){
        printf("Your username or password is incorrect!!\n");
    }else if(*login_status == LOGIN_ALREADY){
        printf("Your account is being used in another address!!\n");
    }else{
        printf("You have logged in successfully!!\n");
    }
}

void handleRegister(int sockfd, char *message){
    char newName[255], newUsername[255], newPassword[255];

    viewRegister(newName, newUsername, newPassword);
    makeRegisterMessage(newName, newUsername, newPassword, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if (result == REGISTER_SUCCESS) {
        printf("You have registered successfully!!\n");
    } else {
        printf("Username has been existed!!\n");
    }
}

void handleRequestUser(int sockfd, char *username, char *message) {
    int user_choice;
    do {
        viewUser();
        printf("Choice: ");
        scanf("%d", &user_choice);
        clearKeyboardBuffer();
        switch(user_choice) {
            case 1:{
                handleBrowseFilm(sockfd);
                break;
            }
            case 2:{
                // char *title;
                handleSearchByTitle(sockfd);
                break;
            }
            case 3:{
                handleBookTicket(sockfd);
                break;
            }
            case 4:{
                handleViewTickets(sockfd, username);
                break;
            }
            case 5:{
                handleLogout(sockfd, message);
                user_choice = 0;
                break;
            }
            case 6:{
                printf("%s\n", username);
                handleChangePassword(sockfd, username, message);
                break;
            }
        }
    } while (user_choice != 0);
}

void handleLogout(int sockfd, char *message){
    makeLogoutMessage(message);
    sendMessage(sockfd, message);
    recvResult(sockfd);
    printf("You have been successfully logged out!!\n");
}

void handleChangePassword(int sockfd, char *username, char *message){
    char oldPassword[255], newPassword[255];
    viewChangePassword(oldPassword, newPassword);
    makeChangePasswordMessage(username, oldPassword, newPassword, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    // printf("%d\n", result);
    if (result == CHANGE_PASSWORD_SUCCESS) {
        printf("Changed password successfully\n");
    } else {
        printf("Changed password fail!!\n");
    }
}

/*----------BOOK TICKET--------*/
void handleBookTicket(int sockfd) {
    int choose;
    do{
        bookTicket();
        printf("Choice: ");
        scanf("%d", &choose);
        clearKeyboardBuffer();
        
        switch(choose){
            case 1:{
                char film_id[255];
                char cinema_id[255];
                char showtime_id[255];
                char seat_id[255];
                char message[2048];

                // Bước 1: Hiển thị danh sách phim
                printf("\n=== SELECT FILM ===\n");
                memset(message, 0, sizeof(message));
                makeShowFilmMessage(message);
                sendMessage(sockfd, message);
                
                int result = recvResult(sockfd);
                if (result != FIND_FILM_SUCCESS && result != NO_FILMS) {
                    printf("Error loading films!\n");
                    break;
                }
                if (result == NO_FILMS) {
                    printf("No films available!\n");
                    break;
                }
                
                while (1) {
                    recvMessage(sockfd, message);
                    if (strcmp(message, "END") == 0) break;
                    printf("%s\n", message);
                }
                
                printf("Enter film ID: ");
                fgets(film_id, sizeof(film_id), stdin);
                film_id[strcspn(film_id, "\n")] = 0;
                
                // Bước 2: Hiển thị rạp chiếu phim đó
                printf("\n=== SELECT CINEMA ===\n");
                memset(message, 0, sizeof(message));
                makeShowCinemaByFilmMessage(film_id, message);
                sendMessage(sockfd, message);
                
                result = recvResult(sockfd);
                if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
                    printf("Error loading cinemas!\n");
                    break;
                }
                if (result == NO_CINEMAS) {
                    printf("No cinemas available for this film!\n");
                    break;
                }
                
                while (1) {
                    recvMessage(sockfd, message);
                    if (strcmp(message, "END") == 0) break;
                    printf("%s\n", message);
                }
                
                printf("Enter cinema ID: ");
                fgets(cinema_id, sizeof(cinema_id), stdin);
                cinema_id[strcspn(cinema_id, "\n")] = 0;

                // Bước 3: Hiển thị suất chiếu
                printf("\n=== SELECT SHOWTIME ===\n");
                memset(message, 0, sizeof(message));
                makeShowTimeByFilmCinemaMessage(film_id, cinema_id, message);
                sendMessage(sockfd, message);
                
                result = recvResult(sockfd);
                if (result != BROWSE_TIME_SUCCESS && result != NO_SHOWTIMES) {
                    printf("Error loading showtimes!\n");
                    break;
                }
                if (result == NO_SHOWTIMES) {
                    printf("No showtimes available!\n");
                    break;
                }
                
                while (1) {
                    recvMessage(sockfd, message);
                    if (strcmp(message, "END") == 0) break;
                    printf("%s\n", message);
                }
                
                printf("Enter showtime ID: ");
                fgets(showtime_id, sizeof(showtime_id), stdin);
                showtime_id[strcspn(showtime_id, "\n")] = 0;

                // Bước 4: Hiển thị ghế trống
                printf("\n=== SELECT SEAT ===\n");
                memset(message, 0, sizeof(message));
                makeShowSeatMessage(showtime_id, message);
                sendMessage(sockfd, message);
                
                result = recvResult(sockfd);
                if (result == INVALID_REQUEST || result == INVALID_SHOWTIME) {
                    printf("Invalid showtime!\n");
                    break;
                }
                if (result == NO_SEATS_AVAILABLE) {
                    printf("No seats available!\n");
                    break;
                }
                if (result != VIEW_CHAIR_SUCCESS) {
                    printf("Error loading seats!\n");
                    break;
                }
                
                int seat_available = 0;

                while (1) {
                    recvMessage(sockfd, message);
                    if (strcmp(message, "END") == 0) break;
                    if (strncmp(message, "SEAT|", 5) == 0) {
                        seat_available = 1;
                        char *seat_id = strtok(message + 5, "|");
                        char *seat_name = strtok(NULL, "|");
                        if (seat_id && seat_name) {
                            printf("ID: %-4s  Seat: %s\n", seat_id, seat_name);
                        }
                    }
                }
                if (!seat_available) {
                    printf("No seats available!\n");
                    break;
                }
                printf("Enter seat ID: ");
                fgets(seat_id, sizeof(seat_id), stdin);
                seat_id[strcspn(seat_id, "\n")] = 0;

                // Bước 5: Đặt vé
                memset(message, 0, sizeof(message));
                makeBookTicketMessage(showtime_id, seat_id, message);
                sendMessage(sockfd, message);

                result = recvResult(sockfd);
                if (result == BOOK_TICKET_SUCCESS) {
                    printf("\nBOOK TICKET SUCCESS!\n");
                }
                else if (result == BOOK_TICKET_FAIL) {
                    printf("\nBOOK TICKET FAILED! (Seat already booked or system error)\n");
                }
                break;
            }
            case 2:{
                choose = 0;
                break;
            }
            default:
                printf("Invalid choice!\n");
        }
    } while(choose != 0);
}

/*----------END BOOK TICKET--------*/


/*----------FIND FILM--------*/
void handleSearchByTitle(int sockfd){
    char title[255] = {0};
    char message[1024] = {0};
    
    printf("\nEnter film title to search: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;
    
    makeSearchFilmByTitleMessage(title, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if(result == FIND_FILM_SUCCESS){
        printf("\n=== SEARCH RESULTS ===\n");
        while(1){
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if(strcmp(message, "END") == 0) break;
            printf("%s\n", message);
        }
    }else if(result == FIND_FILM_FAIL){
        printf("\nNo films found!\n");
    }
}
/*----------END FIND FILM--------*/



/*----------BROWSE FILM--------*/

// Browse theo thể loại
void handleBrowseFollowCategory(int sockfd){
    char category_id[255];
    char message[2048];

    printf("\n=== BROWSE FILM BY CATEGORY ===\n");
    
    // Show category list
    memset(message, 0, sizeof(message));
    makeShowCategoryMessage(message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result != SHOW_CATEGORY_SUCCESS && result != NO_CATEGORIES) {
        printf("Error loading categories!\n");
        return;
    }
    if (result == NO_CATEGORIES) {
        printf("No categories available!\n");
        return;
    }
    
    // Read until "END"
    while(1){
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if(strcmp(message, "END") == 0) break;
        printf("%s\n", message);
    }

    printf("Enter category ID: ");
    fgets(category_id, sizeof(category_id), stdin);
    category_id[strcspn(category_id, "\n")] = 0;

    memset(message, 0, sizeof(message));
    makeBrowseFollowCategoryMessage(category_id, message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if(result == BROWSE_CATEGORY_SUCCESS){
        printf("\n=== FILM LIST ===\n");
        while(1){
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if(strcmp(message, "END") == 0) break;
            printf("%s\n", message);
        }
    } else {
        printf("\nNo films found in this category!\n");
    }
}

// Browse theo rạp
void handleBrowseFollowCinema(int sockfd){
    char cinema_id[255];
    char message[2048];

    printf("\n=== BROWSE FILM BY CINEMA ===\n");
    
    memset(message, 0, sizeof(message));
    makeShowCinemaMessage(message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
        printf("Error loading cinemas!\n");
        return;
    }
    if (result == NO_CINEMAS) {
        printf("No cinemas available!\n");
        return;
    }
    
    // Read until "END"
    while(1){
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if(strcmp(message, "END") == 0) break;
        printf("%s\n", message);
    }

    printf("Enter cinema ID: ");
    fgets(cinema_id, sizeof(cinema_id), stdin);
    cinema_id[strcspn(cinema_id, "\n")] = 0;

    memset(message, 0, sizeof(message));
    makeBrowseFollowCinemaMessage(cinema_id, message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if(result == BROWSE_THEATER_SUCCESS){
        printf("\n=== FILM LIST ===\n");
        while(1){
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if(strcmp(message, "END") == 0) break;
            printf("%s\n", message);
        }
    } else {
        printf("\nNo films found in this cinema!\n");
    }
}

// Browse theo giờ chiếu
void handleBrowseFollowShowTime(int sockfd){
    char time_slot[255];
    char message[2048];

    printf("\n=== SELECT FILM BY SHOWTIME ===\n");
    
    // Hiển thị danh sách giờ chiếu
    memset(message, 0, sizeof(message));
    makeShowPremieredTimeMessage(message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result != BROWSE_TIME_SUCCESS && result != NO_SHOWTIMES) {
        printf("Error loading showtimes!\n");
        return;
    }
    if (result == NO_SHOWTIMES) {
        printf("No showtimes available!\n");
        return;
    }
    
    // Read until "END"
    while(1){
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if(strcmp(message, "END") == 0) break;
        printf("%s\n", message);
    }

    printf("Enter showtime ID: ");
    fgets(time_slot, sizeof(time_slot), stdin);
    time_slot[strcspn(time_slot, "\n")] = 0;

    memset(message, 0, sizeof(message));
    makeBrowseFollowPremieredTimeMessage(time_slot, message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if(result == BROWSE_TIME_SUCCESS){
        printf("\n=== FILM LIST ===\n");
        while(1){
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if(strcmp(message, "END") == 0) break;
            printf("%s\n", message);
        }
    } else {
        printf("\nNo films found in this time slot!\n");
    }
}

void handleBrowseFilm(int sockfd){
    int choice;
    do{
        browseFilm();
        printf("Choice: ");
        scanf("%d", &choice);
        clearKeyboardBuffer();
        
        switch(choice){
            case 1: handleBrowseFollowCategory(sockfd); break;
            case 2: handleBrowseFollowCinema(sockfd); break;
            case 3: choice = 0; break;
            default: printf("Invalid choice!\n");
        }
    } while(choice != 0);
}

/*----------END BROWSE FILM--------*/

/*----------TICKET----------*/
void handleViewTickets(int sockfd, char *username) {
    char message[2048];

    printf("\n=== MY TICKETS ===\n");

    // 1. Get all tickets
    memset(message, 0, sizeof(message));
    makeViewTicketsMessage(username, message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if (result == NO_TICKETS) {
        while (1) {
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if (strcmp(message, "END") == 0) break;
        }
        printf("\nNo tickets found.\n");
        return;
    }
    if (result == FAILED_RETRIEVE) {
        printf("\nFailed to retrieve tickets.\n");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        printf("\nError loading tickets!\n");
        return;
    }

    while (1) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) 
            break;
        printf("%s\n", message);
    }

    int choice = 0;
    do {
        viewTicket();
        printf("Choice: ");
        scanf("%d", &choice);
        clearKeyboardBuffer();
        
        switch (choice)
        {
        case 1: {
            handleViewTicketDetail(sockfd);
            break;
        }
        case 2: {
            choice = 0;
            break;
        }
        default:
            printf("Invalid choice!\n");
            break;
        }
    } while (choice != 0);
    
}

// View detail information of a ticket
void handleViewTicketDetail(int sockfd){
    char ticket_id[50];
    char message[2048];
    
    printf("Enter ticket ID: ");
    fgets(ticket_id, sizeof(ticket_id), stdin);
    ticket_id[strcspn(ticket_id, "\n")] = 0;
    
    memset(message, 0, sizeof(message));
    makeViewTicketDetailMessage(ticket_id, message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if (result == TICKET_NOT_FOUND) {
        printf("\nTicket not found!\n");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        printf("\nError loading ticket detail!\n");
        return;
    }
    
    // printf("\n--- Ticket Detail ---\n");
    while(1){
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if(strcmp(message, "END") == 0) 
            break;
        printf("%s\n", message);
    }

    printf("\n=== End of Ticket Detail ===\n\n");
}

/*----------END TICKET----------*/

/*----------MANAGER--------*/
void handleRequestManager(int sockfd, char *username, char *message) {
    int manager_choice = 0;
    do {
        viewManager();
        printf("Choice: ");
        scanf("%d", &manager_choice);
        clearKeyboardBuffer();
        switch (manager_choice) {
            case 1: {
                handleAddFilm(sockfd);
                break;
            }
            case 2: {
                handleAddShowTime(sockfd);
                break;
            }    
            case 3: {
                handleBrowseFilm(sockfd);
                break;
            }
            case 4: {
                handleLogout(sockfd, message);
                manager_choice = 0;
                break;
            }
            default: {
                printf("Invalid choice!\n");
                break;
            }
        }
    } while(manager_choice != 0);
}

/*-----ADD FILM-----*/

void handleAddFilm(int sockfd) {
    char title[255];
    char category_id[20];
    char show_time[20];
    char message[2048];

    printf("\n=== ADD NEW FILM ===\n");
    // 1. Show all categories
    memset(message, 0, sizeof(message));
    makeShowCategoryMessage(message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if (result != SHOW_CATEGORY_SUCCESS && result != NO_CATEGORIES) {
        printf("Error loading categories!\n");
        return;
    }
    if (result == NO_CATEGORIES) {
        printf("No categories available!\n");
        return;
    }

    printf("\nAvailable Categories: \n");
    while (1) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) 
            break;
        printf("%s\n", message);
    }
    // 2. Enter film info
    printf("Enter film title: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = 0;    

    if (strlen(title) == 0) {
        printf("Title cannot be empty!\n");
        return;
    }
    printf("Enter category ID: ");
    fgets(category_id, sizeof(category_id), stdin);
    category_id[strcspn(category_id, "\n")] = 0;    

    printf("Enter show time (in minutes): ");
    fgets(show_time, sizeof(show_time), stdin);
    show_time[strcspn(show_time, "\n")] = 0; 
    
    int duration = atoi(show_time);
    if (duration <= 0) {
        printf("Invalid show time!\n");
        return;
    }

    // 3. Send message to server
    memset(message, 0, sizeof(message));
    makeAddFilmMessage(title, category_id, show_time, message);
    sendMessage(sockfd, message);

    // 4. Receive result from server
    result = recvResult(sockfd);
    if (result == ADD_FILM_SUCCESS) {
        printf("Add new film successfully!!\n");
    } else if (result == FILM_EXISTS) {
        printf("Film already exists in the system!!\n");
    } else if (result == ADD_FILM_FAIL) {
        printf("Add new film failed!!\n");
    }

}
/*-----END ADD FILM-----*/

/*-----ADD SHOW TIME-----*/
void handleAddShowTime(int sockfd) {
    char film_id[255];
    char cinema_id[255];
    char room_id[255];
    char start_datetime[255];
    char message[2048];

    printf("\n=== ADD SHOW TIME ===\n");

    // 1. Show available films 
    memset(message, 0, sizeof(message));
    makeShowFilmMessage(message);
    sendMessage(sockfd, message);

    int result = recvResult(sockfd);
    if (result != FIND_FILM_SUCCESS && result != NO_FILMS) {
        printf("Error loading films!\n");
        return;
    }
    if (result == NO_FILMS) {
        printf("No films available!\n");
        return;
    }

    printf("\nAvailable Films: \n");
    while (1) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0)
            break;
        printf("%s\n", message);
    }
    printf("Enter film ID: ");
    fgets(film_id, sizeof(film_id), stdin);
    film_id[strcspn(film_id, "\n")] = 0;

    // 2. Show available cinemas
    memset(message, 0, sizeof(message));
    makeShowCinemaMessage(message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if (result != BROWSE_THEATER_SUCCESS && result != NO_CINEMAS) {
        printf("Error loading cinemas!\n");
        return;
    }
    if (result == NO_CINEMAS) {
        printf("No cinemas available!\n");
        return;
    }

    printf("\nAvailable Cinemas: \n");
    while (1) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) {
            break;
        }
        printf("%s\n", message);
    }
    printf("Enter cinema ID: ");    
    fgets(cinema_id, sizeof(cinema_id), stdin);
    cinema_id[strcspn(cinema_id, "\n")] = 0;    

    // 3. Show rooms in selected cinema
    memset(message, 0, sizeof(message));
    makeShowRoomsByCinemaMessage(cinema_id, message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if (result == INVALID_CINEMA) {
        printf("Invalid cinema ID!\n");
        return;
    }
    if (result == NO_ROOMS || result == FAILED_ROOMS) {
        printf("No rooms available in this cinema!\n");
        return;
    }
    if (result != VIEW_CHAIR_SUCCESS) {
        printf("Error loading rooms!\n");
        return;
    }

    printf("\nAvailable Rooms: \n");
    while (1) {
        memset(message, 0, sizeof(message));
        recvMessage(sockfd, message);
        if (strcmp(message, "END") == 0) {
            break;  
        }
        printf("%s\n", message);
    }
    printf("\nEnter room ID: ");
    fgets(room_id, sizeof(room_id), stdin);
    room_id[strcspn(room_id, "\n")] = 0;

    // 4. Show current showtimes in the selected room
    memset(message, 0, sizeof(message));
    makeShowShowtimesByRoomMessage(room_id, message);
    sendMessage(sockfd, message);

    result = recvResult(sockfd);
    if (result == SHOW_TIME_FAIL) {
        printf("\nNo showtimes in this room yet.\n");
    } else if (result == SHOW_SHOWTIME_SUCCESS) {
        printf("\nCurrent Showtimes in Room:\n");
        while (1) {
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if (strcmp(message, "END") == 0) {
                break;  
            }
            printf("%s\n", message);
        }
    }
    printf("Enter start time (YYYY-MM-DD HH:MM:SS): ");
    printf("Format example: 2025-12-11 14:30:00\n");
    fgets(start_datetime, sizeof(start_datetime), stdin);
    start_datetime[strcspn(start_datetime, "\n")] = 0;  

    // Validate datetime format 
    if (strlen(start_datetime) != 19) {
        printf("Invalid time format!\n");
        return;
    }
    // 6. Send to the server
    memset(message, 0, sizeof(message));
    makeAddShowtimeMessage(film_id, cinema_id, room_id, start_datetime, message);
    sendMessage(sockfd, message);

    // 7. Receive return result from the server
    result = recvResult(sockfd);
    if (result == ADD_SHOWTIME_SUCCESS) {
        printf("\nShowtime added successfully!\n");
    } else if (result == INVALID_FILM_ID) {
        printf("\nInvalid film ID!\n");
    } else if (result == DB_QUERY_ERROR) {
        printf("\nDatabase error!\n");
    } else if (result == INVALID_CINEMA_ID) {
        printf("\nInvalid cinema ID!\n");
    } else if (result == INVALID_ROOM_ID) {
        printf("\nInvalid room ID or room is not active!\n");
    } else if (result == ROOM_NOT_BELONG_CINEMA) {
        printf("\nRoom does not belong to the selected cinema!\n");
    } else if (result == INVALID_DATETIME_FORMAT) {
        printf("\nInvalid datetime format!\n");
    } else if (result == SCHEDULING_CONFLICT) {
        printf("\nScheduling conflict! There is another showtime in this room at that time.\n");
    } else {
        printf("\nFailed to add showtime!\n");
    }
}
/*-----END ADD SHOW TIME-----*/

/*----------END MANAGER--------*/

/*----------ADMIN MANAGEMENT--------*/

void handleShowAllUsers(int sockfd){
    char message[2048];
    
    printf("\n=== ALL USERS ===\n");
    
    memset(message, 0, sizeof(message));
    makeShowAllUsersMessage(message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if(result == SHOW_ALL_USERS_SUCCESS){
        printf("%-5s %-20s %-20s %-10s\n", "ID", "Name", "Username", "Role");
        printf("---------------------------------------------------------------\n");
        while(1){
            memset(message, 0, sizeof(message));
            recvMessage(sockfd, message);
            if(strcmp(message, "END") == 0) break;
            printf("%s\n", message);
        }
    } else {
        printf("\nFailed to retrieve user list!\n");
    }
}

void handleDeleteUser(int sockfd){
    char user_id[255];
    char message[1024];
    
    printf("\n=== DELETE USER ===\n");
    printf("Enter user ID to delete: ");
    fgets(user_id, sizeof(user_id), stdin);
    user_id[strcspn(user_id, "\n")] = 0;
    
    memset(message, 0, sizeof(message));
    makeDeleteUserMessage(user_id, message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if(result == DELETE_USER_SUCCESS){
        printf("\nUser deleted successfully!\n");
    } else {
        printf("\nFailed to delete user!\n");
    }
}

void handleChangeUserRole(int sockfd){
    char user_id[255];
    char new_role[255];
    char message[1024];
    
    printf("\n=== CHANGE USER ROLE ===\n");
    printf("Enter user ID: ");
    fgets(user_id, sizeof(user_id), stdin);
    user_id[strcspn(user_id, "\n")] = 0;
    
    printf("Enter new role (0=Admin, 1=Manager, 2=User): ");
    fgets(new_role, sizeof(new_role), stdin);
    new_role[strcspn(new_role, "\n")] = 0;
    
    memset(message, 0, sizeof(message));
    makeChangeUserRoleMessage(user_id, new_role, message);
    sendMessage(sockfd, message);
    
    int result = recvResult(sockfd);
    if(result == UPDATE_USER_ROLE_SUCCESS){
        printf("\nUser role updated successfully!\n");
    } else {
        printf("\nFailed to update user role!\n");
    }
}

void handleRequestAdmin(int sockfd, char *username, char *message){
    int admin_choice;
    do {
        viewAdmin();
        printf("Choice: ");
        scanf("%d", &admin_choice);
        clearKeyboardBuffer();
        
        switch(admin_choice){
            case 1: {
                handleShowAllUsers(sockfd);
                break;
            }
            case 2: {
                handleDeleteUser(sockfd);
                break;
            }
            case 3: {
                handleChangeUserRole(sockfd);
                break;
            }
            case 4: {
                handleLogout(sockfd, message);
                admin_choice = 0;
                break;
            }
            default: {
                printf("Invalid choice!\n");
                break;
            }
        }
    } while (admin_choice != 0);     
}



