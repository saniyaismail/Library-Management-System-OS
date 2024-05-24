#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "server.h"
// #include "library_operations.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9992
// #define MAX_PASSWORD 100

void display_menu() {
    printf("\nStudent Menu:\n");
    printf("1. Display all books\n");
    printf("2. Borrow book\n");
    printf("3. Return book\n");
    printf("4. Display borrowed books\n");
    printf("5. Exit\n");
    printf("Enter your choice: ");
}

void display_librarian_menu() {
    printf("\nLibrarian Menu:\n");
    printf("1. Add Book\n");
    printf("2. Remove Book\n");
    printf("3. Find Book\n");
    printf("4. Update Book\n");
    printf("5. Add User\n");
    printf("6. Remove User\n");
    printf("7. Find User\n");
    printf("8. Update User\n");
    printf("9. Display All Books\n");
    printf("10. Display All Users\n");
    printf("11. Exit\n");
    printf("Enter your choice: ");
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[1024];
    int userid;
    char password[MAX_PASSWORD];
    int login_result;
    char server_message[256];

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); // Server IP
    server_addr.sin_port = htons(SERVER_PORT);
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    while (1) {
        printf("Enter id: ");
        scanf("%d", &userid);
        printf("Enter password: ");
        scanf("%s", password);

        // Send username and password to the server
        write(client_socket, &userid, sizeof(userid));
        write(client_socket, password, sizeof(password));

        // Receive login result
        read(client_socket, server_message, sizeof(server_message) - 1);
        server_message[sizeof(server_message) - 1] = '\0';  // Null-terminate the received message
        printf("%s\n", server_message);

        if (strcmp(server_message, "Welcome student\n") != 0 && strcmp(server_message, "Welcome librarian\n") != 0) {
            close(client_socket);
            return 1;
        }

        int choice;

        if (strcmp(server_message, "Welcome student\n") == 0) {
            while (1) {
                display_menu();
                scanf("%d", &choice);

                //write(client_socket, &choice, sizeof(choice));

                switch (choice) {
                    case 1: {
                        choice = 9;
                        write(client_socket, &choice, sizeof(choice));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Displaying all books...\n");
                        // Implement book display based on server response
                        break;
                    }
                    case 2: {
                        choice = 11;
                        write(client_socket, &choice, sizeof(choice));
                        int bookID, quantity;
                        printf("Enter book ID to borrow: ");
                        scanf("%d", &bookID);
                        printf("Enter quantity to borrow: ");
                        scanf("%d", &quantity);

                        write(client_socket, &userid, sizeof(userid));
                        write(client_socket, &bookID, sizeof(bookID));
                        write(client_socket, &quantity, sizeof(quantity));

                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Borrow book result: %d\n", result);
                        break;
                    }
                    case 3: {
                        choice = 12;
                        write(client_socket, &choice, sizeof(choice));
                        int bookID, quantity;
                        printf("Enter book ID to return: ");
                        scanf("%d", &bookID);
                        printf("Enter quantity to return: ");
                        scanf("%d", &quantity);

                        write(client_socket, &userid, sizeof(userid));
                        write(client_socket, &bookID, sizeof(bookID));
                        write(client_socket, &quantity, sizeof(quantity));

                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Return book result: %d\n", result);
                        break;
                    }
                    case 4: {
                        choice = 13;
                        write(client_socket, &choice, sizeof(choice));
                        write(client_socket, &userid, sizeof(userid));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Displaying borrowed books...\n");
                        break;
                    }
                    case 5: {
                        close(client_socket);
                        return 0;
                    }
                    default: {
                        printf("Invalid choice, please try again\n");
                        break;
                    }
                }
            }
        } else if (strcmp(server_message, "Welcome librarian\n") == 0) {
            while (1) {
                display_librarian_menu();
                scanf("%d", &choice);
                write(client_socket, &choice, sizeof(choice));

                switch (choice) {
                    case 1: {
                        struct Book new_book;
                        printf("Enter book ID: ");
                        scanf("%d", &new_book.bookID);
                        printf("Enter book title: ");
                        scanf("%s", new_book.title);
                        printf("Enter book author: ");
                        scanf("%s", new_book.author);
                        printf("Enter book quantity: ");
                        scanf("%d", &new_book.quantity);

                        write(client_socket, &new_book, sizeof(new_book));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Add book result: %d\n", result);
                        break;
                    }
                    case 2: {
                        int bookID;
                        printf("Enter book ID to remove: ");
                        scanf("%d", &bookID);

                        write(client_socket, &bookID, sizeof(bookID));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Remove book result: %d\n", result);
                        break;
                    }
                    case 3: {
                        int bookID;
                        printf("Enter book ID to find: ");
                        scanf("%d", &bookID);

                        write(client_socket, &bookID, sizeof(bookID));
                        int result;
                        struct Book found_book;
                        read(client_socket, &result, sizeof(result));
                        if (result == LIB_SUCCESS) {
                            read(client_socket, &found_book, sizeof(found_book));
                            printf("Book ID: %d\nTitle: %s\nAuthor: %s\nQuantity: %d\n",
                                   found_book.bookID, found_book.title, found_book.author, found_book.quantity);
                        } else {
                            printf("Book not found.\n");
                        }
                        break;
                    }
                    case 4: {
                        int bookID;
                        struct Book updated_book;
                        printf("Enter book ID to update: ");
                        scanf("%d", &bookID);
                        printf("Enter new title: ");
                        scanf("%s", updated_book.title);
                        printf("Enter new author: ");
                        scanf("%s", updated_book.author);
                        printf("Enter new quantity: ");
                        scanf("%d", &updated_book.quantity);

                        write(client_socket, &bookID, sizeof(bookID));
                        write(client_socket, &updated_book, sizeof(updated_book));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Update book result: %d\n", result);
                        break;
                    }
                    case 5: {
                        struct User new_user;
                        printf("Enter user ID: ");
                        scanf("%d", &new_user.userID);
                        printf("Enter username: ");
                        scanf("%s", new_user.username);
                        printf("Enter password: ");
                        scanf("%s", new_user.password);
                        printf("Enter admin status (1 for admin, 0 for regular user): ");
                        scanf("%d", &new_user.isAdmin);

                        write(client_socket, &new_user, sizeof(new_user));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Add user result: %d\n", result);
                        break;
                    }
                    case 6: {
                        int userID;
                        printf("Enter user ID to remove: ");
                        scanf("%d", &userID);

                        write(client_socket, &userID, sizeof(userID));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Remove user result: %d\n", result);
                        break;
                    }
                    case 7: {
                        int userID;
                        printf("Enter user ID to find: ");
                        scanf("%d", &userID);

                        write(client_socket, &userID, sizeof(userID));
                        int result;
                        struct User found_user;
                        read(client_socket, &result, sizeof(result));
                        if (result == LIB_SUCCESS) {
                            read(client_socket, &found_user, sizeof(found_user));
                            printf("User ID: %d\nUsername: %s\nAdmin status: %d\n",
                                   found_user.userID, found_user.username, found_user.isAdmin);
                        } else {
                            printf("User not found.\n");
                        }
                        break;
                    }
                    case 8: {
                        int userID;
                        struct User updated_user;
                        printf("Enter user ID to update: ");
                        scanf("%d", &userID);
                        printf("Enter new username: ");
                        scanf("%s", updated_user.username);
                        printf("Enter new password: ");
                        scanf("%s", updated_user.password);
                        printf("Enter new admin status (1 for admin, 0 for regular user): ");
                        scanf("%d", &updated_user.isAdmin);

                        write(client_socket, &userID, sizeof(userID));
                        write(client_socket, &updated_user, sizeof(updated_user));
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Update user result: %d\n", result);
                        break;
                    }
                    case 9: {
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Displaying all books...\n");
                        // Implement book display based on server response
                        break;
                    }
                    case 10: {
                        int result;
                        read(client_socket, &result, sizeof(result));
                        printf("Displaying all users...\n");
                        // Implement user display based on server response
                        break;
                    }
                    case 11: {
                        close(client_socket);
                        return 0;
                    }
                    default: {
                        printf("Invalid choice, please try again\n");
                        break;
                    }
                }
            }
        }

        close(client_socket);
        return 0;
    }
}
