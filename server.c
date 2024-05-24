#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include "server.h"

struct LibraryInfo library_handle;

int library_create() {
    int book_fd = open("Books.dat", O_RDONLY | O_CREAT, 0644);
    int user_fd = open("Users.dat", O_RDONLY | O_CREAT, 0644);
    int cart_fd = open("Carts.dat", O_RDONLY | O_CREAT, 0644);
    
    if (book_fd == -1 || user_fd == -1 || cart_fd == -1) {
        perror("Error in creating library files.\n");
        return LIB_FILE_ERROR;
    }

    // Initialize counts
    library_handle.book_count = 0;
    library_handle.user_count = 0;
    library_handle.cart_count = 0;

    close(book_fd);
    close(user_fd);
    close(cart_fd);

    return LIB_SUCCESS;
}


int library_open() {
    FILE *book_fd = fopen("Books.dat", "rb+");
    FILE *user_fd = fopen("Users.dat", "rb+");
    FILE *cart_fd = fopen("Carts.dat", "rb+");
    // int book_fd = open("Books.dat", O_RDWR, 0644);
    // int user_fd = open("Users.dat", O_RDWR, 0644);
    // int cart_fd = open("Carts.dat", O_RDWR, 0644);
    
    if (fileno(book_fd) == -1 || fileno(user_fd) == -1 || fileno(cart_fd) == -1) {
        perror("Error in opening library files.\n");
        return LIB_FILE_ERROR;
    }

    library_handle.book_data_fp = book_fd;
    library_handle.user_data_fp = user_fd;
    library_handle.cart_data_fp = cart_fd;

    library_handle.book_size = sizeof(struct Book);
    library_handle.user_size = sizeof(struct User);
    library_handle.cart_size = sizeof(struct Cart);

    // Update statuses
    library_handle.book_status = LIBRARY_OPEN;
    library_handle.user_status = LIBRARY_OPEN;
    library_handle.cart_status = LIBRARY_OPEN;

    library_handle.book_count = 0;
    library_handle.user_count = 0;
    library_handle.cart_count = 0;

    return LIB_SUCCESS;
}

int library_add_book(int bookID, char* title, char* author, int quantity) {
    if (library_handle.book_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct Book newBook;
        int offset = lseek(fileno(library_handle.book_data_fp), 0, SEEK_END);
        
        // Set a write lock on the whole file to prevent concurrent modifications
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = lseek(fileno(library_handle.book_data_fp), 0, SEEK_END);
        fcntl(fileno(library_handle.book_data_fp), F_SETLKW, &lock);

        // Check if bookID already exists
        fseek(library_handle.book_data_fp, 0, SEEK_SET);
        while (fread(&newBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
            if (newBook.bookID == bookID) {
                lock.l_type = F_UNLCK;
                fcntl(fileno(library_handle.book_data_fp), F_SETLK, &lock);  // Unlock
                return BOOK_ALREADY_EXISTS;
            }
        }

        // Write book details to file
        newBook.bookID = bookID;
        strncpy(newBook.title, title, MAX_BOOK_TITLE - 1);
        strncpy(newBook.author, author, MAX_USERNAME - 1);
        newBook.quantity = quantity;

        int putRecord = fwrite(&newBook, sizeof(struct Book), 1, library_handle.book_data_fp);
        if (putRecord != 1) {
            lock.l_type = F_UNLCK;
            fcntl(fileno(library_handle.book_data_fp), F_SETLK, &lock);  // Unlock
            return BOOK_ADD_FAILED;
        }

        // Increment book count in library handle
        library_handle.book_count++;

        lock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &lock);  // Unlock

        return LIB_SUCCESS;
    }
}



