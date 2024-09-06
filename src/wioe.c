#include "wioe.h"
#include <stdint.h>
#include <string.h>

#define BUFLEN 528
#define ISON(x) (x ? "ON" : "OFF")
#define init_t &()

// Structs and helper methods

struct wioe {
    wioe_params* actual_params;
    int serial_fd;
    int pipe_fd[2];
    char valid;
    pthread_mutex_t lock;
};

int wioe_handle_packet(char* buf, size_t len) {
    printf("\n%s\n", buf);
    // Check for error
    if (strstr(buf, "ERROR") != NULL) { return 0; }
    // Get the pkt in hexadecimal
    char pkt[BUFLEN];
    sscanf(buf, "+TEST: LEN:%*d, RSSI:%*d, SNR:%*d\n\n+TEST: RX \"%[^\"]\"", pkt);  
    // Convert to char
    char* pos = pkt;
    size_t count = 0;
    for (; count < (strlen(pkt) / 2); count++) {
        sscanf(pos, "%2hhx", &pkt[count]);
        pos += 2;
    }
    len = count <= len ? count : len;
    strncpy(buf, pkt, len);
    return len;
}

// Main methods

wioe* wioe_init(wioe_params* params, char* serial_port) {
    if (sodium_init() < 0) { return NULL; }
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) { return NULL; }
    int serial_fd = open_serial(serial_port);
    wioe* device = NULL;
    if (serial_fd >= 0) {
        device = (wioe*) malloc(sizeof(wioe));
        device->actual_params = (wioe_params*) malloc(sizeof(wioe));
        device->serial_fd = serial_fd;
        device->pipe_fd[0] = pipe_fd[0];
        device->pipe_fd[1] = pipe_fd[1];
        unsigned char buf[BUFLEN];
        int r = write(serial_fd, "AT+MODE=TEST\n", 14);
        r = read_serial(serial_fd, 1000, buf, sizeof(buf));
        if (r > 0) {
            r = wioe_update(device, params);
            device->valid = r ? 1 : 0;
        }
        if (pthread_mutex_init(&device->lock, NULL) != 0) { 
            wioe_destroy(device);
            return NULL;
        }
    }
    return device;
}

int wioe_update(wioe* device, wioe_params* params) {
    ssize_t r = 0;
    // First verify params
    if (params->frequency < MINFREQ || params->frequency > MAXFREQ
        || params->spreading_factor < MINSF || params->spreading_factor > MAXSF
        || (params->bandwidth != BW1 && params->bandwidth != BW2 && params->bandwidth != BW3)
        || params->tx_preamble < MINTX || params->tx_preamble > MAXTX
        || params->rx_preamble < MINRX || params->rx_preamble > MAXRX
        || params->power < MINPOW || params->power > MAXPOW) {
        return -1;
    }
    // Write command to update configuration
    unsigned char buf[BUFLEN];
    snprintf((char*) buf, BUFLEN - 1, "AT+TEST=RFCFG,F:%.6f,SF%i,%i,%i,%i,%i,%s,%s,%s\n",
        params->frequency,
        params->spreading_factor,
        params->bandwidth,
        params->tx_preamble,
        params->rx_preamble,
        params->power,
        ISON(params->crc),
        ISON(params->inverted_iq),
        ISON(params->public_lorawan));
    pthread_mutex_lock(&device->lock);
    r = write(device->serial_fd, (char*) buf, strlen((char*) buf) + 1);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    // Read to make sure there is no error
    pthread_mutex_lock(&device->lock);
    r = read_serial(device->serial_fd, 1000, buf, BUFLEN);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    if (strstr((char*) buf, "ERROR") != NULL) { return -1; }
    // Copy new parameters
    memcpy(&device->actual_params, params, sizeof(wioe_params));
    return 0;
}

