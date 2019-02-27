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
AudioPlaySdWav           playWav;        //xy=131.6666717529297,453
AudioInputI2S            i2sRec;         //xy=133.6666717529297,36
// AudioFilterBiquad        biquad1;        //xy=164.6666717529297,69
// AudioFilterBiquad        biquad2;        //xy=189.6666717529297,101
// AudioFilterBiquad        biquad3;        //xy=212.6666717529297,134
// AudioFilterBiquad        biquad4;        //xy=234.6666717529297,167
// AudioFilterBiquad        biquad5;        //xy=259.6666717529297,199
// AudioFilterBiquad        biquad6;        //xy=276.6666717529297,232
// AudioFilterBiquad        biquad7;        //xy=301.6666717529297,265
// AudioFilterBiquad        biquad8;        //xy=324.6666717529297,298
// AudioFilterBiquad        biquad9;        //xy=346.6666717529297,331
// AudioFilterBiquad        biquad10;       //xy=371.6666717529297,364
AudioMixer4              monMixer;       //xy=566.6666564941406,455.0000114440918
AudioMixer4              recMixer;         //xy=570.3334274291992,346.0000114440918
AudioAnalyzePeak         peak;           //xy=752.6666717529297,116
AudioRecordQueue         queueSdc;       //xy=758.6666717529297,40
AudioOutputUSB           usbRec;           //xy=757.6666679382324,352.0000457763672
AudioOutputI2S           i2sMon;         //xy=766.6666717529297,459.00001335144043
AudioConnection          patchCord1(playWav, 0, monMixer, 1);
// AudioConnection          patchCord2(i2sRec, 0, biquad1, 0);
// AudioConnection          patchCord3(biquad1, biquad2);
// AudioConnection          patchCord4(biquad2, biquad3);
// AudioConnection          patchCord5(biquad3, biquad4);
// AudioConnection          patchCord6(biquad4, biquad5);
// AudioConnection          patchCord7(biquad5, biquad6);
// AudioConnection          patchCord8(biquad6, biquad7);
// AudioConnection          patchCord9(biquad7, biquad8);
// AudioConnection          patchCord10(biquad8, biquad9);
// AudioConnection          patchCord11(biquad9, biquad10);
AudioConnection          patchCord12(i2sRec, 0, monMixer, 0);
AudioConnection          patchCord13(i2sRec, peak);
AudioConnection          patchCord14(i2sRec, queueSdc);
AudioConnection          patchCord15(i2sRec, 0, recMixer, 0);
AudioConnection          patchCord16(monMixer, 0, i2sMon, 0);
AudioConnection          patchCord17(monMixer, 0, i2sMon, 1);
AudioConnection          patchCord18(recMixer, 0, usbRec, 0);
AudioControlSGTL5000     sgtl5000; 
// GUItool: end automatically generated code

const int                     audioInput = AUDIO_INPUT_LINEIN;
// const int                     audioInput = AUDIO_INPUT_MIC;
String 												sd_path = "--";
int														vol_ctrl;
float													vol_value;
elapsedMillis									peak_interval_ms;

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
	tmElements_t tm;
	
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
		breakTime(next_record.ts, tm);
		MONPORT.printf("Next record @ %02dh%02dm%02ds\n", tm.Hour, tm.Minute, tm.Second);
	}
	sd_path = createSDpath();
	setRecInfos(&next_record, sd_path);
	unsigned long dur = next_record.dur.Second + 
										(next_record.dur.Minute * SECS_PER_MIN) + 
										(next_record.dur.Hour * SECS_PER_HOUR);
	alarm_rec_id = Alarm.timerOnce(dur, timerRecDone);	
}

/* setRecInfos(struct recInfos*, String)
 * -------------------------------------
 * Set the information related to the pointed recording.
 * IN:	- pointer to a record struct (struct recInfos*)
 *			- root path name (String)
 * OUT:	- none
 */
void setRecInfos(struct recInfo* rec, String root_path) {
	rec->dur.Second = rec_window.length.Second;
	rec->dur.Minute = rec_window.length.Minute;
	rec->dur.Hour = rec_window.length.Hour;
	rec->rpath.remove(0);
	rec->rpath.concat(root_path.c_str());
	rec->rpath.concat(".wav");
	rec->mpath.remove(0);
	rec->mpath.concat(root_path.c_str());
	rec->mpath.concat(".txt");
	rec->t_set = (bool)rec->ts;
	createMetadata(rec);
}

/* startRecording(String)
 * ----------------------
 * Open the file path and start the recording queue.
 * IN:	- file path (String)
 * OUT:	- none
 *
 */
void startRecording(String path) {
  frec = SD.open(path.c_str(), FILE_WRITE);
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
    byte buffer[REC_WRITE_BUF_SIZE];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queueSdc.readBuffer(), REC_READ_BUF_SIZE);
    queueSdc.freeBuffer();
    memcpy(buffer+REC_READ_BUF_SIZE, queueSdc.readBuffer(), REC_READ_BUF_SIZE);
    queueSdc.freeBuffer();
		// elapsedMicros usec = 0;
    frec.write(buffer, REC_WRITE_BUF_SIZE);
    tot_rec_bytes += REC_WRITE_BUF_SIZE;
		// MONPORT.print("SD write, us=");
		// MONPORT.println(usec);
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
      frec.write((byte*)queueSdc.readBuffer(), REC_READ_BUF_SIZE);
      queueSdc.freeBuffer();
      tot_rec_bytes += REC_READ_BUF_SIZE;
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
	sd_path = "--";
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
	rec->rpath.remove(0);
	rec->mpath.remove(0);
	rec->gps_lat = 0;
	rec->gps_long = 0;
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
	sd_path = "--";
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
	recMixer.gain(0, 1);
	// sgtl5000.lineOutLevel(GSTL5000_OUTLEVEL_DEF);
	setHpGain();
}

/* stopMonitoring(void)
 * --------------------
 * To stop monitoring, close the mixer channel
 */
void stopMonitoring(void) {
	monMixer.gain(MIXER_CH_REC, 0);
	recMixer.gain(0, 0);
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
	if(peak_interval_ms > 24) {
		if(peak.available()) {
			peak_interval_ms = 0;
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
	sgtl5000.adcHighPassFilterDisable();
  monMixer.gain(MIXER_CH_REC, 0);
  monMixer.gain(MIXER_CH_SDC, 0);
	recMixer.gain(0, 0);
}
