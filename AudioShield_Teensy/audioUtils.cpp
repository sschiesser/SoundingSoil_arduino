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
AudioInputI2S i2sRec;      // xy=76,36
AudioPlaySdWav playWav;    // xy=78,114
AudioMixer4 monMixer;      // xy=260,114
AudioAnalyzePeak peak;     // xy=445,65
AudioOutputI2S i2sMon;     // xy=447,118
AudioRecordQueue queueSdc; // xy=456,30
AudioConnection patchCord1(i2sRec, 0, monMixer, 0);
AudioConnection patchCord2(i2sRec, 0, peak, 0);
AudioConnection patchCord3(i2sRec, 0, queueSdc, 0);
AudioConnection patchCord4(playWav, 0, monMixer, 1);
AudioConnection patchCord5(monMixer, 0, i2sMon, 0);
AudioConnection patchCord6(monMixer, 0, i2sMon, 1);
AudioControlSGTL5000 sgtl5000; // xy=251,186
// GUItool: end automatically generated code

const int audioInput = AUDIO_INPUT_LINEIN;
String rec_path = "--";
int vol_ctrl;
float vol_value;
elapsedMillis peak_interval;
elapsedMillis hpgain_interval;

/* prepareRecording(bool)
 * ----------------------
 * Fetch a GPS fix (if demanded), set the record timestamp to now(),
 * create the file path according to it, save all record information
 * (including GPS fix) and start the record timer.
 * IN:	- sync with GPS (bool)
 * OUT:	- none
 */
void prepareRecording(bool sync) {
  bool gps_fix = true;
  tmElements_t tm;

  startLED(&leds[LED_RECORD], LED_MODE_ON);
  if (sync) {
    // gpsPowerOn();
    // gpsEnable(true);
    startLED(&leds[LED_RECORD], LED_MODE_WAITING);
    gps_fix = gpsGetData();
    // gpsPowerOff();
    // gpsEnable(false);
    if (!gps_fix) {
      if (next_record.gps_source == GPS_NONE) {
        startLED(&leds[LED_RECORD], LED_MODE_WARNING_LONG);
      } else {
        startLED(&leds[LED_RECORD], LED_MODE_ON);
      }
    } else {
      if (next_record.gps_source == GPS_NONE) {
        setCurTime(0, TSOURCE_GPS);
      }
      startLED(&leds[LED_RECORD], LED_MODE_ON);
    }
  }
  next_record.tss = now();
  breakTime(next_record.tss, tm);
  rec_path = createSDpath();
  setRecInfos(&next_record, rec_path);
  unsigned long dur = next_record.dur.Second +
                      (next_record.dur.Minute * SECS_PER_MIN) +
                      (next_record.dur.Hour * SECS_PER_HOUR);
  dur = (unsigned long)((float)dur * REC_DUR_CORRECTION_RATIO);
  if (debug)
    snooze_usb.printf("Audio:   Set recording duration to %d\n", dur);
  if (debug)
    snooze_usb.printf("Audio:   Preparing recording. Time source: %d, current "
                      "time: %02dh%02dm%02ds, GPS source: %d\n",
                      time_source, tm.Hour, tm.Minute, tm.Second, next_record.gps_source);
  if (dur != 0) {
    alarm_rec_id = Alarm.timerOnce(dur, timerRecDone);
  }
}

/* setRecInfos(struct recInfos*, String)
 * -------------------------------------
 * Set the information related to the pointed recording.
 * IN:	- pointer to a record struct (struct recInfos*)
 *			- recording path name (String)
 * OUT:	- none
 */
