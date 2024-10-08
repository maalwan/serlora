#ifndef TERM_INTERFACE_H_
#define TERM_INTERFACE_H_

#include <stdio.h>    // Standard input/output functions
#include <stdlib.h>   // Standard library functions (e.g., memory allocation)
#include <string.h>   // String handling functions
#include <pthread.h>  // POSIX threads (e.g., thread creation and synchronization)
#include <unistd.h>   // UNIX standard functions (e.g., read, write)
#include <fcntl.h>    // File control options (e.g., open)
#include <termios.h>  // Terminal I/O interfaces (e.g., setting baud rate)
#include <sys/select.h> // Functions for I/O multiplexing (e.g., select)

// Constants for terminal interface
#define MAX_HISTORY_SIZE 10       // Maximum number of command history entries
#define MAX_COMMAND_LENGTH 512    // Maximum length of a command

// Opaque term struct used to represent term interface object
typedef struct term term;

// Initializes the terminal interface and starts handling input.
//
// @param callback Function pointer to a callback that will be invoked with
//                 with a null terminated string.
// @param ptr Additional parameter passed to the callback function.
// @return 0 on success, or a non-zero value on error.
int term_interface(int (*callback)(char*, void*),
                   int (*cleanup)(void*),
                   void* ptr);

// Initializes the terminal interface and starts handling input asynchronously.
//
// @param callback Function pointer to a callback that will be invoked with
//                 with a null terminated string.
// @param ptr Additional parameter passed to the callback function.
// @return Pointer to a `term` structure representing the terminal interface,
//         or NULL on error.
term* term_interface_async(int (*callback)(char*, void*),
                           int (*cleanup)(void*),
                           void* ptr);

// Prints a string to the terminal interface.
//
// @param info Pointer to a `term` structure representing the terminal interface.
// @param str String to be printed.
void term_print(term* info, char* str);

// Waits for the terminal interface to finish its operations and joins the thread.
// Also cleans up memory used by the term object. The term object cannot be reused.
//
// @param info Pointer to a `term` structure representing the terminal interface.
// @return 0 on success, or a non-zero value on error.
int term_join(term* info);

// Conveys whether the terminal has been quit
//
// @return 1 if complete, 0 otherwise
int term_is_complete(term* info);

#endif  // TERM_INTERFACE_H_