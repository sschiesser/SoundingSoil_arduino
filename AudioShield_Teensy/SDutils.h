/*
* SDutils.h
*/
#ifndef _SDUTILS_H_
#define _SDUTILS_H_

#include "main.h"
#include <SD.h>

// Wave header values
#define WAVE_FMT_CHUNK_SIZE         16
#define WAVE_FORMAT_PCM				1
#define WAVE_NUM_CHANNELS			1
#define WAVE_SAMPLING_RATE          44100
#define WAVE_BYTES_PER_SEC          705600 // 44100 * 16
#define WAVE_BYTES_PER_SAMP         2
#define WAVE_BITS_PER_SAMP          16
#define WAVE_FLENGTH_POS			4
#define WAVE_DLENGTH_POS			40

// SDcard pins definition
// -> Audio shield slot !! USED IN SSSHIELD V1.0 !!!
// #define SDCARD_CS_PIN				10
// #define SDCARD_MOSI_PIN				7
// #define SDCARD_SCK_PIN				14
// -> Teensy built-in
// #define SDCARD_CS_PIN			    BUILTIN_SDCARD
// #define SDCARD_MOSI_PIN				11
// #define SDCARD_SCK_PIN				13
// -> Custom SDcard adapter !! TO USE FOR SSSHIELD V2.0+ !!!
#define SDCARD_CS_PIN				10
#define SDCARD_MOSI_PIN				28
#define SDCARD_MISO_PIN				39
#define SDCARD_SCK_PIN				27

extern struct waveHd				wave_header;
extern File 						frec;
extern File							fmeta;
extern unsigned long 				tot_rec_bytes;



/* ======================
* FUNCTIONS DECLARATIONS
* ====================== */
void initSDcard(void);
String createSDpath(void);
void createMetadata(struct recInfo* rec);
void initWaveHeader(void);
void writeWaveHeader(String path, unsigned long dlen);


#endif /* _SDUTILS_H_ */
