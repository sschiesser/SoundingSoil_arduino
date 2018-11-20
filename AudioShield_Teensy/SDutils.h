/*
 * SDutils.h
 */
 
#ifndef _SDUTILS_H_
#define _SDUTILS_H_

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "gpsRoutines.h"

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

// SDcard pins definition (Audio Shield slot)
#define SDCARD_CS_PIN					10
#define SDCARD_MOSI_PIN				7
#define SDCARD_SCK_PIN				14

//Wave header variable
extern struct waveHd					wave_header;
// SD card file handle
extern File 									frec;
// Total amount of recorded bytes
extern unsigned long 					tot_rec_bytes;
// Path name of the recorded file, defined by GPS or remote data
extern String 								rec_path;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */

void initSDcard(void);
String createSDpath(struct gpsRMCtag tag);
void initWaveHeader(void);
void writeWaveHeader(String path, unsigned long dlen);


#endif /* _SDUTILS_H_ */