int library_find_book(int bookID, struct Book *foundBook) {
    if (library_handle.book_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct Book searchBook;
        
        // Seek to the beginning of the book data file
        fseek(library_handle.book_data_fp, 0, SEEK_SET);
        
        while (fread(&searchBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
            if (searchBook.bookID == bookID) {
                // If bookID matches, copy the book details to foundBook
                *foundBook = searchBook;
                return LIB_SUCCESS;  // Book found successfully
            }
        }
        
        return BOOK_NOT_FOUND;  // Book with given bookID not found
    }
}


int library_remove_book(int bookID) {
    if (library_handle.book_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        FILE *tempFile;
        FILE *bookFile;
        struct Book currentBook;
        int found = 0;

        // Open the original book data file for reading
        bookFile = fopen("Books.dat", "r");
        if (bookFile == NULL) {
            return LIB_FILE_ERROR;
        }

        // Open a temporary file for writing
        tempFile = fopen("temp.dat", "w");
        if (tempFile == NULL) {
            fclose(bookFile);
            return LIB_FILE_ERROR;
        }

        // Set a write lock on the whole file to prevent concurrent modifications
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = lseek(fileno(bookFile), 0, SEEK_END);
        fcntl(fileno(bookFile), F_SETLKW, &lock);

        // Seek to the beginning of the book data file
        fseek(bookFile, 0, SEEK_SET);

        // Copy records except the one with the specified bookID to the temporary file
        while (fread(&currentBook, sizeof(struct Book), 1, bookFile)) {
            if (currentBook.bookID != bookID) {
                fwrite(&currentBook, sizeof(struct Book), 1, tempFile);
            } else {
                found = 1; // Mark book as found
            }
        }

        lock.l_type = F_UNLCK;
        fcntl(fileno(bookFile), F_SETLK, &lock);  // Unlock

        fclose(bookFile);
        fclose(tempFile);

        // Remove the original file
        remove("Books.dat");

        // Rename the temporary file to the original filename
        rename("temp.dat", "Books.dat");

        // Reopen the updated book data file
        library_handle.book_data_fp = fopen("Books.dat", "r+");
        if (library_handle.book_data_fp == NULL) {
            return LIB_FILE_ERROR;
        }

        if (found) {
            library_handle.book_count--; // Decrement book count
            return LIB_SUCCESS;
        } else {
            return BOOK_NOT_FOUND;
        }
    }
}



int library_update_book(int bookID, char* newTitle, char* newAuthor, int newQuantity) {
    if (library_handle.book_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct Book currentBook;
        int found = 0;

        // Set a write lock on the whole file to prevent concurrent modifications
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = lseek(fileno(library_handle.book_data_fp), 0, SEEK_END);
        fcntl(fileno(library_handle.book_data_fp), F_SETLKW, &lock);

        // Seek to the beginning of the book data file
        fseek(library_handle.book_data_fp, 0, SEEK_SET);

        // Loop through records to find the book with the specified bookID
        while (fread(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
            if (currentBook.bookID == bookID) {
                // Update book details
                strncpy(currentBook.title, newTitle, MAX_BOOK_TITLE - 1);
                strncpy(currentBook.author, newAuthor, MAX_USERNAME - 1);
                currentBook.quantity = newQuantity;

                // Set file pointer to the position to write the updated record
                fseek(library_handle.book_data_fp, -sizeof(struct Book), SEEK_CUR);

                // Write the updated record back to the file
                fwrite(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp);

                found = 1; // Mark book as found
                break; // Exit loop after updating
            }
        }

        lock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &lock);  // Unlock

        if (found) {
            return LIB_SUCCESS; // Book updated successfully
        } else {
            return BOOK_NOT_FOUND; // Book with given bookID not found
        }
    }
}


int library_add_user(int userID, char* username, char* password, int isAdmin) {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct User newUser;
        
        // Set a write lock on the whole file to prevent concurrent modifications
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = lseek(fileno(library_handle.user_data_fp), 0, SEEK_END);
        fcntl(fileno(library_handle.user_data_fp), F_SETLKW, &lock);

        // Check if userID already exists
        fseek(library_handle.user_data_fp, 0, SEEK_SET);
        while (fread(&newUser, sizeof(struct User), 1, library_handle.user_data_fp)) {
            if (newUser.userID == userID) {
                lock.l_type = F_UNLCK;
                fcntl(fileno(library_handle.user_data_fp), F_SETLK, &lock);  // Unlock
                return USER_ALREADY_EXISTS;
            }
        }

        // Write user details to file
        newUser.userID = userID;
        strncpy(newUser.username, username, MAX_USERNAME - 1);
        strncpy(newUser.password, password, MAX_PASSWORD - 1);
        newUser.isAdmin = isAdmin;

        int putRecord = fwrite(&newUser, sizeof(struct User), 1, library_handle.user_data_fp);
        if (putRecord != 1) {
            lock.l_type = F_UNLCK;
            fcntl(fileno(library_handle.user_data_fp), F_SETLK, &lock);  // Unlock
            return USER_ADD_FAILED;
        }

        // Increment user count in library handle
        library_handle.user_count++;

        lock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.user_data_fp), F_SETLK, &lock);  // Unlock

        return LIB_SUCCESS;
    }
}

