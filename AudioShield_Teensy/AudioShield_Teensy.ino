/*
  AudioShield firmware for SoundingSoil
  -------------------------------------
  Mixing different code bases:
  - Serial_monitor (ICST)
  - Teensy_recorder (PJRC)
*/
#include "main.h"

// Load drivers
SnoozeDigital 								button_wakeup; 	// Wakeup pins on Teensy 3.6:
																							// 2,4,6,7,9,10,11,13,16,21,22,26,30,33
SnoozeAlarm										alarm_rec;
SnoozeAlarm										alarm_led;
// install driver into SnoozeBlock
SnoozeBlock 									snooze_config(button_wakeup);


volatile struct wState 				working_state;
struct rWindow								rec_window;
struct recInfo								last_record;
struct recInfo								next_record;

bool													ready_to_sleep;

void setup() {
  // Initialize serial ports:
  MONITORPORT.begin(115200);	// Serial monitor port
  BC127PORT.begin(9600);			// BC127 communication port
  GPSPORT.begin(9600);				// GPS port

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
	setDefaultTime();
}

void loop() {
SLEEP:
	int who;
	// you need to update before sleeping.
	but_rec.update();
	but_mon.update();
	but_blue.update();
	// switch off i2s clock before sleeping
	SIM_SCGC6 &= ~SIM_SCGC6_I2S;
	Alarm.delay(10);
	// returns module that woke up processor from hibernation
	who = Snooze.hibernate(snooze_config);
	if(who == WAKESOURCE_RTC) {
		// if alarm wake-up (from 'snooze') -> remove alarm, adjust time and re-start recording
		snooze_config -= alarm_rec;
		adjustTime(TSOURCE_REC);
		working_state.rec_state = RECSTATE_RESTART;
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
	Alarm.delay(10);

WORK:
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
    // Serial.print("Record button pressed: rec_state = "); Serial.println(working_state.rec_state);
    if(working_state.rec_state == RECSTATE_OFF) {
			working_state.rec_state = RECSTATE_REQ_ON;
    }
    else {
			snooze_config -= alarm_rec;
			Alarm.free(alarm_rec_id);
			working_state.rec_state = RECSTATE_REQ_OFF;
    }
		button_call = (enum bCalls)BCALL_NONE;
  }
  if(button_call == BUTTON_MONITOR_PIN) {
		if(working_state.mon_state == MONSTATE_OFF) {
			working_state.mon_state = MONSTATE_REQ_ON;
		}
		else {
			working_state.mon_state = MONSTATE_REQ_OFF;
		}
		button_call = (enum bCalls)BCALL_NONE;
	}
  if(button_call == BUTTON_BLUETOOTH_PIN) {
		if(working_state.ble_state == BLESTATE_OFF) {
			working_state.ble_state = BLESTATE_REQ_ADV;
		}
		else {
			working_state.ble_state = BLESTATE_REQ_OFF;
		}
		button_call = (enum bCalls)BCALL_NONE;
  }

  // REC state actions
	switch(working_state.rec_state) {
		case RECSTATE_REQ_ON: {
			next_record.cnt = 0;
			prepareRecording(true);
			working_state.rec_state = RECSTATE_ON;
			startRecording(next_record.path);
			sendCmdOut(BCNOT_REC_STATE);
			sendCmdOut(BCNOT_FILEPATH);
			break;
		}
			
		case RECSTATE_RESTART: {
			prepareRecording(false);
			working_state.rec_state = RECSTATE_ON;
			startRecording(next_record.path);
			sendCmdOut(BCNOT_REC_STATE);
			break;
		}
		
		case RECSTATE_REQ_WAIT: {
			stopRecording(next_record.path);
			pauseRecording();
			break;
		}
		
		case RECSTATE_ON: {
			continueRecording();
			detectPeaks();
			break;
		}
		
		case RECSTATE_REQ_OFF: {
			stopRecording(next_record.path);
			finishRecording();
			stopLED(&leds[LED_PEAK]);
			working_state.rec_state = RECSTATE_OFF;
			sendCmdOut(BCNOT_REC_STATE);
			break;
		}
		
		default:
			break;
	}
  // MON state actions
	switch(working_state.mon_state) {
		case MONSTATE_REQ_ON: {
			startLED(&leds[LED_MONITOR], LED_MODE_ON);
			startMonitoring();
			working_state.mon_state = MONSTATE_ON;
			sendCmdOut(BCNOT_MON_STATE);
			sendCmdOut(BCCMD_VOL_UP);
			break;
		}
		
		case MONSTATE_ON: {
			setHpGain();
			detectPeaks();
			break;
		}
		
		case MONSTATE_REQ_OFF: {
			stopMonitoring();
			stopLED(&leds[LED_MONITOR]);
			stopLED(&leds[LED_PEAK]);
			working_state.mon_state = MONSTATE_OFF;
			sendCmdOut(BCNOT_MON_STATE);
			break;
		}
		
		default:
			break;
	}
  // BLE state actions
	switch(working_state.ble_state) {
		case BLESTATE_REQ_ADV: {
			startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			bc127PowerOn();
			Alarm.delay(1000);
			bc127AdvStart();
			working_state.ble_state = BLESTATE_ADV;
			break;
		}
		
		case BLESTATE_REQ_CONN: {
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			}
			else {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			}
			working_state.ble_state = BLESTATE_CONNECTED;
			break;
		}
		
		case BLESTATE_CONNECTED: {
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			}
			break;
		}
		
		case BLESTATE_REQ_DIS: {
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			}
			else {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			}
			sendCmdOut(BCCMD_ADV_ON);
			working_state.ble_state = BLESTATE_ADV;
			break;
		}
		
		case BLESTATE_REQ_OFF: {
			bc127AdvStop();
			Alarm.delay(100);
			bc127PowerOff();
			stopLED(&leds[LED_BLUETOOTH]);
			working_state.ble_state = BLESTATE_OFF;
			break;
		}
		
		default:
		break;
	}
  // BT state actions
	switch(working_state.bt_state) {
		case BTSTATE_REQ_CONN: {
			startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			working_state.bt_state = BTSTATE_CONNECTED;
			sendCmdOut(BCNOT_BT_STATE);
			break;
		}
		
		case BTSTATE_REQ_DIS: {
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			}
			else {
				bc127PowerOff();
				stopLED(&leds[LED_BLUETOOTH]);
				// working_state.ble_state = BLESTATE_OFF;
			}
			working_state.bt_state = BTSTATE_OFF;
			sendCmdOut(BCNOT_BT_STATE);
			break;
		}
		
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

	// Stay awake or go to sleep?...
	// ...first check all other working states
	if( (working_state.mon_state == MONSTATE_OFF) &&
			(working_state.ble_state == BLESTATE_OFF) &&
			(working_state.bt_state == BTSTATE_OFF) ) {
		if(working_state.rec_state == RECSTATE_WAIT) working_state.rec_state = RECSTATE_REQ_WAIT;
		ready_to_sleep = true;
	}
	else {
		ready_to_sleep = false;
	}
	
	// ...then decide which alarm to set (SLEEP -> snooze, WORK -> timeAlarms)
	time_t delta;
	tmElements_t tm1, tm2, tm3;
	switch(working_state.rec_state) {
		case RECSTATE_REQ_WAIT: {
			delta = next_record.ts - now();
			breakTime(now(), tm1);
			breakTime(next_record.ts, tm2);
			breakTime(delta, tm3);
			Serial.printf("Current time: %02d.%02d.%02d, %02dh%02dm%02ds\n", 
										tm1.Day, tm1.Month, (tm1.Year-30), tm1.Hour, tm1.Minute, tm1.Second);
			Serial.printf("Delta: %02dh%02dm%02ds\n", 
										tm3.Hour, tm3.Minute, tm3.Second);
			if(ready_to_sleep) {
				Serial.println("Setting up Snooze alarm");
				snooze_config += alarm_rec;
				alarm_rec.setRtcTimer(tm3.Hour, tm3.Minute, tm3.Second);
				working_state.rec_state = RECSTATE_IDLE;
				sendCmdOut(BCNOT_REC_STATE);
				delay(100);
				goto SLEEP;
			}
			else {
				alarm_wait_id = Alarm.alarmOnce(tm2.Hour, tm2.Minute, tm2.Second, alarmNextRec);
				working_state.rec_state = RECSTATE_WAIT;
				sendCmdOut(BCNOT_REC_STATE);
				goto WORK;
			}
			break;
		}
			
		case RECSTATE_OFF:
			if(ready_to_sleep) goto SLEEP;
			else goto WORK;
			break;
		
		default:
			goto WORK;
			break;
	}
}
	

