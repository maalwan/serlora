#include "ser.h"

int open_serial(char* serial_port) {
	int serial_fd;
	struct termios tty;
	
    // Open serial port
    serial_fd = open(serial_port, O_RDWR);
    if (serial_fd < 0) {
        perror("Error opening serial port");
        return -1;
    }

    // Configure serial port
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("Error from tcgetattr");
        close(serial_fd);
        return -1;
    }
    cfsetospeed(&tty, B230400); // Set baud rate to 230400
    cfsetispeed(&tty, B230400);
    tty.c_cflag |= (CLOCAL | CREAD); // Enable reading and ignore modem control signals
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // One stop bit
    tty.c_cflag &= ~CSIZE; // Clear data size bits
    tty.c_cflag |= CS8; // 8 bits per byte
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control
    tty.c_oflag &= ~OPOST; // Raw output
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) { // Set new attributes
        perror("Error from tcsetattr");
        close(serial_fd);
        return -1;
    }
	return serial_fd;
}

struct read_args {
	int serial_fd;
	size_t ms;
	char* buf;
	size_t len;
	int pipe_fd;
};

ssize_t backend_serial(void* args) {
	struct read_args* ra = (struct read_args*) args;
	// Read with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(ra->serial_fd, &read_fds);
	if (ra->pipe_fd != -1) {
		FD_SET(ra->pipe_fd, &read_fds);
	}
	struct timeval init_t = {.tv_sec = 0, .tv_usec = ra->ms * 1000};
	int select_result = select(ra->serial_fd + 1, &read_fds, NULL, NULL, ra->ms != 0 ? &init_t : NULL);
	// If pipe is signaled, quit early
	if (ra->pipe_fd != -1 && FD_ISSET(ra->pipe_fd, &read_fds)) {
		return 0;
	}
	// If select timeouts, quit early
	if (select_result < 0 || !FD_ISSET(ra->serial_fd, &read_fds)) {
		perror("Timeout");
		return -1;
	}
	// Read response continously
	int bytes_read = 0;
	while (FD_ISSET(ra->serial_fd, &read_fds)) {
		// Read response
		bytes_read += read(ra->serial_fd, ra->buf + bytes_read, ra->len - bytes_read);
		if (bytes_read < 0) {
			perror("Serial Error");
			return -1;
		}
		// Select again in case we are reading faster than we are recieving data
		// Setup timeout
		struct timeval norm_t = { .tv_sec = 0, .tv_usec = NORM_TIMEOUT};
		FD_ZERO(&read_fds); FD_SET(ra->serial_fd, &read_fds);
		// Wait for next bytes to come in
		select_result = select(ra->serial_fd + 1, &read_fds, NULL, NULL, &norm_t);
		if (select_result < 0) {
			perror("Select Error");
			return -1;
		}
	}
	ra->buf[bytes_read - 2] = '\0'; // null terminate and strip new lines
	return bytes_read - 2;
}


ssize_t read_serial(int serial_fd, size_t ms, char* buf, size_t len) {
	struct read_args ra = {
		.serial_fd = serial_fd,
		.ms = ms, 
		.buf = buf, 
		.len = len, 
		.pipe_fd = -1
	};
	return backend_serial((void*) &ra);
}

struct ser_async {
	struct read_args* ra;
	pthread_t pid;
	int pipe_fds[2];
	int rtn;
};

void* async_thread(void* args) {
	((ser_async*) args)->rtn = backend_serial(args);
	pthread_exit(NULL);
}

ser_async* read_serial_async(int serial_fd, char* buf, size_t len) {
	int pipe_fds[2];
	if (pipe(pipe_fds) == -1) {
		perror("Pipe Error");
		return NULL;
	}

	struct read_args* ra = malloc(sizeof(struct read_args));
	ra->serial_fd = serial_fd;
	ra->ms = 0;
	ra->buf = buf;
	ra->len = len;
	ra->pipe_fd = pipe_fds[0];

	ser_async* sa = malloc(sizeof(ser_async));
	sa->ra = ra;
	sa->pipe_fds[0] = pipe_fds[0];
	sa->pipe_fds[1] = pipe_fds[1];
	
	if (pthread_create(&sa->pid, NULL, async_thread, &ra) != 0) {
		perror("pthread_create Error");
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		free(ra);
		free(sa);
		return NULL;
	}
	
	return sa;
}

int ser_quit_async(ser_async* sa) {
	char stop_signal = 'X';
	int r = write(sa->pipe_fds[1], &stop_signal, sizeof(stop_signal));
    if (r < 0) {
        perror("Concurrency Error");
        return r;
    }
	r = ser_sync(sa);
	if (r < 0) {
		perror("Serial Read Error");
	}
	return r;
}

int ser_sync(ser_async* sa) {
	pthread_join(sa->pid, NULL);
	close(sa->pipe_fds[0]);
	close(sa->pipe_fds[1]);
	free(sa->ra);
	int r = sa->rtn;
	free(sa);
	return r;
}


/*
int read_serial(int serial_fd, char* buf, size_t len, int (*callback)(const char* buf)) {
    // Wait for data to become available for reading
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(serial_fd, &read_fds);
	if (callback != NULL) {
		FD_SET(STDIN_FILENO, &read_fds);
	}

	// Setup timeout
    struct timeval init_t = { .tv_sec = INIT_TIMEOUT, .tv_usec = 0};
	struct timeval norm_t = { .tv_sec = 0, .tv_usec = NORM_TIMEOUT};
    int select_result = -1;
	int bytes_read = 0;

	while (1) {
		// Read with timeout
		select_result = select(serial_fd + 1, &read_fds, NULL, NULL, callback != NULL ? NULL : &init_t);
		if (select_result == 0) {
			perror("Timeout");
			close(serial_fd);
			return -1;
		}
        if (callback != NULL && FD_ISSET(STDIN_FILENO, &read_fds)) {
            // User input available on stdin
            char input;
            if (read(STDIN_FILENO, &input, sizeof(input)) < 0) {
                perror("Error reading from stdin");
                return -1;
            }
            if (input == 'q') {
                return bytes_read;
            }
        }
		if (FD_ISSET(serial_fd, &read_fds)) {
			while (FD_ISSET(serial_fd, &read_fds)) {
				// Read response
				bytes_read += read(serial_fd, buf + bytes_read, len - bytes_read);
				if (bytes_read < 0) {
					perror("Error reading from serial port");
					close(serial_fd);
					return -1;
				}

				norm_t.tv_sec = 0; norm_t.tv_usec = NORM_TIMEOUT;
				FD_ZERO(&read_fds); FD_SET(serial_fd, &read_fds);
				// Wait for next bytes to come in
				select_result = select(serial_fd + 1, &read_fds, NULL, NULL, &norm_t);
				if (select_result < 0) {
					perror("Error in select");
					close(serial_fd);
					return -1;
				}
			}
			buf[bytes_read - 2] = '\0'; // null terminate and strip new line
			// callback(buf); // printf("%s\n", buf);
			if (callback == NULL) {
				return bytes_read - 2;
			} else {
				callback(buf);
				bytes_read = 0;
			}
		}
		FD_ZERO(&read_fds);
		FD_SET(serial_fd, &read_fds);
		FD_SET(STDIN_FILENO, &read_fds);
	}

	return -1; // Should never exit through here
}
*/