int library_remove_user(int userID) {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    }

    FILE *tempFile;
    FILE *userFile;
    struct User currentUser;
    int found = 0;

    // Open the original user data file for reading
    userFile = fopen("Users.dat", "r");
    if (userFile == NULL) {
        return LIB_FILE_ERROR;
    }

    // Open a temporary file for writing
    tempFile = fopen("temp.dat", "w");
    if (tempFile == NULL) {
        fclose(userFile);
        return LIB_FILE_ERROR;
    }

    // Set a write lock on the whole file to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = lseek(fileno(userFile), 0, SEEK_END);
    fcntl(fileno(userFile), F_SETLKW, &lock);

    // Seek to the beginning of the user data file
    fseek(userFile, 0, SEEK_SET);

    // Copy records except the one with the specified userID to the temporary file
    while (fread(&currentUser, sizeof(struct User), 1, userFile)) {
        if (currentUser.userID != userID) {
            fwrite(&currentUser, sizeof(struct User), 1, tempFile);
        } else {
            found = 1; // Mark user as found
        }
    }

    lock.l_type = F_UNLCK;
    fcntl(fileno(userFile), F_SETLK, &lock);  // Unlock

    fclose(userFile);
    fclose(tempFile);

    // Remove the original file
    remove("Users.dat");

    // Rename the temporary file to the original filename
    rename("temp.dat", "Users.dat");

    // Reopen the updated user data file
    library_handle.user_data_fp = fopen("Users.dat", "r+");
    if (library_handle.user_data_fp == NULL) {
        return LIB_FILE_ERROR;
    }

    if (found) {
        library_handle.user_count--; // Decrement user count
        return LIB_SUCCESS;
    } else {
        return USER_NOT_FOUND;
    }
}



int library_find_user(int userID, struct User *foundUser) {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct User searchUser;
        
        // Seek to the beginning of the user data file
        fseek(library_handle.user_data_fp, 0, SEEK_SET);
        
        while (fread(&searchUser, sizeof(struct User), 1, library_handle.user_data_fp)) {
            if (searchUser.userID == userID) {
                // If userID matches, copy the user details to foundUser
                *foundUser = searchUser;
                return LIB_SUCCESS;  // User found successfully
            }
        }
        
        return USER_NOT_FOUND;  // User with given userID not found
    }
}

int library_update_user(int userID, char* newUsername, char* newPassword, int newIsAdmin) {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct User currentUser;
        int found = 0;

        // Set a write lock on the whole file to prevent concurrent modifications
        struct flock lock;
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;
        lock.l_start = 0;
        lock.l_len = lseek(fileno(library_handle.user_data_fp), 0, SEEK_END);
        fcntl(fileno(library_handle.user_data_fp), F_SETLKW, &lock);

        // Seek to the beginning of the user data file
        fseek(library_handle.user_data_fp, 0, SEEK_SET);

        // Loop through records to find the user with the specified userID
        while (fread(&currentUser, sizeof(struct User), 1, library_handle.user_data_fp)) {
            if (currentUser.userID == userID) {
                // Update user details
                strncpy(currentUser.username, newUsername, MAX_USERNAME - 1);
                strncpy(currentUser.password, newPassword, MAX_PASSWORD - 1);
                currentUser.isAdmin = newIsAdmin;

                // Set file pointer to the position to write the updated record
                fseek(library_handle.user_data_fp, -sizeof(struct User), SEEK_CUR);

                // Write the updated record back to the file
                fwrite(&currentUser, sizeof(struct User), 1, library_handle.user_data_fp);

                found = 1; // Mark user as found
                break; // Exit loop after updating
            }
        }

        lock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.user_data_fp), F_SETLK, &lock);  // Unlock

        if (found) {
            return LIB_SUCCESS; // User updated successfully
        } else {
            return USER_NOT_FOUND; // User with given userID not found
        }
    }
}


