#include <stdio.h>
#include <stdlib.h>

#include "term_interface.h"
#include "ser.h"
#include "wioe.h"

// Simple decision callback for RAW commands
int simple_callback(int argc, char** argv, void* serial_fd_ptr) {
    if (argc != 2) {
        puts("Invalid Command");
        return 0;
    }
	// Recover serial_fd
	int serial_fd = *((int*) serial_fd_ptr);
    
	// Create buffer for command that will be sent to device
    int len = strlen(argv[1]) + 1;
    char command[len]; memcpy(command, argv[1], len);
    command[len - 1] = '\n'; // Add \n for serial command
    
	// Write: non-blocking, Read: blocking
    if (!strcmp(argv[0], "write")) {
        // Write
        write(serial_fd, command, len);
        // Read
        char buf[1024];
        int bytes = read_serial(serial_fd, 100, buf, sizeof(buf));
        puts(buf);
        if (bytes < 0) { return 1; }
    } else if (!strcmp(argv[0], "read")) {
        // Write 
        write(serial_fd, command, len); 
        // Read 
        char buf[1024]; 
        int bytes = read_serial(serial_fd, 100, buf, sizeof(buf));
        puts(buf);
        if (bytes < 0) { return 1; }
    } else {
        puts("Invalid Command");
    }

    return 0;
}

// Callback for P2P using wioe.h
int p2p_callback(int argc, char** argv, void* device_ptr) {
	int r;
	// Recover device
	wioe* device = (wioe*) device_ptr;
	// Write and read
    if (!strcmp(argv[0], "write") && argc == 2) {
        // Write
        r = wioe_send(device, argv[1], 0); // Len can be 0 for now
		if (r < 0) {
			puts("Write failed");
			return 1;
		}
    } else if (!strcmp(argv[0], "read") && argc == 1) {
        // Read
		char buf[256];
        r = wioe_recieve(device, buf, sizeof(buf));
		if (r < 0) {
			puts("Read failed");
			return 1;
		}
    } else {
        printf("Invalid Command\n");
		return 0;
    }
    return 0;
}


int main(int argc, char** argv) {
    if (argc != 3){
        puts("usage: wio device_name RAW/P2P");
        return 1;
    }
    int r;
    char path[32];
    r = snprintf(path, sizeof(path), "/dev/cu.%s", argv[1]);
    if (r < 0) { return 1; }

	if (!strcmp(argv[2], "RAW")) {
		// Open serial port
		puts("1");
		int serial_fd = open_serial(path);
		puts("2");
		if (serial_fd < 0) { return 1; }
		puts("3");
		// Hand off control to custom terminal UI
		r = term_interface(&simple_callback, &serial_fd);
		// Close serial port
		close(serial_fd);
	} else if (!strcmp(argv[2], "P2P")) {
		wioe_params params = {
			.frequency = 915,
			.spreading_factor = 7,
			.bandwidth = 125,
			.tx_preamble = 6,
			.rx_preamble = 6,
			.power = 2,
			.crc = 1,
			.inverted_iq = 0,
			.public_lorawan = 0,
		};
		wioe* dev = wioe_init(&params, path);
		if (dev == NULL || !wioe_is_valid(dev)) {
			perror("Failed to initilize device\n");
			return 1;
		}
		r = term_interface(&p2p_callback, dev);
		wioe_destroy(dev);
		if (r < 0 ) { return 1; }
	}
    return 0;
}

