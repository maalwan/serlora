#ifndef SER_H_
#define SER_H_

#include <stdio.h>    // Standard input/output functions
#include <stdlib.h>   // Standard library functions (e.g., memory allocation)
#include <string.h>   // String handling functions
#include <unistd.h>   // UNIX standard functions (e.g., read, write)
#include <fcntl.h>    // File control options (e.g., open)
#include <termios.h>  // Terminal I/O interfaces (e.g., setting baud rate)
#include <sys/select.h> // Functions for I/O multiplexing (e.g., select)
#include <pthread.h>  // POSIX threads (e.g., thread creation and synchronization)

// Constants for timeouts
#define INIT_TIMEOUT 1      // Initial read timeout in seconds
#define NORM_TIMEOUT 5000   // Normal read timeout in nanoseconds

// Opens a serial port for communication.
//
// @param serial_port Path to the serial port device (e.g., "/dev/ttyS0").
// @return File descriptor for the opened serial port, or -1 on error.
int open_serial(char* serial_port);

// Reads data from the serial port.
//
// @param serial_fd File descriptor for the serial port.
// @param ms Timeout for read operation in milliseconds.
// @param buf Buffer to store the read data.
// @param len Length of the buffer.
// @return Number of bytes read, or -1 on error.
ssize_t read_serial(int serial_fd, size_t ms, char* buf, size_t len);

// Reads data from the serial port that takes trigger to prematurely end read
// that is thread safe.
//
// @param serial_fd File descriptor for the serial port.
// @param ms Timeout for read operation in milliseconds.
// @param buf Buffer to store the read data.
// @param len Length of the buffer.
// @param trigger_fd File descriptor of pipe used to cancel read
// @return Number of bytes read, or -1 on error.
ssize_t read_serial_trigger(int serial_fd, size_t ms, char* buf, size_t len, 
                            int trigger_fd);

#endif  // SER_H_
