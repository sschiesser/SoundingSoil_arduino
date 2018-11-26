/*
  AudioShield firmware for SoundingSoil
  -------------------------------------
  Mixing different code bases:
  - Serial_monitor (ICST)
  - Teensy_recorder (PJRC)
*/
#include "main.h"

// Load drivers
SnoozeDigital 								button_wakeup;
// Available digital pins for wakeup on Teensy 3.6: 2,4,6,7,9,10,11,13,16,21,22,26,30,33
SnoozeAlarm										alarm_rec;
SnoozeAlarm										alarm_led;

// install driver into SnoozeBlock
SnoozeBlock 									snooze_config(button_wakeup);

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

volatile struct wState 				working_state;
enum bCalls										button_call;
struct rWindow								rec_window;
struct lastRec								last_record;
void setup() {
  // Initialize serial ports:
  Serial.begin(115200);				// Serial monitor port
  Serial4.begin(9600);				// BC127 communication port
  gpsPort.begin(9600);				// GPS port

	// Say hello
	delay(100);
  Serial.println("AudioShield v1.0");
  Serial.println("----------------");
	delay(20);
	// Initialize peripheral & variables
	initLEDButtons();
	button_wakeup.pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP, FALLING);
	button_wakeup.pinMode(BUTTON_MONITOR_PIN, INPUT_PULLUP, FALLING);
  button_wakeup.pinMode(BUTTON_BLUETOOTH_PIN, INPUT_PULLUP, FALLING);
	
	pinMode(GPS_SWITCH_PIN, OUTPUT);
	digitalWrite(GPS_SWITCH_PIN, LOW);
	pinMode(AUDIO_VOLUME_PIN, INPUT);

	initAudio();
	initSDcard();
	initWaveHeader();
	setDefaultValues();
	bc127Init();
}

