#ifndef SER_H_
#define SER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <pthread.h>

#define INIT_TIMEOUT 1 // Read timeout in seconds
#define NORM_TIMEOUT 5000 // In nanoseconds

typedef struct ser_async ser_async;

int open_serial(char* serial_port);
ssize_t read_serial(int serial_fd, size_t ms, char* buf, size_t len);
ser_async* read_serial_async(int serial_fd, char* buf, size_t len);
int ser_quit_async(ser_async* sa);
int ser_sync(ser_async* sa);

#endif