int library_login(int userid, char* password) {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct User currentUser;

        // Seek to the beginning of the user data file
        fseek(library_handle.user_data_fp, 0, SEEK_SET);

        // Loop through records to find the user with the specified username and password
        while (fread(&currentUser, sizeof(struct User), 1, library_handle.user_data_fp)) {
            if (currentUser.userID  == userid && strcmp(currentUser.password, password) == 0) {
                if (currentUser.isAdmin == 1) {
                    printf("Welcome Librarian!\n");
                    return 1; // User is an admin
                } else {
                    printf("Welcome Student!\n");
                    return 0; // User is not an admin
                }
            }
        }

        printf("Wrong login credentials.\n");
        return 2; // User not found or wrong credentials
    }
}


int library_display_books() {
    if (library_handle.book_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct Book currentBook;

        // Seek to the beginning of the book data file
        fseek(library_handle.book_data_fp, 0, SEEK_SET);

        printf("List of Books:\n");
        while (fread(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
            printf("Book ID: %d, Title: %s, Author: %s, Quantity: %d\n", currentBook.bookID, currentBook.title, currentBook.author, currentBook.quantity);
        }

        return LIB_SUCCESS;
    }
}

int library_display_users() {
    if (library_handle.user_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    } else {
        struct User currentUser;

        // Seek to the beginning of the user data file
        fseek(library_handle.user_data_fp, 0, SEEK_SET);

        printf("List of Users:\n");
        while (fread(&currentUser, sizeof(struct User), 1, library_handle.user_data_fp)) {
            if (currentUser.isAdmin == 1) {
                printf("User ID: %d, Username: %s, User Type: Librarian\n", currentUser.userID, currentUser.username);
            } else {
                printf("User ID: %d, Username: %s, User Type: Student\n", currentUser.userID, currentUser.username);
            }
        }

        return LIB_SUCCESS;
    }
}

int library_add_to_cart(int userID, int bookID, int quantity) {
    if (library_handle.cart_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    }

    if (quantity <= 0) {
        return CART_ADD_FAILED; // Invalid quantity
    }

    struct Cart currentCart;
    struct Book currentBook;
    int foundCart = 0;
    int foundBook = 0;

    // Lock the cart data file for writing
    struct flock cartLock;
    cartLock.l_type = F_WRLCK;
    cartLock.l_whence = SEEK_SET;
    cartLock.l_start = 0;
    cartLock.l_len = 0;
    if (fcntl(fileno(library_handle.cart_data_fp), F_SETLKW, &cartLock) == -1) {
        return FILE_LOCK_FAILED;
    }

    // Lock the book data file for reading and writing
    struct flock bookLock;
    bookLock.l_type = F_WRLCK;
    bookLock.l_whence = SEEK_SET;
    bookLock.l_start = 0;
    bookLock.l_len = 0;
    if (fcntl(fileno(library_handle.book_data_fp), F_SETLKW, &bookLock) == -1) {
        // Unlock the cart data file before returning
        cartLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
        return FILE_LOCK_FAILED;
    }

    // Seek to the beginning of the book data file
    fseek(library_handle.book_data_fp, 0, SEEK_SET);

    // Find the book in the book data file
    while (fread(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
        if (currentBook.bookID == bookID) {
            foundBook = 1;
            break;
        }
    }

    if (!foundBook || currentBook.quantity < quantity) {
        // Unlock both files before returning
        bookLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
        cartLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
        return BOOK_NOT_FOUND;
    }

    // Seek to the beginning of the cart data file
    fseek(library_handle.cart_data_fp, 0, SEEK_SET);

    // Find the user's cart
    while (fread(&currentCart, sizeof(struct Cart), 1, library_handle.cart_data_fp)) {
        if (currentCart.userID == userID) {
            foundCart = 1;
            break;
        }
    }

    if (!foundCart) {
        // If cart not found, create a new cart for the user
        memset(&currentCart, 0, sizeof(struct Cart));
        currentCart.userID = userID;
        currentCart.itemCount = 0;
    }

    int foundItemInCart = 0;
    for (int i = 0; i < currentCart.itemCount; i++) {
        if (currentCart.items[i].bookID == bookID) {
            currentCart.items[i].quantity += quantity;
            foundItemInCart = 1;
            break;
        }
    }

    if (!foundItemInCart) {
        if (currentCart.itemCount < MAX_BOOKS) {
            currentCart.items[currentCart.itemCount].bookID = bookID;
            currentCart.items[currentCart.itemCount].quantity = quantity;
            currentCart.itemCount++;
        } else {
            // Unlock both files before returning
            bookLock.l_type = F_UNLCK;
            fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
            cartLock.l_type = F_UNLCK;
            fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
            return CART_ADD_FAILED;
        }
    }

    // Update the book's quantity in the book data file
    currentBook.quantity -= quantity;
    fseek(library_handle.book_data_fp, -sizeof(struct Book), SEEK_CUR);
    fwrite(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp);

    if (!foundCart) {
        // Add the new cart at the end of the cart data file
        fseek(library_handle.cart_data_fp, 0, SEEK_END);
    } else {
        // Update the existing cart in the cart data file
        fseek(library_handle.cart_data_fp, -sizeof(struct Cart), SEEK_CUR);
    }
    fwrite(&currentCart, sizeof(struct Cart), 1, library_handle.cart_data_fp);

    // Unlock both files
    bookLock.l_type = F_UNLCK;
    fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
    cartLock.l_type = F_UNLCK;
    fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);

    return LIB_SUCCESS;
}

int library_remove_from_cart(int userID, int bookID, int quantity) {
    if (library_handle.cart_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    }

    if (quantity <= 0) {
        return CART_REMOVE_FAILED; // Invalid quantity
    }

    struct Cart currentCart;
    struct Book currentBook;
    int foundCart = 0;
    int foundItemInCart = 0;

    // Lock the cart data file for writing
    struct flock cartLock;
    cartLock.l_type = F_WRLCK;
    cartLock.l_whence = SEEK_SET;
    cartLock.l_start = 0;
    cartLock.l_len = 0;
    if (fcntl(fileno(library_handle.cart_data_fp), F_SETLKW, &cartLock) == -1) {
        return FILE_LOCK_FAILED;
    }

    // Lock the book data file for reading and writing
    struct flock bookLock;
    bookLock.l_type = F_WRLCK;
    bookLock.l_whence = SEEK_SET;
    bookLock.l_start = 0;
    bookLock.l_len = 0;
    if (fcntl(fileno(library_handle.book_data_fp), F_SETLKW, &bookLock) == -1) {
        // Unlock the cart data file before returning
        cartLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
        return FILE_LOCK_FAILED;
    }

    // Seek to the beginning of the cart data file
    fseek(library_handle.cart_data_fp, 0, SEEK_SET);

    // Find the user's cart
    while (fread(&currentCart, sizeof(struct Cart), 1, library_handle.cart_data_fp)) {
        if (currentCart.userID == userID) {
            foundCart = 1;
            break;
        }
    }

    if (!foundCart) {
        // Unlock both files before returning
        bookLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
        cartLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
        return CART_NOT_FOUND;
    }

    for (int i = 0; i < currentCart.itemCount; i++) {
        if (currentCart.items[i].bookID == bookID) {
            if (currentCart.items[i].quantity >= quantity) {
                currentCart.items[i].quantity -= quantity;
                foundItemInCart = 1;
                break;
            } else {
                // If requested quantity is more than available in cart, remove the item completely
                quantity = currentCart.items[i].quantity;
                currentCart.items[i] = currentCart.items[currentCart.itemCount - 1];
                currentCart.itemCount--;
                foundItemInCart = 1;
                break;
            }
        }
    }

    if (!foundItemInCart) {
        // Unlock both files before returning
        bookLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
        cartLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);
        return CART_REMOVE_FAILED;
    }

    // Update the book's quantity in the book data file
    fseek(library_handle.book_data_fp, 0, SEEK_SET);
    while (fread(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
        if (currentBook.bookID == bookID) {
            currentBook.quantity += quantity;
            fseek(library_handle.book_data_fp, -sizeof(struct Book), SEEK_CUR);
            fwrite(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp);
            break;
        }
    }

    // Update the cart in the cart data file
    fseek(library_handle.cart_data_fp, -sizeof(struct Cart), SEEK_CUR);
    fwrite(&currentCart, sizeof(struct Cart), 1, library_handle.cart_data_fp);

    // Unlock both files
    bookLock.l_type = F_UNLCK;
    fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
    cartLock.l_type = F_UNLCK;
    fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);

    return LIB_SUCCESS;
}

