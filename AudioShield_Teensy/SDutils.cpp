/*
* SD utils
*
* Miscellaneous functions to handle with the SD card.
*
*/

#include "SDutils.h"

// Wave header for PCM sound file
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
};
struct waveHd									wave_header;

// SD card file handles
File frec;
File fgps;

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
    // COMMENT FOR BUILTIN_SDCARD AND SSSHIELD V1.0 !!
    SPI.setMISO(SDCARD_MISO_PIN);
    SPI.setSCK(SDCARD_SCK_PIN);
    if(!(SD.begin(SDCARD_CS_PIN))) {
        while(1) {
            if(debug) snooze_usb.printf("SD:      Unable to access the SD card on CS: %d, MOSI: %d, SCK: %d\n", SDCARD_CS_PIN, SDCARD_MOSI_PIN, SDCARD_SCK_PIN);
            startLED(&leds[LED_RECORD], LED_MODE_ON);
            startLED(&leds[LED_MONITOR], LED_MODE_ON);
            startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
            Alarm.delay(250);
            stopLED(&leds[LED_RECORD]);
            stopLED(&leds[LED_MONITOR]);
            stopLED(&leds[LED_BLUETOOTH]);
            Alarm.delay(250);
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
String createSDpath(void) {
    String dir_name = "";
    String file_name = "";
    String path = "";
    tmElements_t tm;
    char buf[24];

    if(time_source != TSOURCE_NONE) {
        breakTime(now(), tm);
        sprintf(buf, "%02d%02d%02d", (tm.Year-30), tm.Month, tm.Day);
        dir_name.concat(buf);
        sprintf(buf, "%02d%02d%02d.wav", tm.Hour, tm.Minute, tm.Second);
        file_name.concat(buf);
    }
    else {
        breakTime(next_record.ts, tm);
        sprintf(buf, "u%02d%02d%02d", (tm.Year-30), tm.Month, tm.Day);
        dir_name.concat(buf);
        sprintf(buf, "u%02d%02d%02d.wav", tm.Hour, tm.Minute, tm.Second);
        file_name.concat(buf);
    }
    sprintf(buf, "/%s/%s", dir_name.c_str(), file_name.c_str());
    path.concat(buf);
    if(SD.exists(dir_name.c_str())) {
        if(SD.exists(path.c_str())) {
            SD.remove(path.c_str());
        }
    }
    else {
        SD.mkdir(dir_name.c_str());
    }

    // String temppath = "il etait une fois un petit canard vert...";
    // return temppath;
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
* ----------------------------
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

    fh = SD.open(path.c_str(), O_WRITE);
    fh.seek(0);
    fh.write((byte*)&wave_header, 44);
    fh.close();
}


/* createMetadata(*rec, path)
* ---------------------------
* Write the metadata file corresponding to the current recording.
* File content:
* - recording full path
* - recording time/date (timestamp)
* - duration/period
* - recording number # of total number
* - GPS latitude (DD)
* - GPS longitude (DD)
*/
void createMetadata(struct recInfo* rec) {
    tmElements_t tm;
    File fh;
    fh = SD.open(rec->mpath.c_str(), FILE_WRITE);
    if(debug) snooze_usb.printf("SD:      Opening: %s\n", rec->mpath.c_str());
    if(fh) {
        fh.printf("Recording meta-data (file: %s)\n", rec->mpath.c_str());
        fh.printf("----------------------------------------------\n");
        fh.printf("- recording path: %s\n", rec->rpath.c_str());
        // if(debug) snooze_usb.printf("SD:      Recording meta-data (file: %s)\n", rec->mpath.c_str());
        // if(debug) snooze_usb.printf("SD:      ----------------------------------------------\n");
        // if(debug) snooze_usb.printf("SD:      - recording path: %s\n", rec->rpath.c_str());
        breakTime(rec->ts, tm);
        fh.printf("- recording date/time: %02d.%02d.%d, %d:%02d'%02d\"\n", tm.Day, tm.Month, (tm.Year+1970), tm.Hour, tm.Minute, tm.Second);
        // if(debug) snooze_usb.printf("SD:      - recording date/time: %02d.%02d.%d, %d:%02d'%02d\"\n", tm.Day, tm.Month, (tm.Year+1970), tm.Hour, tm.Minute, tm.Second);
        fh.printf("- recording duration/period: %d:%02d'%02d\" / %d:%02d'%02d\"\n", rec->dur.Hour, rec->dur.Minute, rec->dur.Second, rec->per.Hour, rec->per.Minute, rec->per.Second);
        fh.printf("- recording #%d of %d\n", (rec->cnt+1), rec->rec_tot);
        fh.printf("- device position (lat, long (DD)): %0.5f, %0.5f\n", rec->gps_lat, rec->gps_long);
        // if(debug) snooze_usb.printf("SD:      - recording duration/period: %d:%02d'%02d\" / %d:%02d'%02d\"\n", rec->dur.Hour, rec->dur.Minute, rec->dur.Second, rec->per.Hour, rec->per.Minute, rec->per.Second);
        // if(debug) snooze_usb.printf("SD:      - recording #%d of %d\n", (rec->cnt+1), rec->rec_tot);
        // if(debug) snooze_usb.printf("SD:      - position (lat, long (DD)): %0.5f, %0.5f\n", rec->gps_lat, rec->gps_long);
    }
    fh.close();
}
