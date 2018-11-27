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
      Alarm.delay(500);
    }
  }
}

/* createSDpath(bool)
 * -----------------
 * Create the folder/file path of the new recording
 * out of retrieved GPS or sent remote values.
 * IN:	- time set (bool)
 * OUT:	- complete recording path (String)
 */
String createSDpath(bool time_set) {
	String dir_name = "";
	String file_name = "";
	String path = "";
	tmElements_t tm;
	char buf[24];
	
	// TIME SET:
	// Use date/time value to create the record path
	// with the format "YYMMDD/hhmmss.wav"
	if(time_set) {
		breakTime(next_record.ts, tm);
		// Serial.printf("Broken time: %02d.%02d.%02d, %02dh%02dm%02ds\n", tm.Day, tm.Month, (tm.Year-30), tm.Hour, tm.Minute, tm.Second);
		sprintf(buf, "%02d%02d%02d", (tm.Year-30), tm.Month, tm.Day);
		dir_name.concat(buf);
		sprintf(buf, "%02d%02d%02d.wav", tm.Hour, tm.Minute, tm.Second);
		file_name.concat(buf);
		sprintf(buf, "/%s/%s", dir_name.c_str(), file_name.c_str());
		path.concat(buf);	
		if(SD.exists(dir_name)) {
			if(SD.exists(path)) {
				SD.remove(path);
			}
		}
		else {
			SD.mkdir(dir_name);
		}
	}
	// TIME NOT SET:
	// Automatically increase the directory and file names with the following rules:
	// -	if new recording (e.g. REC button press) has been called,
	//		create a new empty directory at the lowest disponible value.
	// -	if within a window recording process (e.g. 1 minute/1 hour during 3 days),
	//		stay in the last directory and increase the file name.
	else {
		dir_name.concat("000000");
		file_name.concat("000000");
		unsigned int dir_val = 0;
		unsigned int file_val = 0;
		// Increase dir_name until no matching directory exists
		while(SD.exists(dir_name)) {
			dir_val = dir_name.toInt();
			dir_val += 1;
			sprintf(buf, "%06d", dir_val);
			dir_name.remove(0);
			dir_name.concat(buf);
		}
		// If NOT currently withing a window recording, create a fresh directory
		// if(working_state.mon_state != MONSTATE_ON) { // <-- for test purposes...
		if(working_state.rec_state != RECSTATE_RESTART) {
			SD.mkdir(dir_name);
		}
		// If waiting for the next window recording, stay in the same directory and increase file_name
		else {
			// Decrease dir_name to find the last existing directory
			dir_val = dir_name.toInt();
			dir_val -= 1;
			sprintf(buf, "%06d", dir_val);
			dir_name.remove(0);
			dir_name.concat(buf);
			// Generate whole path and check if it exists
			sprintf(buf, "/%s/%s.wav", dir_name.c_str(), file_name.c_str());
			path.remove(0);
			path.concat(buf);
			// Increase path until no name matches
			while(SD.exists(path)) {
				file_val = file_name.toInt();
				file_val += 1;
				sprintf(buf, "%06d", file_val);
				file_name.remove(0);
				file_name.concat(buf);
				sprintf(buf, "/%s/%s.wav", dir_name.c_str(), file_name.c_str());
				path.remove(0);
				path.concat(buf);
			}
		}
		sprintf(buf, "/%s/%s.wav", dir_name.c_str(), file_name.c_str());
		path.remove(0);
		path.concat(buf);
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
