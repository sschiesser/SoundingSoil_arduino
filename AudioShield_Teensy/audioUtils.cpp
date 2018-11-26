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

/* prepareRecording(void)
 * ----------------------
 * Fetch a timestamp and set all the necessary informations
 * needed for the coming (next) record.
 * IN:	- none
 * OUT:	- none
 */
void prepareRecording(void) {
	String rec_path = "";
	
	startLED(&leds[LED_RECORD], LED_MODE_ON);
	gpsPowerOn();
	Alarm.delay(1000);
	startLED(&leds[LED_RECORD], LED_MODE_WAITING);
	gpsGetData();
	gpsPowerOff();
	if(timeStatus() == timeNotSet) {
		next_record.ts = 0;
		rec_path = createSDpath(false);
		startLED(&leds[LED_RECORD], LED_MODE_WARNING);
	}
	else {
		next_record.ts = now();
		rec_path = createSDpath(true);
		startLED(&leds[LED_RECORD], LED_MODE_ON);
	}
	setRecInfos(&next_record, rec_path, last_record.cnt);
}

/* setRecInfos(struct recInfos*, String, unsigned int)
 * ---------------------------------------------------
 * Set the information related to the pointed recording
 * and start the alarm at recording duration.
 * IN:	- pointer to a record struct (struct recInfos*)
 *			- recording path name (String)
 *			- record counter (unsigned int)
 * OUT:	- none
 */
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
	rec->t_set = (bool)rec->ts;
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
//    elapsedMicros usec = 0;
    frec.write(buffer, 512);
    tot_rec_bytes += 512;
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

/* pauseRecording(void)
 *
 */
void pauseRecording(void) {
	last_record = next_record;
	if(last_record.t_set) {
		next_record.ts = last_record.ts + (rec_window.period.Hour * SECS_PER_HOUR) +
											(rec_window.period.Minute * SECS_PER_MIN) + rec_window.period.Second;
		Serial.printf("Time set. Last record: %ld, next record: %ld\n", last_record.ts, next_record.ts);
	}
	else {
		Serial.printf("Time NOT set... need to define next ts\n");
	}
	stopLED(&leds[LED_RECORD]);
}

/* finishRecording(void)
 * ---------------------
 *
 */
void finishRecording(void) {
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
