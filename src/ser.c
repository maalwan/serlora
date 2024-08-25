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

ssize_t read_serial(int serial_fd, size_t ms, char* buf, size_t len) {
	// Read with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(serial_fd, &read_fds);
	struct timeval init_t = {.tv_sec = ms / 1000, .tv_usec = (ms % 1000) * 1000};
	int select_result = select(serial_fd + 1, &read_fds, NULL, NULL, ms != 0 ? &init_t : NULL);
	// If select timeouts, quit early
	if (select_result < 0 || !FD_ISSET(serial_fd, &read_fds)) {
		perror("Timeout");
		return -1;
	}
	// Read response continously
	int bytes_read = 0;
	while (FD_ISSET(serial_fd, &read_fds)) {
		// Read response
		bytes_read += read(serial_fd, buf + bytes_read, len - bytes_read);
		if (bytes_read < 0) {
			perror("Serial Error");
			return -1;
		}
		// Select again in case we are reading faster than we are recieving data
		// Setup timeout
		struct timeval norm_t = { .tv_sec = 0, .tv_usec = NORM_TIMEOUT};
		FD_ZERO(&read_fds); FD_SET(serial_fd, &read_fds);
		// Wait for next bytes to come in
		select_result = select(serial_fd + 1, &read_fds, NULL, NULL, &norm_t);
		if (select_result < 0) {
			perror("Select Error");
			return -1;
		}
	}
	buf[bytes_read - 2] = '\0'; // null terminate and strip new lines
	return bytes_read - 2;
}