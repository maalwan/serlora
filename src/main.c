#include <stdio.h>
#include <stdlib.h>

#include "term_interface.h"
#include "ser.h"
#include "wioe.h"

// Callback for P2P using wioe.h
int p2p_callback(int argc, char** argv, void* device_ptr) {
    // Recover device
    wioe* device = (wioe*) device_ptr;
    // Stop the reading in the other thread
    wioe_cancel_recieve(device);
    // Send the message
    wioe_send(device, argv[0], strlen(argv[0]) + 1);
    return 0;
}

int main(int argc, char** argv) {
    // Get path
    if (argc != 2){
        puts("usage: wio device_path");
        return EXIT_FAILURE;
    }
    int r;
    char path[32];
    r = snprintf(path, sizeof(path), "/dev/cu.%s", argv[1]);
    if (r < 0) { return 1; }

    // Setup device
    wioe_params params = {
        .frequency = 915,
        .spreading_factor = 7,
        .bandwidth = 500,
        .tx_preamble = 6,
        .rx_preamble = 6,
        .power = 0,
        .crc = 1,
        .inverted_iq = 0,
        .public_lorawan = 0,
    };
    wioe* dev = wioe_init(&params, path);
    if (dev == NULL || !wioe_is_valid(dev)) {
        perror("Failed to initilize device");
        return EXIT_FAILURE;
    }

    // Setup terminal
    term* info = term_interface_async(&p2p_callback, dev);

    // Basic communication protocol
    while (!term_is_complete(info)) {
        char buf[256];
        if (wioe_recieve(dev, buf, 256)) {
            term_print(info, buf);
        }
        usleep(2000);
    }

    // Cleanup
    wioe_destroy(dev);
    if (r < 0 ) { return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
