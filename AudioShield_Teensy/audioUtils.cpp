/*
 * Audio utilities
 * 
 * Miscellaneous functions related to the
 * audio processes (recording, monitoring, etc.)
 * 
 */

#include "audioUtils.h"

// Audio connections definition
// GUItool: begin automatically generated code
AudioInputI2S									i2sRec;							//xy=172,125
AudioRecordQueue							queueSdc;						//xy=512,120
AudioAnalyzePeak							peak;								//xy=512,169
AudioPlaySdWav								playWav;						//xy=172,225
AudioMixer4										mixer;							//xy=350,225
AudioOutputI2S								i2sPlayMon;					//xy=512,228
AudioConnection								patchCord1(playWav, 0, mixer, 1);
AudioConnection								patchCord2(i2sRec, 0, queueSdc, 0);
AudioConnection								patchCord3(i2sRec, 0, peak, 0);
AudioConnection								patchCord4(i2sRec, 0, mixer, 0);
AudioConnection								patchCord5(mixer, 0, i2sPlayMon, 0);
AudioConnection								patchCord6(mixer, 0, i2sPlayMon, 1);
AudioControlSGTL5000					sgtl5000;						//xy=172,323
// // GUItool: end automatically generated code
const int                     audioInput = AUDIO_INPUT_LINEIN;


void prepareRecording(void) {
	bool ret;
	String rec_path = "";
	
	startLED(&leds[LED_RECORD], LED_MODE_WAITING);
	gpsPowerOn();
	delay(1000);
	ret = gpsGetData();
	gpsPowerOff();
	if(!ret) {
		// Serial.printf("No GPS fix received. timeStatus = %d, time = %ld\n", timeStatus(), now());
		// Time is not sychronized yet -> send a short warning
		if(timeStatus() == timeNotSet) {
			rec_path = createSDpath(false);
			startLED(&leds[LED_RECORD], LED_MODE_WARNING);
		}
		// Time synchronize from an older value
		else {
			rec_path = createSDpath(true);
			startLED(&leds[LED_RECORD], LED_MODE_ON);
		}
	}
	// GPS data valid -> adjust the internal time
	else {
		// adjustTime(TSOURCE_GPS);
		createSDpath(true);
	}
	setRecInfos(&next_record, rec_path, last_record.cnt);
}

void setRecInfos(struct recInfo* rec, String path, unsigned int rec_cnt) {
	unsigned long dur = rec_window.length.Second + 
										(rec_window.length.Minute * SECS_PER_MIN) + 
										(rec_window.length.Hour * SECS_PER_HOUR);
	rec->dur.Second = rec_window.length.Second;
	rec->dur.Minute = rec_window.length.Minute;
	rec->dur.Hour = rec_window.length.Hour;
	rec->path.remove(0);
	rec->path.concat(path.c_str());
	rec->cnt = rec_cnt;
	if(timeStatus() == timeNotSet) {
		rec->t_set = false;
		rec->ts = (time_t)0;
	}
	else {
		rec->t_set = true;
		rec->ts = now();
	}
	Serial.printf("Record information:\n-duration: %02dh%02dm%02ds\n-path: '%s'\n-time set: %d\n-rec time: %ld\n", rec->dur.Hour, rec->dur.Minute, rec->dur.Second, rec->path.c_str(), rec->t_set, rec->ts);
	Alarm.timerOnce(dur, alarmRecDone);	
}

/* startRecording(String)
 *
 */
void startRecording(String path) {
  Serial.println("Start recording");
  
  frec = SD.open(path, FILE_WRITE);
  if(frec) {
    queueSdc.begin();
    tot_rec_bytes = 0;
  }
  else {
    Serial.println("file opening error");
    while(1);
  }
}

/* continueRecording(void)
 *
 */
void continueRecording(void) {
  if(queueSdc.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queueSdc.readBuffer(), 256);
    queueSdc.freeBuffer();
    memcpy(buffer+256, queueSdc.readBuffer(), 256);
    queueSdc.freeBuffer();
    // write all 512 bytes to the SD card
//    elapsedMicros usec = 0;
    frec.write(buffer, 512);
    tot_rec_bytes += 512;
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queueSdc object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
//    Serial.print("SD write, us=");
//    Serial.println(usec);
  }
}

/* stopRecording(String)
 *
 */
void stopRecording(String path) {
  Serial.println("Stop recording");
  queueSdc.end();
  if(working_state.rec_state) {
    while(queueSdc.available() > 0) {
      frec.write((byte*)queueSdc.readBuffer(), 256);
      queueSdc.freeBuffer();
      tot_rec_bytes += 256;
    }
    frec.close();

    writeWaveHeader(path, tot_rec_bytes);
  }
  // working_state.rec_state = false;
}

/* startMonitoring(void)
 *
 */
void startMonitoring(void) {
	mixer.gain(MIXER_CH_REC, 1);
}

/* stopMonitoring(void)
 *
 */
void stopMonitoring(void) {
	mixer.gain(MIXER_CH_REC, 0);
	// working_state.mon_state = false;
}


/* initAudio(void)
 * ---------------
 */
void initAudio(void) {
  // Memory buffer for the record queue
  AudioMemory(60);

  // Enable the audio shield, select input, enable output
  sgtl5000.enable();
  sgtl5000.inputSelect(audioInput);
  sgtl5000.volume(0.5);
  mixer.gain(MIXER_CH_REC, 0);
  mixer.gain(MIXER_CH_SDC, 0);
}