int library_display_cart(int userID) {
    if (library_handle.cart_status == LIBRARY_CLOSED) {
        return LIB_FILE_ERROR;
    }

    struct Cart currentCart;
    struct Book currentBook;
    int foundCart = 0;

    // Lock the cart data file for reading
    struct flock cartLock;
    cartLock.l_type = F_RDLCK;
    cartLock.l_whence = SEEK_SET;
    cartLock.l_start = 0;
    cartLock.l_len = 0;
    if (fcntl(fileno(library_handle.cart_data_fp), F_SETLKW, &cartLock) == -1) {
        return FILE_LOCK_FAILED;
    }

    // Seek to the beginning of the cart data file
    fseek(library_handle.cart_data_fp, 0, SEEK_SET);

    // Find the user's cart
    while (fread(&currentCart, sizeof(struct Cart), 1, library_handle.cart_data_fp)) {
        if (currentCart.userID == userID) {
            foundCart = 1;
            break;
        }
    }

    // Unlock the cart data file
    cartLock.l_type = F_UNLCK;
    fcntl(fileno(library_handle.cart_data_fp), F_SETLK, &cartLock);

    if (!foundCart) {
        return CART_NOT_FOUND;
    }

    printf("Cart items for User ID %d:\n", userID);
    printf("%-10s %-30s %-15s %-10s\n", "Book ID", "Title", "Author", "Quantity");

    for (int i = 0; i < currentCart.itemCount; i++) {
        int bookID = currentCart.items[i].bookID;

        // Lock the book data file for reading
        struct flock bookLock;
        bookLock.l_type = F_RDLCK;
        bookLock.l_whence = SEEK_SET;
        bookLock.l_start = 0;
        bookLock.l_len = 0;
        if (fcntl(fileno(library_handle.book_data_fp), F_SETLKW, &bookLock) == -1) {
            return FILE_LOCK_FAILED;
        }

        // Seek to the beginning of the book data file
        fseek(library_handle.book_data_fp, 0, SEEK_SET);

        // Find and display book details
        while (fread(&currentBook, sizeof(struct Book), 1, library_handle.book_data_fp)) {
            if (currentBook.bookID == bookID) {
                printf("%-10d %-30s %-15s %-10d\n", currentBook.bookID, currentBook.title, currentBook.author, currentCart.items[i].quantity);
                break;
            }
        }

        // Unlock the book data file
        bookLock.l_type = F_UNLCK;
        fcntl(fileno(library_handle.book_data_fp), F_SETLK, &bookLock);
    }

    return LIB_SUCCESS;
}

