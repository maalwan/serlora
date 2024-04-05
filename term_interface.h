#ifndef TERM_INTERFACE_H_
#define TERM_INTERFACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

#define MAX_HISTORY_SIZE 10
#define MAX_COMMAND_LENGTH 512

int term_interface(int (*callback)(int, char**, void*), void* ptr);

#endif
