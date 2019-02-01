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
SnoozeAlarm										snooze_rec;
SnoozeAlarm										snooze_led;
// install driver into SnoozeBlock
SnoozeBlock 									snooze_config(button_wakeup);


volatile struct wState 				working_state;
struct rWindow								rec_window;
struct recInfo								last_record;
struct recInfo								next_record;

bool													ready_to_sleep;

void setup() {
  // Initialize serial ports:
  MONPORT.begin(115200);			// Serial monitor port
  BLUEPORT.begin(9600);				// BC127 communication port
  GPSPORT.begin(9600);				// GPS port

	initLEDButtons();
	button_wakeup.pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP, FALLING);
	button_wakeup.pinMode(BUTTON_MONITOR_PIN, INPUT_PULLUP, FALLING);
  button_wakeup.pinMode(BUTTON_BLUETOOTH_PIN, INPUT_PULLUP, FALLING);
	
	pinMode(GPS_SWITCH_PIN, OUTPUT);
	digitalWrite(GPS_SWITCH_PIN, LOW);
	pinMode(AUDIO_VOLUME_PIN, INPUT);

	Alarm.delay(500);
	initAudio();
	initSDcard();
	initWaveHeader();
	setDefaultValues();
	setTimeSource();
	initBc127();
	Alarm.delay(500);
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
	Alarm.delay(100);
	// returns module that woke up processor from hibernation
	who = Snooze.hibernate(snooze_config);
	if(who == WAKESOURCE_RTC) {
		// if alarm wake-up (from 'snooze') -> remove alarm, adjust time and re-start recording
		snooze_config -= snooze_rec;
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
	Alarm.delay(100);

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
    MONPORT.print("Record button pressed: rec_state = "); MONPORT.println(working_state.rec_state);
    if(working_state.rec_state == RECSTATE_OFF) {
			working_state.rec_state = RECSTATE_REQ_ON;
    }
    else {
			snooze_config -= snooze_rec;
			// Alarm.free(alarm_wait_id);
			// Alarm.free(alarm_rec_id);
			working_state.rec_state = RECSTATE_REQ_OFF;
    }
		button_call = (enum bCalls)BCALL_NONE;
  }
  if(button_call == BUTTON_MONITOR_PIN) {
    MONPORT.print("Monitor button pressed: mon_state = "); MONPORT.println(working_state.mon_state);
		if(working_state.mon_state == MONSTATE_OFF) {
			working_state.mon_state = MONSTATE_REQ_ON;
		}
		else {
			working_state.mon_state = MONSTATE_REQ_OFF;
		}
		button_call = (enum bCalls)BCALL_NONE;
	}
  if(button_call == BUTTON_BLUETOOTH_PIN) {
    MONPORT.print("Bluetooth button pressed: ble_state = "); MONPORT.println(working_state.ble_state);
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
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_REC_STATE);
				sendCmdOut(BCNOT_FILEPATH);
			}
			break;
		}
			
		case RECSTATE_RESTART: {
			prepareRecording(false);
			working_state.rec_state = RECSTATE_ON;
			startRecording(next_record.path);
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_REC_STATE);
				sendCmdOut(BCNOT_FILEPATH);
			}
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
			Alarm.free(alarm_wait_id);
			Alarm.free(alarm_rec_id);

			stopRecording(next_record.path);
			finishRecording();
			stopLED(&leds[LED_PEAK]);
			working_state.rec_state = RECSTATE_OFF;
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_REC_STATE);
			}
			break;
		}
		
		default:
			break;
	}
  // MON state actions
	switch(working_state.mon_state) {
		case MONSTATE_REQ_ON: {
			MONPORT.printf("MON req... states: BT %d, BLE %d, REC %d, MON %d\n",
				working_state.bt_state, working_state.ble_state,
				working_state.rec_state, working_state.mon_state);
			startLED(&leds[LED_MONITOR], LED_MODE_ON);
			startMonitoring();
			working_state.mon_state = MONSTATE_ON;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				sendCmdOut(BCCMD_MON_START);
				Alarm.delay(50);
				sendCmdOut(BCCMD_VOL_A2DP);
				Alarm.delay(50);
			}
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_MON_STATE);
				Alarm.delay(50);
			}
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
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				sendCmdOut(BCCMD_MON_STOP);
				Alarm.delay(50);
			}
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_MON_STATE);
				Alarm.delay(50);
			}
			working_state.mon_state = MONSTATE_OFF;
			break;
		}
		
		default:
			break;
	}
  // BLE state actions
	switch(working_state.ble_state) {
		case BLESTATE_REQ_ADV: {
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			}
			else {
				// bc127Reset();
				// delay(500);
				bc127BlueOn();
				delay(500);
				startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
				alarm_adv_id = Alarm.timerOnce(BLEADV_TIMEOUT_S, alarmAdvTimeout);
			}
			bc127AdvStart();
			working_state.ble_state = BLESTATE_ADV;
			break;
		}
		
		case BLESTATE_REQ_CONN: {
			Alarm.free(alarm_adv_id);
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
			BLE_conn_id = 0;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				working_state.ble_state = BLESTATE_REQ_ADV;
			}
			else {
				working_state.ble_state = BLESTATE_REQ_OFF;
			}
			break;
		}
		
		case BLESTATE_REQ_OFF: {
			Alarm.free(alarm_adv_id);
			bc127AdvStop();
			Alarm.delay(100);
			if(working_state.bt_state != BTSTATE_OFF) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			}
			else {
				bc127BlueOff();
				stopLED(&leds[LED_BLUETOOTH]);
			}
			working_state.ble_state = BLESTATE_OFF;
			break;
		}
		
		default:
		break;
	}
  // BT state actions
	switch(working_state.bt_state) {
		case BTSTATE_REQ_CONN: {
			Alarm.free(alarm_adv_id);
			startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			working_state.bt_state = BTSTATE_CONNECTED;
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				sendCmdOut(BCNOT_BT_STATE);
			}
			break;
		}
		
		case BTSTATE_REQ_DIS: {
			if(working_state.ble_state == BLESTATE_CONNECTED) {
				startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
				sendCmdOut(BCNOT_BT_STATE);
			}
			else {
				bc127BlueOff();
				stopLED(&leds[LED_BLUETOOTH]);
				working_state.ble_state = BLESTATE_OFF;
			}
			working_state.bt_state = BTSTATE_OFF;
			BT_conn_id = 0;
			BT_peer_address = "";
			BT_peer_name = "auto";
			
			break;
		}
		
		default:
			break;
	}
	
	// Serial messaging
  if (BLUEPORT.available()) {
    String inMsg = BLUEPORT.readStringUntil('\r');
    int outMsg = parseSerialIn(inMsg);
    if(!sendCmdOut(outMsg)) {
      MONPORT.println("Sending command error!!");
    }
  }	
  if (MONPORT.available()) {
    String manInput = MONPORT.readStringUntil('\n');
    int len = manInput.length() - 1;
    BLUEPORT.print(manInput.substring(0, len)+'\r');
		MONPORT.printf("Sent to BLUEPORT: %s\n", manInput.c_str());
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
			MONPORT.printf("Current time: %02d.%02d.%02d, %02dh%02dm%02ds\n", 
										tm1.Day, tm1.Month, (tm1.Year-30), tm1.Hour, tm1.Minute, tm1.Second);
			MONPORT.printf("Delta: %02dh%02dm%02ds\n", 
										tm3.Hour, tm3.Minute, tm3.Second);
			if(ready_to_sleep) {
				MONPORT.println("Setting up Snooze alarm");
				snooze_config += snooze_rec;
				snooze_rec.setRtcTimer(tm3.Hour, tm3.Minute, tm3.Second);
				working_state.rec_state = RECSTATE_IDLE;
				if(working_state.ble_state == BLESTATE_CONNECTED) {
					sendCmdOut(BCNOT_REC_STATE);
					delay(100);
				}
				goto SLEEP;
			}
			else {
				alarm_wait_id = Alarm.alarmOnce(tm2.Hour, tm2.Minute, tm2.Second, alarmNextRec);
				working_state.rec_state = RECSTATE_WAIT;
				if(working_state.ble_state == BLESTATE_CONNECTED) {
					sendCmdOut(BCNOT_REC_STATE);
				}
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
	BT_conn_id = 0;
	BLE_conn_id = 0;
	BT_peer_address = "";
	BT_peer_name = "auto";
}