void setRecInfos(struct recInfo *rec, String path) {
  rec->dur.Second = rec_window.duration.Second;
  rec->dur.Minute = rec_window.duration.Minute;
  rec->dur.Hour = rec_window.duration.Hour;
  rec->per.Second = rec_window.period.Second;
  rec->per.Minute = rec_window.period.Minute;
  rec->per.Hour = rec_window.period.Hour;
  rec->rpath.remove(0);
  rec->rpath.concat(path.c_str());
  rec->mpath = rec->rpath;
  rec->mpath.replace(".wav", ".txt");
  rec->t_set = (bool)rec->tss;
  rec->rec_tot = rec_window.occurences;
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
  if (frec) {
    queueSdc.begin();
    tot_rec_bytes = 0;
  } else {
    if (debug)
      snooze_usb.println("Audio:   file opening error");
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
  if (queueSdc.available() >= 2) {
    byte buffer[REC_WRITE_BUF_SIZE];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queueSdc.readBuffer(), REC_READ_BUF_SIZE);
    queueSdc.freeBuffer();
    memcpy(buffer + REC_READ_BUF_SIZE, queueSdc.readBuffer(),
           REC_READ_BUF_SIZE);
    queueSdc.freeBuffer();
    // elapsedMicros usec = 0;
    frec.write(buffer, REC_WRITE_BUF_SIZE);
    tot_rec_bytes += REC_WRITE_BUF_SIZE;
    // if(debug) snooze_usb.print("Audio:   SD write, us=");
    // if(debug) snooze_usb.println(usec);
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
  queueSdc.end();
  if (working_state.rec_state) {
    while (queueSdc.available() > 0) {
      frec.write((byte *)queueSdc.readBuffer(), REC_READ_BUF_SIZE);
      queueSdc.freeBuffer();
      tot_rec_bytes += REC_READ_BUF_SIZE;
    }
    frec.close();

    writeWaveHeader(path, tot_rec_bytes);
  }
  if (debug)
    snooze_usb.println("Audio:   Recording stopped, writing metadata");

  createMetadata(&next_record);
}

/* pauseRecording(void)
 * --------------------
 * Prepare the next record informations.
 * IN:	- none
 * OUT:	- none
 */
void pauseRecording(void) {
  tmElements_t tm;
  breakTime(now(), tm);

  last_record = next_record;
  next_record.tss =
      last_record.tss +
      ((rec_window.period.Hour * SECS_PER_HOUR) +
       (rec_window.period.Minute * SECS_PER_MIN) + rec_window.period.Second);
  rec_path = "--";
  next_record.cnt++;
  if (debug)
    snooze_usb.printf("Audio:   Pausing recording. Time source: %d, current "
                      "time: %02dh%02dm%02ds, GPS source: %d\n",
                      time_source, tm.Hour, tm.Minute, tm.Second, next_record.gps_source);
  stopLED(&leds[LED_RECORD]);
}

/* resetRecInfo(struct recInfo*)
 * -----------------------------
 * Set all values of the pointed record to 0
 * IN:	- pointer to the record (struct recInfo*)
 * OUT:	- none
 */
void resetRecInfo(struct recInfo *rec) {
  rec->tss = 0;
  rec->tsp = 0;
  rec->dur.Hour = 0;
  rec->dur.Minute = 0;
  rec->dur.Second = 0;
  rec->per.Hour = 0;
  rec->per.Minute = 0;
  rec->per.Second = 0;
  rec->t_set = false;
  rec->rpath.remove(0);
  rec->mpath.remove(0);
  rec->gps_lat = 1000.0;
  rec->gps_long = 1000.0;
  rec->gps_source = GPS_NONE;
  rec->cnt = 0;
  rec->rec_tot = 0;
  rec->man_stop = false;
}

/* finishRecording(void)
 * ---------------------
 * Reset the record informations, time source
 * and switch-off the REC LED.
 * IN:	- none
 * OUT:	- none
 */
void finishRecording(void) {
  tmElements_t tm;
  resetRecInfo(&last_record);
  resetRecInfo(&next_record);
  breakTime(now(), tm);
  rec_path = "--";
  if (debug)
    snooze_usb.printf("Audio:   Finishing recording. Time source: %d, current "
                      "time: %02dh%02dm%02ds, GPS source: %d\n",
                      time_source, tm.Hour, tm.Minute, tm.Second, next_record.gps_source);

  if ((time_source == TSOURCE_GPS) || (time_source == TSOURCE_PHONE))
    time_source = TSOURCE_TEENSY;

  if ((next_record.gps_source == GPS_RECORDER) || (next_record.gps_source == GPS_PHONE))
    next_record.gps_source = GPS_NONE;

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
  hpgain_interval = 0;
  peak_interval = 0;
  setHpGain();
}

/* stopMonitoring(void)
 * --------------------
 * To stop monitoring, close the mixer channel
 */
void stopMonitoring(void) { monMixer.gain(MIXER_CH_REC, 0); }

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
  // if(debug) snooze_usb.printf("Audio:   gain = %1.2f\n", gain);
  sgtl5000.volume(gain);
}

/* detectPeaks(void)
 * -----------------
 * Detect possible peaks at line input and
 * notify them with the peak LED.
 * IN:	- none
 * OUT:	- none
 */
void detectPeaks(void) {
  if (peak.available()) {
    if (peak.read() >= 1.0) {
      startLED(&leds[LED_PEAK], LED_MODE_ON);
    } else {
      stopLED(&leds[LED_PEAK]);
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
  // Set pin mode for the audio volume knob
  pinMode(AUDIO_VOLUME_PIN, INPUT);

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
}
