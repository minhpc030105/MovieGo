/*====================RESULT CODE====================*/

/*----------ACCOUNT----------*/
#define LOGIN_SUCCESS_USER        1010
#define LOGIN_SUCCESS_MANAGER     1011
#define LOGIN_SUCCESS_ADMIN       1012
#define LOGIN_FAIL                2011
#define LOGIN_ALREADY             2012
#define LOGOUT_SUCCESS            1030
#define REGISTER_SUCCESS          1020
#define REGISTER_FAIL             2021
#define CHANGE_PASSWORD_SUCCESS   1110
#define CHANGE_PASSWORD_FAIL      2110
/*----------END ACCOUNT----------*/


/*----------ADD FILM--------*/
#define ADD_FILM_SUCCESS          1040
#define ADD_FILM_FAIL             2041
#define POST_FILM_SUCCESS         1050
#define POST_FILM_FAIL            2051
#define EDIT_FILM_SUCCESS         1060
#define EDIT_FILM_FAIL            2061
#define NO_EDIT_FILM              2062
#define FILM_EXISTS               2063
/*----------END ADD FILM--------*/


/*----------FIND FILM--------*/
#define FIND_FILM_SUCCESS         1073
#define FIND_FILM_FAIL            2072
/*----------END FIND FILM--------*/


/*----------BROWSE FILM--------*/
#define BROWSE_CATEGORY_SUCCESS   1070
#define BROWSE_THEATER_SUCCESS    1071
#define BROWSE_TIME_SUCCESS       1072
#define BROWSE_FAIL               2071
/*----------END BROWSE FILM--------*/


/*----------SHOW DATA CODES--------*/
#define SHOW_CATEGORY_SUCCESS     1065
#define SHOW_CATEGORY_FAIL        2060

#define VIEW_CHAIR_SUCCESS        1080
#define VIEW_CHAIR_FAIL           2081

#define SHOW_SHOWTIME_SUCCESS     1091
#define SHOW_TIME_FAIL            2092

#define CANT_LOAD_CATEGORIES      3001
#define NO_CATEGORIES             3002
#define CANT_LOAD_CINEMAS         3003
#define NO_CINEMAS                3004
#define CANT_LOAD_SHOWTIMES       3005
#define NO_SHOWTIMES              3006
#define CANT_LOAD_FILMS           3007
#define NO_FILMS                  3008
#define CANT_LOAD_SEATS           3009
#define NO_SEATS                  3010
#define CANT_LOAD_ROOMS           3011
#define NO_ROOMS                  3012
#define NO_TICKETS                3013
#define FAILED_RETRIEVE           3014
#define TICKET_NOT_FOUND          3015
#define INVALID_CINEMA            3016
#define FAILED_ROOMS              3017
#define INVALID_REQUEST           3018
#define INVALID_SHOWTIME          3019
#define NO_SEATS_AVAILABLE        3020
/*----------END SHOW DATA CODES--------*/


/*----------BOOK TICKET--------*/
#define CHOOSE_CHAIR_SUCCESS      1090
#define CHOOSE_CHAIR_FAIL         2091
#define BOOK_TICKET_SUCCESS       1200
#define BOOK_TICKET_FAIL          2200
/*----------END BOOK TICKET--------*/


/*----------ADD SHOWTIME CODES--------*/
#define INVALID_FILM_ID           3021
#define DB_QUERY_ERROR            3022
#define INVALID_CINEMA_ID         3023
#define INVALID_ROOM_ID           3024
#define ROOM_NOT_BELONG_CINEMA    3025
#define INVALID_DATETIME_FORMAT   3026
#define SCHEDULING_CONFLICT       3027
#define ADD_SHOWTIME_SUCCESS      1092
#define ADD_SHOWTIME_FAIL         2090
/*----------END ADD SHOWTIME CODES--------*/


/*----------ADMIN MANAGEMENT----------*/
#define SHOW_ALL_USERS_SUCCESS    1300
#define SHOW_ALL_USERS_FAIL       2300
#define DELETE_USER_SUCCESS       1301
#define DELETE_USER_FAIL          2301
#define UPDATE_USER_ROLE_SUCCESS  1302
#define UPDATE_USER_ROLE_FAIL     2302
/*----------END ADMIN MANAGEMENT----------*/

/*====================END RESULT CODE====================*/