void loop() {
	int who;
SLEEP:
	// you need to update before sleeping.
	but_rec.update();
	but_mon.update();
	but_blue.update();
	// switch off i2s clock before sleeping
	SIM_SCGC6 &= ~SIM_SCGC6_I2S;
	delay(10);
	// returns module that woke up processor from hibernation
	who = Snooze.hibernate(snooze_config);
	if(who == WAKESOURCE_RTC) {
		// if alarm wake-up (from 'snooze') -> remove alarm and start recording
		snooze_config -= alarm_rec;
		// if(working_state.rec_state == RECSTATE_WAIT) working_state.rec_state = RECSTATE_REQ_ON;
	}
	else {
		// if button wake-up -> debounce first and notify which button was pressed
		Bounce cur_bt = Bounce(who, BUTTON_BOUNCE_TIME_MS);
		elapsedMillis timeout = 0;
		while (timeout < (BUTTON_BOUNCE_TIME_MS+1)) cur_bt.update();
		button_call = (enum bCalls)who;
	}
	// if not sleeping anymore, re-enable i2s clock
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	delay(10);

WORK:
	bool ret;
	float gain;
	// needed for TimeAlarms timers
	Alarm.delay(0);
	// needed for button bounces	
	but_rec.update();
	but_mon.update();
	but_blue.update();
	// if button falling edge detected (awake state) -> notify which button was pressed
	if(but_rec.fallingEdge()) button_call = (enum bCalls)BUTTON_RECORD_PIN;
	if(but_mon.fallingEdge()) button_call = (enum bCalls)BUTTON_MONITOR_PIN;
	if(but_blue.fallingEdge()) button_call = (enum bCalls)BUTTON_BLUETOOTH_PIN;
    
  // centralized button call actions coming from SLEEP or WORK mode
  if(button_call == BUTTON_RECORD_PIN) {
    Serial.print("Record button pressed: rec_state = "); Serial.println(working_state.rec_state);
    if(working_state.rec_state == RECSTATE_OFF) {
			working_state.rec_state = RECSTATE_REQ_ON;
    }
    else {
			working_state.rec_state = RECSTATE_REQ_OFF;
    }
		button_call = (enum bCalls)BCALL_NONE;
  }
  if(button_call == BUTTON_MONITOR_PIN) {
    Serial.print("Monitor button pressed: mon_state = "); Serial.println(working_state.mon_state);
		if(working_state.mon_state == MONSTATE_OFF) {
			working_state.mon_state = MONSTATE_REQ_ON;
		}
		else {
			working_state.mon_state = MONSTATE_REQ_OFF;
		}
		button_call = (enum bCalls)BCALL_NONE;
	}
  if(button_call == BUTTON_BLUETOOTH_PIN) {
    Serial.print("Bluetooth button pressed: BLE = "); Serial.print(working_state.ble_state);
		Serial.print(", BT = "); Serial.println(working_state.bt_state);
		if(working_state.ble_state == BLESTATE_IDLE) {
			working_state.ble_state = BLESTATE_REQ_ADV;
		}
		else {
			working_state.ble_state = BLESTATE_REQ_OFF;
		}
		button_call = (enum bCalls)BCALL_NONE;
  }

  // REC state actions
	switch(working_state.rec_state) {
		case RECSTATE_REQ_ON:
		case RECSTATE_WAIT: {
			startLED(&leds[LED_RECORD], LED_MODE_WAITING);
			gpsPowerOn();
			delay(1000);
			ret = gpsGetData();
			gpsPowerOff();
			// GPS data did not returned a valid fix
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
			setRecInfos(rec_path, last_record.cnt);
			working_state.rec_state = RECSTATE_ON;
			startRecording(rec_path);
			break;
		}
		
		case RECSTATE_REQ_WAIT: {
			stopRecording(rec_path);
			stopLED(&leds[LED_RECORD]);
			working_state.rec_state = RECSTATE_WAIT;
			break;
		}
		
		case RECSTATE_ON: {
			continueRecording();
			break;
		}
		
		case RECSTATE_REQ_OFF: {
			stopRecording(rec_path);
			stopLED(&leds[LED_RECORD]);
			working_state.rec_state = RECSTATE_OFF;
			break;
		}
		
		default:
			break;
	}
  // MON state actions
	switch(working_state.mon_state) {
		case MONSTATE_REQ_ON:
			startLED(&leds[LED_MONITOR], LED_MODE_ON);
			startMonitoring();
			working_state.mon_state = MONSTATE_ON;
			break;
		
		case MONSTATE_ON:
			vol_ctrl = analogRead(AUDIO_VOLUME_PIN);
			gain = (float)vol_ctrl / 1023.0;
			mixer.gain(MIXER_CH_REC, gain);
			break;
		
		case MONSTATE_REQ_OFF:
			stopMonitoring();
			stopLED(&leds[LED_MONITOR]);
			working_state.mon_state = MONSTATE_OFF;
			break;
		
		default:
			break;
	}
  // BLE state actions
	switch(working_state.ble_state) {
		case BLESTATE_REQ_ADV:
			startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			bc127PowerOn();
			delay(1000);
			bc127AdvStart();
			working_state.ble_state = BLESTATE_ADV;
			break;
		
		case BLESTATE_REQ_CONN:
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			}
			else {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			}
			working_state.ble_state = BLESTATE_CONNECTED;
			break;
		
		case BLESTATE_CONNECTED:
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			}
			break;
		
		case BLESTATE_REQ_DIS:
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			}
			else {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			}
			sendCmdOut(BCCMD_BLE_ADV_ON);
			working_state.ble_state = BLESTATE_ADV;
			break;
		
		case BLESTATE_REQ_OFF:
			bc127AdvStop();
			delay(200);
			bc127PowerOff();
			stopLED(&leds[LED_BLUETOOTH]);
			working_state.ble_state = BLESTATE_IDLE;
			break;
		
		default:
		break;
	}
  // BT state actions
	switch(working_state.bt_state) {
		case BTSTATE_REQ_CONN:
			startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			working_state.bt_state = BTSTATE_CONNECTED;
			break;
		
		case BTSTATE_REQ_DIS:
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			}
			else {
				bc127PowerOff();
				stopLED(&leds[LED_BLUETOOTH]);
				working_state.ble_state = BLESTATE_IDLE;
			}
			working_state.bt_state = BTSTATE_IDLE;
			break;
		
		default:
			break;
	}
	
	// Serial messaging
  if (Serial4.available()) {
    String inMsg = Serial4.readStringUntil('\r');
    int outMsg = parseSerialIn(inMsg);
    if(!sendCmdOut(outMsg)) {
      Serial.println("Sending command error!!");
    }
  }	
  if (Serial.available()) {
    String manInput = Serial.readStringUntil('\n');
    int len = manInput.length() - 1;
    Serial4.print(manInput.substring(0, len)+'\r');
  }

	// Stay awake or go to sleep?
	if( (working_state.mon_state == MONSTATE_OFF) &&
			(working_state.ble_state == BLESTATE_IDLE) &&
			(working_state.bt_state == BTSTATE_IDLE) ) {
		if( (working_state.rec_state == RECSTATE_OFF) ||
			(working_state.rec_state == RECSTATE_WAIT) ) {
			snooze_config += alarm_rec;
			alarm_rec.setRtcTimer(0,0,5);
			goto SLEEP;
		}
		else {
			goto WORK;
		}
	}
	else {
		goto WORK;
	}
}