int library_close() {
    if (library_handle.book_status == LIBRARY_OPEN) {
        fclose(library_handle.book_data_fp);
        library_handle.book_status = LIBRARY_CLOSED;
    }

    if (library_handle.user_status == LIBRARY_OPEN) {
        fclose(library_handle.user_data_fp);
        library_handle.user_status = LIBRARY_CLOSED;
    }

    if (library_handle.cart_status == LIBRARY_OPEN) {
        fclose(library_handle.cart_data_fp);
        library_handle.cart_status = LIBRARY_CLOSED;
    }



    return LIB_SUCCESS;
}

int server_fd;

void *client_handler(void *socket_desc);

void *client_handler(void *socket_desc) {
    int client_socket = *((int *)socket_desc);
    char buffer[BUFFER_SIZE];
    int read_size;
    int userid;
    char password[MAX_PASSWORD];
    char message[256];

    while (1) {
        read(client_socket, &userid, sizeof(userid));
        read(client_socket, password, sizeof(password));
        int login_result = library_login(userid, password);

        if (login_result == 0) {
            strcpy(message, "Welcome student\n");
        } else if (login_result == 1) {
            strcpy(message, "Welcome librarian\n");
        } else {
            strcpy(message, "Invalid login for the user\n");
            write(client_socket, message, strlen(message));
            continue;
        }
        write(client_socket, message, strlen(message));

        int option;
        while (1) {
            read(client_socket, &option, sizeof(option));
            switch (option) {
                case 1: {
                    struct Book new_book;
                    read(client_socket, &new_book, sizeof(new_book));
                    int result = library_add_book(new_book.bookID, new_book.title, new_book.author, new_book.quantity);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 2: {
                    int bookID;
                    read(client_socket, &bookID, sizeof(bookID));
                    int result = library_remove_book(bookID);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 3: {
                    int bookID;
                    struct Book found_book;
                    read(client_socket, &bookID, sizeof(bookID));
                    int result = library_find_book(bookID, &found_book);
                    write(client_socket, &result, sizeof(result));
                    if (result == LIB_SUCCESS) {
                        write(client_socket, &found_book, sizeof(found_book));
                    }
                    break;
                }
                case 4: {
                    int bookID;
                    struct Book updated_book;
                    read(client_socket, &bookID, sizeof(bookID));
                    read(client_socket, &updated_book, sizeof(updated_book));
                    int result = library_update_book(bookID, updated_book.title, updated_book.author, updated_book.quantity);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 5: {
                    struct User new_user;
                    read(client_socket, &new_user, sizeof(new_user));
                    int result = library_add_user(new_user.userID, new_user.username, new_user.password, new_user.isAdmin);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 6: {
                    int userID;
                    read(client_socket, &userID, sizeof(userID));
                    int result = library_remove_user(userID);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 7: {
                    int userID;
                    struct User found_user;
                    read(client_socket, &userID, sizeof(userID));
                    int result = library_find_user(userID, &found_user);
                    write(client_socket, &result, sizeof(result));
                    if (result == LIB_SUCCESS) {
                        write(client_socket, &found_user, sizeof(found_user));
                    }
                    break;
                }
                case 8: {
                    int userID;
                    struct User updated_user;
                    read(client_socket, &userID, sizeof(userID));
                    read(client_socket, &updated_user, sizeof(updated_user));
                    int result = library_update_user(userID, updated_user.username, updated_user.password, updated_user.isAdmin);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 9: {
                    int result = library_display_books();
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 10: {
                    int result = library_display_users();
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 11: {
                    int userID, bookID, quantity;
                    read(client_socket, &userID, sizeof(userID));
                    read(client_socket, &bookID, sizeof(bookID));
                    read(client_socket, &quantity, sizeof(quantity));
                    int result = library_add_to_cart(userID, bookID, quantity);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 12: {
                    int userID, bookID, quantity;
                    read(client_socket, &userID, sizeof(userID));
                    read(client_socket, &bookID, sizeof(bookID));
                    read(client_socket, &quantity, sizeof(quantity));
                    int result = library_remove_from_cart(userID, bookID, quantity);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                case 13: {
                    int userID;
                    read(client_socket, &userID, sizeof(userID));
                    int result = library_display_cart(userID);
                    write(client_socket, &result, sizeof(result));
                    break;
                }
                default: {
                    printf("Invalid option received\n");
                    break;
                }
            }
        }
    }

    close(client_socket);
    printf("Handler finished\n");
    pthread_exit(NULL);
}


int main() {
    library_create();
    library_open();
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t thread_id;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set SO_REUSEADDR option
    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("Setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Bind socket to IP and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", PORT);

    // Accept and handle client connections
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("Accept failed");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (pthread_create(&thread_id, NULL, client_handler, (void *)&client_socket) != 0) {
            perror("Thread creation failed");
            close(client_socket);
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id); // Detach the thread to avoid memory leaks
    }

    close(server_socket);
    return 0;
}