int wioe_send_bytes(wioe* device, unsigned char* data, size_t len) {
    // Convert to a character representation of hex for wio-e5 device
    unsigned char hex_data[len*2+1];
    for (size_t i = 0; i < len; ++i)
        sprintf((char*) &hex_data[2*i], "%02hhX", data[i]);
    hex_data[len*2+1] = '\0';
    // Try sending to device
    if (!wioe_is_valid(device)) { return -1; }
    unsigned char buf[BUFLEN];
    snprintf((char*) buf, BUFLEN - 1, "AT+TEST=TXLRPKT,\"%s\"\n", hex_data);
    pthread_mutex_lock(&device->lock);
    ssize_t r = write(device->serial_fd, (char*) buf, strlen((char*) buf) + 1);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    // Read to make sure there is no error
    pthread_mutex_lock(&device->lock);
    r = read_serial(device->serial_fd, 1000, buf, BUFLEN);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    if (strstr((char*) buf, "ERROR") != NULL) { return -1; }
    // Read again if necessary to get +TEST: TX DONE
    if (strstr((char*) buf, "\n\n") == NULL) {
        pthread_mutex_lock(&device->lock);
        r = read_serial(device->serial_fd, 1000, buf, BUFLEN);
        pthread_mutex_unlock(&device->lock);
        if (r < 0) { return r; }
        if (strstr((char*) buf, "ERROR") != NULL) { return -1; }
    }
    return 0;
}

int wioe_send_encrypted(wioe* device, char* data, size_t len, const unsigned char *key) {
    // Get timestamp in nanoseconds as nonce
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t timestamp_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    unsigned char nonce[crypto_aead_chacha20poly1305_NPUBBYTES];
    memcpy(nonce, &timestamp_ns, sizeof(timestamp_ns));
    // Encrypt data using libsodium's chacha20poly1305
    unsigned char nonce_ciphertext[crypto_aead_chacha20poly1305_NPUBBYTES + 
                                   len + crypto_aead_chacha20poly1305_ABYTES];
    memcpy(nonce_ciphertext, &timestamp_ns, sizeof(timestamp_ns));
    unsigned long long ciphertext_len;
    crypto_aead_chacha20poly1305_encrypt(nonce_ciphertext + crypto_aead_chacha20poly1305_NPUBBYTES, &ciphertext_len,
                                         (unsigned char*) data, len,
                                         NULL, 0,
                                         NULL, nonce, key);
    return wioe_send_bytes(device, nonce_ciphertext, sizeof nonce_ciphertext);
}

int wioe_recieve_bytes(wioe* device, unsigned char* buf, size_t len) {
    if (!wioe_is_valid(device)) { return -1; }
    unsigned char cmd[BUFLEN];
    pthread_mutex_lock(&device->lock);
    ssize_t r = write(device->serial_fd, "AT+TEST=RXLRPKT\n", 17);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    // Read to make sure there is no error and to clear buffer
    pthread_mutex_lock(&device->lock);
    r = read_serial(device->serial_fd, 1000, cmd, BUFLEN);
    pthread_mutex_unlock(&device->lock);
    if (r < 0) { return r; }
    if (strstr((char*) buf, "ERROR") != NULL) { return -1; }
    // Start reading message while blocking
    pthread_mutex_lock(&device->lock);
    r = read_serial_trigger(device->serial_fd, 0, buf, len, device->pipe_fd[0]);
    pthread_mutex_unlock(&device->lock);
    if (r <= 0) { return r; }
    return wioe_handle_packet((char*) buf, len);
}

int wioe_recieve_encrypted(wioe* device, unsigned char* buf, size_t len, const unsigned char *key) {
    unsigned char nonce_ciphertext[BUFLEN];
    int ciphertext_len = wioe_recieve_bytes(device, nonce_ciphertext, sizeof nonce_ciphertext);
    if (ciphertext_len <= 0) { return ciphertext_len; }
    unsigned char decrypted[BUFLEN];
    unsigned long long decrypted_len;
    if (crypto_aead_chacha20poly1305_decrypt(decrypted, &decrypted_len,
                                             NULL,
                                             nonce_ciphertext + crypto_aead_chacha20poly1305_NPUBBYTES, 
                                             ciphertext_len - crypto_aead_chacha20poly1305_NPUBBYTES,
                                             NULL, 0,
                                             nonce_ciphertext, key) != 0) {
        /* message forged! */
        return -1;
    }
    len = decrypted_len > len ? len : decrypted_len;
    memcpy(buf, decrypted, len);
    return decrypted_len;
}

void wioe_cancel_recieve(wioe* device) {
    write(device->pipe_fd[1], "x", 1);
}

int wioe_is_valid(wioe* device) {
    return device == NULL ? 0 : device->valid;
}

void wioe_destroy(wioe* device) {
    close(device->serial_fd);
    close(device->pipe_fd[0]);
    close(device->pipe_fd[1]);
    pthread_mutex_destroy(&device->lock);
    free(device->actual_params);
    free(device);
}
