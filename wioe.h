#ifndef WIOE_H_
#define WIOE_H_

#include "ser.h"

typedef struct wioe wioe;
typedef struct {
	double frequency;
	unsigned char spreading_factor;
	unsigned short bandwidth;
	unsigned char tx_preamble;
	unsigned char rx_preamble;
	unsigned char power;
	unsigned char crc;
	unsigned char inverted_iq;
	unsigned char public_lorawan;
} wioe_params;

enum {
	MAXFREQ = 928, MINFREQ = 902,
	MAXSF = 12, MINSF = 7,
	BW1 = 125, BW2 = 250, BW3 = 500,
	MAXTX = 12, MINTX = 4,
	MAXRX = 12, MINRX = 4,
	MAXPOW = 22, MINPOW = 0
};

wioe* wioe_init(wioe_params* params, char* serial_port);
int wioe_update(wioe* device, wioe_params* params);
int wioe_send(wioe* device, char* data, size_t len);
int wioe_recieve(wioe* device, char* buf, size_t len);
int wioe_is_valid(wioe* device);
void wioe_destroy(wioe* device);

#endif
