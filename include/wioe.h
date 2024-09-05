#ifndef WIOE_H_
#define WIOE_H_

#include "ser.h"  // Include serial communication functions
#include <sodium.h>

// Forward declaration of wioe structure
typedef struct wioe wioe;

// Structure to hold configuration parameters for the Wio-E5 module
typedef struct {
    double frequency;                // Frequency for communication in MHz
    unsigned char spreading_factor;  // Spreading factor for LoRa modulation
    unsigned short bandwidth;        // Bandwidth in kHz
    unsigned char tx_preamble;       // Preamble length for transmission
    unsigned char rx_preamble;       // Preamble length for reception
    unsigned char power;             // Transmission power in dBm
    unsigned char crc;               // CRC enable/disable flag
    unsigned char inverted_iq;       // IQ inversion flag
    unsigned char public_lorawan;    // Public LoRaWAN flag
} wioe_params;

// Constants for LoRa communication parameters
enum {
    MAXFREQ = 928,   // Maximum frequency in MHz
    MINFREQ = 902,   // Minimum frequency in MHz
    MAXSF = 12,      // Maximum spreading factor
    MINSF = 7,       // Minimum spreading factor
    BW1 = 125,       // Bandwidth option 1 in kHz
    BW2 = 250,       // Bandwidth option 2 in kHz
    BW3 = 500,       // Bandwidth option 3 in kHz
    MAXTX = 12,      // Maximum transmission power in dBm
    MINTX = 4,       // Minimum transmission power in dBm
    MAXRX = 12,      // Maximum reception power in dBm
    MINRX = 4,       // Minimum reception power in dBm
    MAXPOW = 22,     // Maximum power level
    MINPOW = 0       // Minimum power level
};

// Initializes a Wio-E5 device with specified parameters and serial port
//
// @param params Params used to specify the messaging characteristics of
//               of the module
// @serial_port The string representation of the serial_port the device
//              is connected to (i.e. if /dev/tty.usbserial20, then use
//              usbserial20).
// @return the wioe object for further use
wioe* wioe_init(wioe_params* params, char* serial_port);

// Updates the configuration of an existing Wio-E5 device
//
// @param device The initialized wioe device
// @param params Params used to specify the messaging characteristics of
//               of the module
// @return 0 on success, or a non-zero value on error.
int wioe_update(wioe* device, wioe_params* params);

// Sends data through the Wio-E5 device
//
// @param device The initialized wioe device
// @param data The data to be sent
// @param len Len in bytes of the data to be sent
// @return 0 on success, or a non-zero value on error.
int wioe_send(wioe* device, char* data, size_t len);

// Sends encrypted ChaCha20 data through the Wio-E5 device
//
// @param device The initialized wioe device
// @param data The data to be sent
// @param len Len in bytes of the data to be sent
// @param key The encryption key being used of len crypto_aead_chacha20poly1305_KEYBYTES
// @return 0 on success, or a non-zero value on error.
int wioe_send_encrypted(wioe* device, char* data, size_t len, const unsigned char *key);

// Receives data from the Wio-E5 device
//
// @param device The initialized wioe device
// @param buf The buffer where the recieved data should be stored
// @param len The size of the buffer
// @return 0 on success, or a non-zero value on error.
int wioe_recieve(wioe* device, char* buf, size_t len);

// Receives encrypted ChaCha20 data from the Wio-E5 device
//
// @param device The initialized wioe device
// @param buf The buffer where the recieved data should be stored
// @param len The size of the buffer
// @param key The encryption key being used of len crypto_aead_chacha20poly1305_KEYBYTES
// @return 0 on success, or a non-zero value on error.
int wioe_recieve_encrypted(wioe* device, char* buf, size_t len, const unsigned char *key);

// If the device is currently reading (in another thread), then this
// function will cancel the blocking function
// @param device The initialized wioe device
void wioe_cancel_recieve(wioe* device);

// Checks if the Wio-E5 device is valid and properly initialized
//
// @return 0 if invalid, 1 if valid
int wioe_is_valid(wioe* device);

// Cleans up and deallocates resources for the Wio-E5 device
//
// @param device The initialized wioe device
void wioe_destroy(wioe* device);

#endif /* WIOE_H_ */