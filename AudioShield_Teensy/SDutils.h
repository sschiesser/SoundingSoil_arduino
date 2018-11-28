/*
 * SDutils.h
 */
#ifndef _SDUTILS_H_
#define _SDUTILS_H_

#include "main.h"

// Wave header values
#define WAVE_FMT_CHUNK_SIZE		16
#define WAVE_FORMAT_PCM				1
#define WAVE_NUM_CHANNELS			1
#define WAVE_SAMPLING_RATE		44100
#define WAVE_BYTES_PER_SEC		705600 // 44100 * 16 
#define WAVE_BYTES_PER_SAMP		2
#define WAVE_BITS_PER_SAMP		16
#define WAVE_FLENGTH_POS			4
#define WAVE_DLENGTH_POS			40

// SDcard pins definition (Audio shield slot)
#define SDCARD_CS_PIN					10
#define SDCARD_MOSI_PIN				7
#define SDCARD_SCK_PIN				14

extern struct waveHd					wave_header;
extern File 									frec;
extern unsigned long 					tot_rec_bytes;



/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void initSDcard(void);
String createSDpath(void);
void initWaveHeader(void);
void writeWaveHeader(String path, unsigned long dlen);


#endif /* _SDUTILS_H_ */