/* setDefaultValues(void)
 * ----------------------
 */
void setDefaultValues(void) {
  working_state.rec_state = RECSTATE_OFF;
  working_state.mon_state = MONSTATE_OFF;
  working_state.bt_state = BTSTATE_OFF;
  working_state.ble_state = BLESTATE_OFF;
	rec_window.length.Second = RWIN_LEN_DEF_SEC;
	rec_window.length.Minute = RWIN_LEN_DEF_MIN;
	rec_window.length.Hour = RWIN_LEN_DEF_HOUR;
	rec_window.length.Day = RWIN_LEN_DEF_DAY;
	rec_window.length.Month = RWIN_LEN_DEF_MON;
	rec_window.length.Year = RWIN_LEN_DEF_YEAR;
	rec_window.period.Second = RWIN_PER_DEF_SEC;
	rec_window.period.Minute = RWIN_PER_DEF_MIN;
	rec_window.period.Hour = RWIN_PER_DEF_HOUR;
	rec_window.period.Day = RWIN_PER_DEF_DAY;
	rec_window.period.Month = RWIN_PER_DEF_MON;
	rec_window.period.Year = RWIN_PER_DEF_YEAR;
	rec_window.occurences = RWIN_OCC_DEF;
	last_record.cnt = 0;
	next_record = last_record;
}



