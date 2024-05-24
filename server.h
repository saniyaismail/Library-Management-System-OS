#ifndef LIBRARY_OPERATIONS_H
#define LIBRARY_OPERATIONS_H

// Error codes
#define LIB_SUCCESS 0
#define LIB_FILE_ERROR 1

// Inventory error codes
#define BOOK_ADD_FAILED 2
#define BOOK_NOT_FOUND 3
#define BOOK_ALREADY_EXISTS 4
#define BOOK_DELETE_FAILED 5
#define LIBRARY_OPEN 15
#define LIBRARY_CLOSED 16

// User error codes
#define USER_ADD_FAILED 6
#define USER_NOT_FOUND 7
#define USER_ALREADY_EXISTS 8
#define USER_DELETE_FAILED 9
#define USER_ALREADY_OPEN 19
#define USER_NOT_OPEN 20

// Cart error codes
#define CART_ADD_FAILED 10
#define CART_REMOVE_FAILED 11
#define CART_NOT_FOUND 12
#define CART_ALREADY_OPEN 17
#define CART_NOT_OPEN 18

// File locking error codes
#define FILE_LOCK_FAILED 13
#define FILE_UNLOCK_FAILED 14

#define BUFFER_SIZE 1024

// Socket programming error codes
// #define SOCKET_INIT_FAILED 15
// #define SOCKET_ACCEPT_FAILED 16
// #define SOCKET_READ_FAILED 17
// #define SOCKET_WRITE_FAILED 18

#define MAX_BOOKS 100
#define MAX_USERS 100
#define MAX_BOOK_TITLE 100
#define MAX_USERNAME 50
#define MAX_PASSWORD 50
#define MAX_CLIENTS 10
#define PORT 9992
// #define MAX_CLIENTS 5

struct Book {
    int bookID;
    char title[MAX_BOOK_TITLE];
    char author[MAX_USERNAME];
    int quantity;
};

struct User {
    int userID;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    int isAdmin;
};

struct CartEntry {
    int bookID;
    int quantity;
};

struct Cart {
    int userID;
    struct CartEntry items[MAX_BOOKS];
    int itemCount;
};

struct BookInventory {
    int bookCount;
    struct Book books[MAX_BOOKS];
};

struct UserDatabase {
    int userCount;
    struct User users[MAX_USERS];
};

struct LibraryInfo {
    FILE * book_data_fp;
    FILE * user_data_fp;
    FILE * cart_data_fp;
    int book_size;
    int user_size;
    int cart_size;
    int book_status;
    int user_status;
    int cart_status;
    int book_count;
    int user_count;
    int cart_count;
    // struct BookInventory inventory;
    // struct UserDatabase users;
    // struct Cart carts[MAX_CLIENTS];
    //int isServerRunning;
};

extern struct LibraryInfo library_handle;

int library_create();
int library_open();
//int library_load_data();
int library_close();
int library_add_book(int bookID, char* title, char* author, int quantity);
int library_remove_book(int bookID);
int library_find_book(int bookID, struct Book *);
int library_update_book(int bookID, char* newTitle, char* newAuthor, int newQuantity);
int library_add_user(int userID, char* username, char* password, int isAdmin);
int library_remove_user(int userID);
int library_find_user(int userID, struct User *);
int library_update_user(int userID, char* newUsername, char* newPassword, int newIsAdmin);
int library_login(int userid, char* password);
//int library_login2(char* username, char* password);
//int library_logout(int userID);
int library_display_books();
int library_display_users();
int library_display_cart(int userID);
int library_add_to_cart(int userID, int bookID, int quantity);
int library_remove_from_cart(int userID, int bookID, int quantity);
//int library_checkout_cart(int userID);

#endif