void setRecInfos(String path, unsigned int rec_cnt) {
	unsigned long dur = rec_window.length.Second + 
										(rec_window.length.Minute * SECS_PER_MIN) + 
										(rec_window.length.Hour * SECS_PER_HOUR);
	last_record.dur.Second = rec_window.length.Second;
	last_record.dur.Minute = rec_window.length.Minute;
	last_record.dur.Hour = rec_window.length.Hour;
	last_record.path.remove(0);
	last_record.path.concat(path.c_str());
	last_record.cnt = rec_cnt;
	if(timeStatus() == timeNotSet) {
		last_record.t_set = false;
		last_record.ts = (time_t)0;
	}
	else {
		last_record.t_set = true;
		last_record.ts = now();
	}
	Serial.printf("Record information:\n-duration: %02dh%02dm%02ds\n-path: '%s'\n-time set: %d\n-rec time: %ld\n", last_record.dur.Hour, last_record.dur.Minute, last_record.dur.Second, last_record.path.c_str(), last_record.t_set, last_record.ts);
	Alarm.timerOnce(dur, alarmRecDone);	
}

/* alarmRecDone(void)
 * ------------------
 * Callback of a TimeAlarms timer triggered when a record window is finished.
 * IN:	- none
 * OUT:	- none
 */
void alarmRecDone(void) {
	if((rec_window.occurences == 0) || (last_record.cnt < rec_window.occurences)) {
		working_state.rec_state = RECSTATE_REQ_WAIT;
	}
	else {
		working_state.rec_state = RECSTATE_REQ_OFF;
	}
}

/* adjustTime(enum tSources)
 * -------------------------
 * Adjust local time after an external source
 * (GPS or app over BLE) has provided a new value.
 * IN:	- external source (enum tSources)
 * OUT:	- none
 */
void adjustTime(enum tSources source) {
	int year;
	byte month, day, hour, minute, second;
	unsigned long fix_age;
	
	switch(source) {
		case TSOURCE_GPS:
			gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &fix_age);
			setTime(hour, minute, second, day, month, year);
			adjustTime(TIME_OFFSET * SECS_PER_HOUR);
			Teensy3Clock.set(now());
			break;
			
		case TSOURCE_BLE:
			setTime(ble_time);
			Teensy3Clock.set(ble_time);
			break;
			
		default:
			break;
	}
	Serial.printf("Time adjusted from source#%d. Current time: %ld\n", source, now());
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

/* setDefaultValues(void)
 * ----------------------
 */
void setDefaultValues(void) {
  working_state.rec_state = RECSTATE_OFF;
  working_state.mon_state = MONSTATE_OFF;
  working_state.bt_state = BTSTATE_IDLE;
  working_state.ble_state = BLESTATE_IDLE;
	rec_window.length.Second = RWIN_LEN_DEF_S;
	rec_window.length.Minute = RWIN_LEN_DEF_M;
	rec_window.length.Hour = RWIN_LEN_DEF_H;
	rec_window.period.Second = RWIN_PER_DEF_S;
	rec_window.period.Minute = RWIN_PER_DEF_M;
	rec_window.period.Hour = RWIN_PER_DEF_H;
	rec_window.occurences = RWIN_OCC_DEF;
	last_record.cnt = 0;
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