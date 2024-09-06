#include <stdio.h>
#include <stdlib.h>

#include "term_interface.h"
#include "ser.h"
#include "wioe.h"

struct callback_args {
    wioe* device;
    unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];
};

// Callback for P2P using wioe.h
int p2p_callback(char* arg, void* info_args) {
    // Recover args
    struct callback_args* info = (struct callback_args*) info_args;
    wioe* device = info->device;
    unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];
    memcpy(key, &info->key, crypto_aead_chacha20poly1305_KEYBYTES);
    // Stop the reading in the other thread
    wioe_cancel_recieve(device);
    // Send the message
    wioe_send_encrypted(device, arg, strlen(arg) + 1, key);
    return 0;
}

// Callback for P2P using wioe.h
int p2p_cleanup(void* info_args) {
    // Recover args
    struct callback_args* info = (struct callback_args*) info_args;
    wioe* device = info->device;
    wioe_cancel_recieve(device);
    return 0;
}

int main(int argc, char** argv) {
    // Args
    if (argc != 3){
        puts("usage: wio device_path password");
        return EXIT_FAILURE;
    }
    // Get path
    int r;
    char path[32];
    r = snprintf(path, sizeof(path), "/dev/cu.%s", argv[1]);
    if (r < 0) { return 1; }
    // Get key from password
    unsigned char salt[crypto_pwhash_SALTBYTES];
    memset(salt, 0, sizeof salt);
    unsigned char key[crypto_aead_chacha20poly1305_KEYBYTES];
    if (crypto_pwhash
        (key, sizeof key, argv[2], strlen(argv[2]), salt,
         crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
         crypto_pwhash_ALG_DEFAULT) != 0) {
        // out of memory
        return EXIT_FAILURE;
    }
    printf("Key: ");
    for (int i = 0; i < crypto_aead_chacha20poly1305_KEYBYTES; i++) {
        printf("%02hhx ", key[i]);
    }
    printf("\n");

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
    struct callback_args info_args;
    info_args.device = dev;
    memcpy(info_args.key, key, sizeof key);
    term* info = term_interface_async(&p2p_callback, &p2p_cleanup, (void*) &info_args);

    // Basic communication protocol
    while (!term_is_complete(info)) {
        unsigned char buf[256];
        int bytes = wioe_recieve_encrypted(dev, buf, sizeof buf, key);
        if (bytes > 0) {
            // Null terminate buf
            buf[bytes] = '\0';
            // Output
            char out[512];
            snprintf(out, sizeof(out), "\033[1;31mRecieved:\033[0m %s", buf);
            term_print(info, out);
        } else if (bytes < 0) {
            printf("error");
        }
        usleep(20000);  // 20 ms
    }

    // Cleanup
    wioe_destroy(dev);
    if (r < 0 ) { return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
