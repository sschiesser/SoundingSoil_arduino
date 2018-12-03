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
String 												rec_path;
int														vol_ctrl;
elapsedMillis									peak_interval;

/* prepareRecording(bool)
 * ----------------------
 * Fetch a GPS fix (if demanded), set the record timestamp to now(),
 * create the file path according to it, save all record information
 * and start the record timer.
 * IN:	- sync with GPS (bool)
 * OUT:	- none
 */
void prepareRecording(bool sync) {
	bool gps_fix = true;
	
	startLED(&leds[LED_RECORD], LED_MODE_ON);
	if(sync) {
		gpsPowerOn();
		Alarm.delay(0);
		startLED(&leds[LED_RECORD], LED_MODE_WAITING);
		gps_fix = gpsGetData();
		gpsPowerOff();
		if(!gps_fix) {
			startLED(&leds[LED_RECORD], LED_MODE_WARNING_LONG);
		}
		else {
			time_source = TSOURCE_GPS;
			startLED(&leds[LED_RECORD], LED_MODE_ON);
		}
		next_record.ts = now();
		Serial.printf("Next record: %ld\n", next_record.ts);
	}
	rec_path = createSDpath();
	setRecInfos(&next_record, rec_path);
	unsigned long dur = next_record.dur.Second + 
										(next_record.dur.Minute * SECS_PER_MIN) + 
										(next_record.dur.Hour * SECS_PER_HOUR);
	alarm_rec_id = Alarm.timerOnce(dur, timerRecDone);	
}

/* setRecInfos(struct recInfos*, String)
 * -------------------------------------
 * Set the information related to the pointed recording.
 * IN:	- pointer to a record struct (struct recInfos*)
 *			- recording path name (String)
 * OUT:	- none
 */
void setRecInfos(struct recInfo* rec, String path) {
	rec->dur.Second = rec_window.length.Second;
	rec->dur.Minute = rec_window.length.Minute;
	rec->dur.Hour = rec_window.length.Hour;
	rec->path.remove(0);
	rec->path.concat(path.c_str());
	rec->t_set = (bool)rec->ts;
}

/* startRecording(String)
 * ----------------------
 * Open the file path and start the recording queue.
 * IN:	- file path (String)
 * OUT:	- none
 *
 */
void startRecording(String path) {
  frec = SD.open(path, FILE_WRITE);
  if(frec) {
    queueSdc.begin();
    tot_rec_bytes = 0;
  }
  else {
    Serial.println("file opening error");
		working_state.rec_state = RECSTATE_REQ_OFF;
    // while(1);
  }
}

/* continueRecording(void)
 * -----------------------
 * Write to the SD card and free the recording queue every 512 samples
 * IN:	- none
 * OUT:	- none
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
 * ---------------------
 * Stop the recording queue, write the remaining data
 * and the WAV header values to the SD card.
 * IN:	- none
 * OUT:	- none
 */
void stopRecording(String path) {
  // Serial.println("Stop recording");
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
}

/* pauseRecording(void)
 * --------------------
 * Prepare the next record informations.
 * IN:	- none
 * OUT:	- none
 */
void pauseRecording(void) {
	last_record = next_record;
	next_record.ts = last_record.ts + (rec_window.period.Hour * SECS_PER_HOUR) +
									(rec_window.period.Minute * SECS_PER_MIN) + rec_window.period.Second;
	// Serial.printf("Next record: %ld\n", next_record.ts);
	next_record.cnt++;
	stopLED(&leds[LED_RECORD]);
}


/* resetRecInfo(struct recInfo*)
 * -----------------------------
 * Set all values of the pointed record to 0
 * IN:	- pointer to the record (struct recInfo*)
 * OUT:	- none
 */
void resetRecInfo(struct recInfo* rec) {
	rec->ts = 0;
	rec->dur.Hour = 0;
	rec->dur.Minute = 0;
	rec->dur.Second = 0;
	rec->t_set = false;
	rec->path.remove(0);
	rec->cnt = 0;
}

/* finishRecording(void)
 * ---------------------
 * Reset the record informations, time source
 * and switch-off the REC LED.
 * IN:	- none
 * OUT:	- none
 */
void finishRecording(void) {
	resetRecInfo(&last_record);
	resetRecInfo(&next_record);
	time_source = TSOURCE_NONE;
	startLED(&leds[LED_RECORD], LED_MODE_WARNING_SHORT);
	// Wait until the notification is finished before sleeping or doing whatever.
	Alarm.delay(500);
}

/* startMonitoring(void)
 * ---------------------
 * To start monitoring, just open the mixer channel at the read gain value
 * IN:	- none
 * OUT:	- none
 */
void startMonitoring(void) {
	float gain;
	vol_ctrl = analogRead(AUDIO_VOLUME_PIN);
	gain = (float)vol_ctrl / 1023.0;
	mixer.gain(MIXER_CH_REC, gain);
}

/* stopMonitoring(void)
 * --------------------
 * To stop monitoring, close the mixer channel
 */
void stopMonitoring(void) {
	mixer.gain(MIXER_CH_REC, 0);
}

/* setHpGain(void)
 * ---------------
 * Read the rotary pot value and set the headphones gain.
 * IN:	- none
 * OUT:	- none
 */
void setHpGain(void) {
	float gain;
	vol_ctrl = analogRead(AUDIO_VOLUME_PIN);
	gain = (float)vol_ctrl / 1023.0;
	mixer.gain(MIXER_CH_REC, gain);
}

/* detectPeaks(void)
 * -----------------
 * Detect possible peaks at line input and
 * notify them with the peak LED.
 * IN:	- none
 * OUT:	- none
 */
void detectPeaks(void) {
	if(peak_interval > 24) {
		if(peak.available()) {
			peak_interval = 0;
			if(peak.read() >= 1.0) {
				startLED(&leds[LED_PEAK], LED_MODE_ON);
			}
			else {
				stopLED(&leds[LED_PEAK]);
			}
		}
	}
}

/* initAudio(void)
 * ---------------
 * Set memory to buffer audio signal, enable & set-up
 * the audio chip and the mixer.
 * IN:	- none
 * OUT:	- none
 */
void initAudio(void) {
  // Memory buffer for the record queue
  AudioMemory(60);

  // Enable the audio shield, select input, enable output
  sgtl5000.enable();
  sgtl5000.inputSelect(audioInput);
  sgtl5000.volume(SGTL5000_VOLUME_DEF);
	sgtl5000.lineInLevel(SGTL5000_INLEVEL_DEF);
  mixer.gain(MIXER_CH_REC, 0);
  mixer.gain(MIXER_CH_SDC, 0);
}
