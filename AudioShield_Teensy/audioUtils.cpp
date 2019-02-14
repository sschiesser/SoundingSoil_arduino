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
AudioInputI2S            i2sRec;         //xy=67,33
AudioPlaySdWav           playWav;        //xy=76,217
AudioFilterBiquad        biquad1;        //xy=134,71
AudioFilterBiquad        biquad2;        //xy=164,107
AudioFilterBiquad        biquad3;        //xy=213,144
AudioFilterBiquad        biquad4;        //xy=259,183
AudioMixer4              monMixer;       //xy=486,212
AudioAnalyzePeak         peak;           //xy=686,113
AudioRecordQueue         queueSdc;       //xy=692,37
AudioOutputI2S           i2sMon;         //xy=700,207
AudioConnection          patchCord1(i2sRec, 0, biquad1, 0);
AudioConnection          patchCord2(playWav, 0, monMixer, 1);
AudioConnection          patchCord3(biquad1, biquad2);
AudioConnection          patchCord4(biquad2, biquad3);
AudioConnection          patchCord5(biquad3, biquad4);
AudioConnection          patchCord6(biquad4, 0, monMixer, 0);
AudioConnection          patchCord7(biquad4, peak);
AudioConnection          patchCord8(biquad4, queueSdc);
AudioConnection          patchCord9(monMixer, 0, i2sMon, 0);
AudioConnection          patchCord10(monMixer, 0, i2sMon, 1);
AudioControlSGTL5000     sgtl5000;       //xy=382,39
// GUItool: end automatically generated code
const int                     audioInput = AUDIO_INPUT_LINEIN;
String 												rec_path = "--";
int														vol_ctrl;
float													vol_value;
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
			setCurTime(0, TSOURCE_GPS);
			startLED(&leds[LED_RECORD], LED_MODE_ON);
		}
		next_record.ts = now();
		MONPORT.printf("Next record: %ld\n", next_record.ts);
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
    MONPORT.println("file opening error");
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
//    MONPORT.print("SD write, us=");
//    MONPORT.println(usec);
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
  // MONPORT.println("Stop recording");
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
	rec_path = "--";
	// MONPORT.printf("Next record: %ld\n", next_record.ts);
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
	rec_path = "--";
	if(time_source == TSOURCE_GPS) time_source = TSOURCE_TEENSY;
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
	monMixer.gain(MIXER_CH_REC, 1);
	// sgtl5000.lineOutLevel(GSTL5000_OUTLEVEL_DEF);
	setHpGain();
}

/* stopMonitoring(void)
 * --------------------
 * To stop monitoring, close the mixer channel
 */
void stopMonitoring(void) {
	monMixer.gain(MIXER_CH_REC, 0);
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
	gain = (float)vol_ctrl * 0.8 / 1023.0;
	sgtl5000.volume(gain);
	// sgtl5000.lineOutLevel(GSTL5000_OUTLEVEL_DEF);
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
	sgtl5000.lineOutLevel(GSTL5000_OUTLEVEL_DEF);
	biquad1.setNotch(0, 340, 0.5);
	biquad1.setNotch(1, 340, 0.5);
	biquad1.setNotch(2, 340, 0.5);
	biquad1.setNotch(3, 340, 0.5);
	biquad2.setNotch(0, 680, 0.5);
	biquad2.setNotch(1, 680, 0.5);
	biquad2.setNotch(2, 680, 0.5);
	biquad2.setNotch(3, 680, 0.5);
	biquad3.setNotch(0, 1020, 0.5);
	biquad3.setNotch(1, 1020, 0.5);
	biquad3.setNotch(2, 1020, 0.5);
	biquad3.setNotch(3, 1020, 0.5);
	biquad4.setNotch(0, 170, 0.5);
	biquad4.setNotch(1, 170, 0.5);
	biquad4.setNotch(2, 170, 0.5);
	biquad4.setNotch(3, 170, 0.5);
  monMixer.gain(MIXER_CH_REC, 0);
  monMixer.gain(MIXER_CH_SDC, 0);
}