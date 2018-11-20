/*
 * SD utils
 * 
 * Miscellaneous functions to handle with the SD card.
 * 
 */
 
#include "SDutils.h"

/*Wave header for PCM sound file */
struct waveHd {
  char  riff[4];                /* "RIFF"                                  					*/
  unsigned long  flength;       /* file length in bytes                    					*/
  char  wave[4];                /* "WAVE"                                  					*/
  char  fmt[4];                 /* "fmt "                                  					*/
  unsigned long  chunk_size;    /* size of FMT chunk in bytes (usually 16) 					*/
  short format_tag;             /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM 					*/
  short num_chans;              /* 1=mono, 2=stereo                        					*/
  unsigned long  srate;         /* Sampling rate in samples per second     					*/
  unsigned long  bytes_per_sec; /* bytes per second = srate*num_chan*bytes_per_samp	*/
  short bytes_per_samp;         /* 2=16-bit mono, 4=16-bit stereo          					*/
  short bits_per_samp;          /* Number of bits per sample               					*/
  char  data[4];                /* "data"                                  					*/
  unsigned long  dlength;       /* data length in bytes (filelength - 44)  					*/
}wave_header;
// SD card file handle
File frec;
// Total amount of recorded bytes
unsigned long tot_rec_bytes = 0;
// Path name of the recorded file, defined by GPS or remote data
String rec_path = "";

/* initSDcard(void)
 * ----------------
 * Initialize SPI port and mount SD card
 * IN:	- none
 * OUT:	- none
 */
void initSDcard(void) {
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if(!(SD.begin(SDCARD_CS_PIN))) {
    while(1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

/* createSDpath(struct gpsRMCtag)
 * -----------------
 * Create the folder/file path of the new recording
 * out of retrieved GPS or sent remote values.
 * IN:	- raw GPS tag (struct gpsRMCtag)
 * OUT:	- complete path (String)
 */
String createSDpath(struct gpsRMCtag tag) {
	Serial.println("Creating new folder/file on the SD card");
	String dirName = "";
	String fileName = "";
	String path = "/";
	char buf[12];
	sprintf(buf, "%02d", tag.date.year);
	dirName.concat(buf);
	sprintf(buf, "%02d", tag.date.month);
	dirName.concat(buf);
	sprintf(buf, "%02d", tag.date.day);
	dirName.concat(buf);

	sprintf(buf, "%02d", tag.time.h);
	fileName.concat(buf);
	sprintf(buf, "%02d", tag.time.min);
	fileName.concat(buf);
	sprintf(buf, "%02d", tag.time.sec);
	fileName.concat(buf);

	path.concat(dirName);
	path.concat("/");
	path.concat(fileName);
	path.concat(".wav");

	if(SD.exists(dirName)) {
		if(SD.exists(path)) {
			SD.remove(path);
		}
	}
	else {
		SD.mkdir(dirName);
	}
	return path;
}

/* initWaveHeader(void)
 * --------------------
 * Write all default values, that won't be changed
 * between different recordings.
 * IN:	- none
 * OUT:	- none
 */
void initWaveHeader(void) {
	// Initialize the wave header
	strncpy(wave_header.riff,"RIFF",4);
	strncpy(wave_header.wave,"WAVE",4);
	strncpy(wave_header.fmt,"fmt ",4);
	// <- missing here the filesize
	wave_header.chunk_size = WAVE_FMT_CHUNK_SIZE;
	wave_header.format_tag = WAVE_FORMAT_PCM;
	wave_header.num_chans = WAVE_NUM_CHANNELS;
	wave_header.srate = WAVE_SAMPLING_RATE;
	wave_header.bytes_per_sec = WAVE_BYTES_PER_SEC;
	wave_header.bytes_per_samp = WAVE_BYTES_PER_SAMP;
	wave_header.bits_per_samp = WAVE_BITS_PER_SAMP;
	strncpy(wave_header.data,"data",4);
	// <- missing here the data size
}

/* writeWaveHeader(path, dlen)
 * ---------------------------------
 * Write data & file length values to the wave header
 * when recording stop has been called
 * IN:	- file path (String)
 *			- number of recorded bytes (unsigned long)
 * OUT:	- none
 */
void writeWaveHeader(String path, unsigned long dlen) {
	File fh;
  wave_header.dlength = dlen;
  wave_header.flength = dlen + 36;
  
  fh = SD.open(path, O_WRITE);
  fh.seek(0);
  fh.write((byte*)&wave_header, 44);
  fh.close();
}
