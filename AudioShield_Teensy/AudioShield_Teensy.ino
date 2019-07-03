/*
AudioShield firmware for SoundingSoil
-------------------------------------
Mixing different code bases:
- Serial_monitor (ICST)
- Teensy_recorder (PJRC)
*/
#include "main.h"

#define ALWAYS_ON_MODE							0

// install driver into SnoozeBlock
// SnoozeBlock 									snooze_config(button_wakeup);

bool                                            debug = true;

volatile struct wState 							working_state;
struct rWindow									rec_window;
struct recInfo									last_record;
struct recInfo									next_record;

volatile bool									ready_to_sleep;

enum gpsSource                                  gps_source;

void setup() {
    // Initialize serial ports:
    if(debug) MONPORT.begin(115200);						// Serial monitor port
    BLUEPORT.begin(9600);						// BC127 communication port
    GPSPORT.begin(9600);						// GPS port

    initLEDButtons();
    button_wakeup.pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP, FALLING);
    button_wakeup.pinMode(BUTTON_MONITOR_PIN, INPUT_PULLUP, FALLING);
    button_wakeup.pinMode(BUTTON_BLUETOOTH_PIN, INPUT_PULLUP, FALLING);

    pinMode(AUDIO_VOLUME_PIN, INPUT);

    initAudio();
    initSDcard();
    initGps();
    initWaveHeader();
    initBc127();
    time_t t = setTimeSource();
    tmElements_t tm;
    breakTime(t, tm);

    setDefaultValues();

    helloWorld();
    if(debug) MONPORT.printf("Info:    SoundingSoil firmware version 1.0 build %02d%02d%02d%02d%02d\n", (tm.Year - 30), tm.Month, tm.Day, tm.Hour, tm.Minute);

    working_state.ble_state = BLESTATE_REQ_ADV;
}

void loop() {
#if(ALWAYS_ON_MODE==1)
    goto WORK;
#else
    goto WORK;
SLEEP:
    int who;
    // you need to update before sleeping.
    but_rec.update();
    but_mon.update();
    but_blue.update();
    // switch off i2s clock before sleeping
    SIM_SCGC6 &= ~SIM_SCGC6_I2S;
    Alarm.delay(50);
    REDUCED_CPU_BLOCK(snooze_cpu) {
        who = Snooze.hibernate(snooze_config);	// returns module that woke up processor
    }
    setTimeSource(); // Re-adjust time, since Snooze doesn't keep it
    if(who == WAKESOURCE_RTC) {
        // Snooze wake-up -> remove alarm and re-start recording
        removeIdleSnooze();
        working_state.rec_state = RECSTATE_REQ_RESTART;
    }
    else {
        // Button wake-up -> debounce and notify which button was pressed
        Bounce cur_bt = Bounce(who, BUTTON_BOUNCE_TIME_MS);
        elapsedMillis timeout = 0;
        while (timeout < (BUTTON_BOUNCE_TIME_MS+1)) cur_bt.update();
        button_call = (enum bCalls)who;
    }
    // if not sleeping anymore, re-enable i2s clock
    SIM_SCGC6 |= SIM_SCGC6_I2S;
    Alarm.delay(50);
#endif // ALWAYS_ON_MODE

WORK:
    Alarm.delay(0); 							// needed for TimeAlarms timers
    but_rec.update();							// }
    but_mon.update();							// } needed for button bounces
    but_blue.update();							// }

    // Button edge detection -> notification
    if(but_rec.fallingEdge()) button_call = (enum bCalls)BUTTON_RECORD_PIN;
    if(but_mon.fallingEdge()) button_call = (enum bCalls)BUTTON_MONITOR_PIN;
    if(but_blue.fallingEdge()) button_call = (enum bCalls)BUTTON_BLUETOOTH_PIN;

    // Centralized button actions from WORK or SLEEP mode...
#if(ALWAYS_ON_MODE==1)
#else
    // ...dealing with IDLE and WAIT modes
    if((button_call == BUTTON_MONITOR_PIN) || (button_call == BUTTON_BLUETOOTH_PIN)) {
        // button interruption during REC IDLE mode -> need to set an new alarm & change to WAIT mode
        if(working_state.rec_state == RECSTATE_IDLE) {
            removeIdleSnooze();
            setWaitAlarm();
            if(working_state.rec_state != RECSTATE_REQ_OFF) {
                working_state.rec_state = RECSTATE_WAIT;
            }
        }
        // button interruption during REC WAIT mode -> need to set a new snooze & change to IDLE mode
        else if(working_state.rec_state == RECSTATE_WAIT) {
            removeWaitAlarm();
            setIdleSnooze();
            working_state.rec_state = RECSTATE_IDLE;
        }
    }
#endif // ALWAYS_ON_MODE

    // ...standard button actions
    if(button_call == BUTTON_RECORD_PIN) {
        // if(debug) MONPORT.printf("Info:    REC! Current state (R/M/BLE/BT): %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state, working_state.ble_state, working_state.bt_state);
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
        // if(debug) MONPORT.printf("Info:    MON! Current state (R/M/BLE/BT): %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state, working_state.ble_state, working_state.bt_state);
        if(working_state.mon_state == MONSTATE_OFF) {
            working_state.mon_state = MONSTATE_REQ_ON;
        }
        else {
            working_state.mon_state = MONSTATE_REQ_OFF;
        }
        button_call = (enum bCalls)BCALL_NONE;
    }
    if(button_call == BUTTON_BLUETOOTH_PIN) {
        // if(debug) MONPORT.printf("Info:    BLUE! Current state (R/M/BLE/BT): %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state, working_state.ble_state, working_state.bt_state);
        if(working_state.ble_state == BLESTATE_OFF) {
            working_state.ble_state = BLESTATE_REQ_ADV;
        }
        else {
            if(working_state.ble_state == BLESTATE_ADV) {
                Alarm.disable(alarm_adv_id);
            }
            working_state.ble_state = BLESTATE_REQ_OFF;
        }
        button_call = (enum bCalls)BCALL_NONE;
    }

    // REC state actions
    switch(working_state.rec_state) {
        case RECSTATE_REQ_ON: {
            if(debug) MONPORT.printf("Info:    Requesting REC ON. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            next_record.cnt = 0;
            prepareRecording(true);
            working_state.rec_state = RECSTATE_ON;
            startRecording(next_record.rpath);
            ready_to_sleep = false;
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_REC_TS);
                sendCmdOut(BCNOT_LATLONG);
                sendCmdOut(BCNOT_RWIN_VALS);
                sendCmdOut(BCNOT_FILEPATH);
                sendCmdOut(BCNOT_REC_NB);
                sendCmdOut(BCNOT_REC_STATE);
            }
            break;
        }

        case RECSTATE_REQ_RESTART: {
            if(debug) MONPORT.printf("Info:    Requesting REC RESTART. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            prepareRecording(true);
            working_state.rec_state = RECSTATE_ON;
            startRecording(next_record.rpath);
            ready_to_sleep = false;
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_REC_TS);
                sendCmdOut(BCNOT_LATLONG);
                sendCmdOut(BCNOT_RWIN_VALS);
                sendCmdOut(BCNOT_FILEPATH);
                sendCmdOut(BCNOT_REC_NB);
                sendCmdOut(BCNOT_REC_STATE);
            }
            break;
        }

        case RECSTATE_REQ_PAUSE: {
            if(debug) MONPORT.printf("Info:    Requesting REC PAUSE. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            stopRecording(next_record.rpath);
            pauseRecording();
            if( (working_state.mon_state != MONSTATE_OFF) || (working_state.ble_state != BLESTATE_OFF) || (working_state.bt_state != BTSTATE_OFF) ) {
                setWaitAlarm();
                working_state.rec_state = RECSTATE_WAIT;
                ready_to_sleep = false;
            }
            else {
                setIdleSnooze();
                working_state.rec_state = RECSTATE_IDLE;
                ready_to_sleep = true;
            }
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_REC_NEXT);
            	sendCmdOut(BCNOT_REC_STATE);
            	sendCmdOut(BCNOT_FILEPATH);
            }
            break;
        }

        case RECSTATE_ON: {
            continueRecording();
            detectPeaks();
            ready_to_sleep = false;
            break;
        }

        case RECSTATE_REQ_OFF: {
            if(debug) MONPORT.printf("Info:    Requesting REC OFF. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            Alarm.free(alarm_wait_id);
            Alarm.free(alarm_rec_id);

            stopRecording(next_record.rpath);
            finishRecording();
            stopLED(&leds[LED_PEAK]);
            working_state.rec_state = RECSTATE_OFF;
            if( (working_state.mon_state == MONSTATE_OFF) && (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
                ready_to_sleep = true;
            }
            else {
                ready_to_sleep = false;
            }
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_REC_STATE);
                sendCmdOut(BCNOT_FILEPATH);
            }
            break;
        }

        case RECSTATE_OFF: {
            if( (working_state.mon_state == MONSTATE_OFF) && (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
                ready_to_sleep = true;
            }
            else {
                ready_to_sleep = false;
            }
            break;
        }

        default:
        break;
    }
    // MON state actions
    switch(working_state.mon_state) {
        case MONSTATE_REQ_ON: {
            if(debug) MONPORT.printf("Info:    Requesting MON ON. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            startLED(&leds[LED_MONITOR], LED_MODE_ON);
            startMonitoring();
            working_state.mon_state = MONSTATE_ON;
            ready_to_sleep = false;
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                sendCmdOut(BCCMD_MON_START);
                sendCmdOut(BCCMD_VOL_A2DP);
                working_state.bt_state = BTSTATE_PLAY;
            }
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_MON_STATE);
            }
            break;
        }

        case MONSTATE_ON: {
            if(hpgain_interval > HPGAIN_INTERVAL_MS) {
                setHpGain();
                hpgain_interval = 0;
            }
            if(peak_interval > PEAK_INTERVAL_MS) {
                detectPeaks();
                peak_interval = 0;
            }
            ready_to_sleep = false;
            break;
        }

        case MONSTATE_REQ_OFF: {
            if(debug) MONPORT.printf("Info:    Requesting MON OFF. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            stopMonitoring();
            stopLED(&leds[LED_MONITOR]);
            stopLED(&leds[LED_PEAK]);
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                sendCmdOut(BCCMD_MON_STOP);
                working_state.bt_state = BTSTATE_CONNECTED;
            }
            working_state.mon_state = MONSTATE_OFF;

            if( (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
                if( (working_state.rec_state == RECSTATE_OFF) || (working_state.rec_state == RECSTATE_IDLE) ) {
                    ready_to_sleep = true;
                }
                // else if(working_state.rec_state == RECSTATE_WAIT) {
                //     // setIdleSnooze();
                //     // working_state.rec_state = RECSTATE_IDLE;
                //     ready_to_sleep = true;
                // }
                else {
                    ready_to_sleep = false;
                }
            }

            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCNOT_MON_STATE);
            }
            break;
        }

        case MONSTATE_OFF: {
            // if( (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
            //     if(working_state.rec_state == RECSTATE_OFF) {
            //         ready_to_sleep = true;
            //     }
            //     else if(working_state.rec_state == RECSTATE_WAIT) {
            //         setIdleSnooze();
            //         working_state.rec_state = RECSTATE_IDLE;
            //         ready_to_sleep = true;
            //     }
            //     else {
            //         ready_to_sleep = false;
            //     }
            // }
            break;
        }

        default:
        break;
    }
    // BLE state actions
    switch(working_state.ble_state) {
        case BLESTATE_REQ_ADV: {
            if(debug) MONPORT.printf("Info:    Requesting BLE ADV. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
            }
            else {
                bc127BlueOn();
                delay(500);
                startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
                alarm_adv_id = Alarm.timerOnce(BLEADV_TIMEOUT_S, alarmAdvTimeout);
            }
            bc127AdvStart();
            working_state.ble_state = BLESTATE_ADV;
            ready_to_sleep = false;
            break;
        }

        case BLESTATE_REQ_CONN: {
            if(debug) MONPORT.printf("Info:    Requesting BLE CONN. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            Alarm.free(alarm_adv_id);
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
            }
            else {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
            }
            working_state.ble_state = BLESTATE_CONNECTED;
            ready_to_sleep = false;
            break;
        }

        case BLESTATE_CONNECTED: {
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
            }
            ready_to_sleep = false;
            break;
        }

        case BLESTATE_REQ_DISC: {
            if(debug) MONPORT.printf("Info:    Requesting BLE DISC. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            BLE_conn_id = 0;
            if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
                working_state.ble_state = BLESTATE_REQ_ADV;
            }
            else {
                working_state.ble_state = BLESTATE_REQ_OFF;
            }

            ready_to_sleep = false;
            break;
        }

        case BLESTATE_REQ_OFF: {
            if(debug) MONPORT.printf("Info:    Requesting BLE OFF. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            if(working_state.ble_state == BLESTATE_ADV) {
                Alarm.disable(alarm_adv_id);
                // Alarm.free(alarm_adv_id);
                bc127AdvStop();
                Alarm.delay(100);
            }
            if(working_state.bt_state != BTSTATE_OFF) {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
            }
            else {
                bc127BlueOff();
                stopLED(&leds[LED_BLUETOOTH]);
            }
            working_state.ble_state = BLESTATE_OFF;
            if(working_state.bt_state == BTSTATE_OFF) {
                // if(working_state.rec_state == RECSTATE_OFF) {
                //     ready_to_sleep = true;
                // }
                if(working_state.rec_state == RECSTATE_WAIT) {
                    removeWaitAlarm();
                    setIdleSnooze();
                    working_state.rec_state = RECSTATE_IDLE;
                    // ready_to_sleep = true;
                }
                // else {
                //     ready_to_sleep = false;
                // }

                if(working_state.mon_state != MONSTATE_OFF) {
                    working_state.mon_state = MONSTATE_REQ_OFF;
                    ready_to_sleep = false;
                }
                else {
                    ready_to_sleep = true;
                }

            }
            else {
                ready_to_sleep = false;
            }
            break;
        }

        default:
        break;
    }
    // BT state actions
    switch(working_state.bt_state) {
        case BTSTATE_REQ_CONN: {
            if(debug) MONPORT.printf("Info:    Requesting BT CONN. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            // Alarm.free(alarm_adv_id);
            working_state.bt_state = BTSTATE_CONNECTED;
            ready_to_sleep = false;
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
                sendCmdOut(BCNOT_BT_STATE);
            }
            else {
                startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
            }
            break;
        }

        case BTSTATE_REQ_DISC: {
            if(debug) MONPORT.printf("Info:    Requesting BT DISC. States: BT %d, BLE %d, REC %d, MON %d\n", working_state.bt_state, working_state.ble_state, working_state.rec_state, working_state.mon_state);
            if(working_state.ble_state == BLESTATE_CONNECTED) {
                sendCmdOut(BCCMD_DEV_DISCONNECT1);
                sendCmdOut(BCCMD_DEV_DISCONNECT2);
                startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
                sendCmdOut(BCNOT_BT_STATE);
            }
            else {
                bc127BlueOff();
                stopLED(&leds[LED_BLUETOOTH]);
                working_state.ble_state = BLESTATE_OFF;
            }
            working_state.bt_state = BTSTATE_OFF;
            BT_id_a2dp = 0;
            BT_id_avrcp = 0;
            BT_peer_address = "";
            BT_peer_name = "auto";
            if( (working_state.rec_state == RECSTATE_OFF) && (working_state.mon_state == MONSTATE_OFF) && (working_state.ble_state == BLESTATE_OFF)) {
                ready_to_sleep = true;
            }
            else {
                ready_to_sleep = false;
            }
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
            if(debug) MONPORT.println("Error: Sending command error!!");
        }
    }
    if (MONPORT.available()) {
        String manInput = MONPORT.readStringUntil('\n');
        int len = manInput.length() - 1;
        BLUEPORT.print(manInput.substring(0, len)+'\r');
        MONPORT.printf("Sent to BLUEPORT: %s\n", manInput.c_str());
    }

#if(ALWAYS_ON_MODE==1)
    if(working_state.rec_state == RECSTATE_REQ_PAUSE) {
        setWaitAlarm();
        working_state.rec_state = RECSTATE_WAIT;
    }
    goto WORK;
#else
    // Sleeping or not sleeping... logical decision chain
    // 1. MON/BLE/BT off
    // 		1.1. if REC == REQ_PAUSE -> prepare the snooze alarm & SLEEP (IDLE mode)
    //		1.2. if REC == OFF or REC == IDLE -> SLEEP
    //		1.3. else -> WORK
    // 2. else
    //		2.1. if REC == REQ_PAUSE -> prepare the time alarm & WORK (WAIT mode)
    //		2.2. else -> WORK
    // if( (working_state.mon_state == MONSTATE_OFF) && (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
    //     if(working_state.rec_state == RECSTATE_REQ_PAUSE) {
    //         setIdleSnooze();
    //         working_state.rec_state = RECSTATE_IDLE;
    //         ready_to_sleep = true;
    //     }
    //     else if((working_state.rec_state == RECSTATE_OFF) || (working_state.rec_state == RECSTATE_IDLE)) {
    //         ready_to_sleep = true;
    //     }
    //     else {
    //         ready_to_sleep = false;
    //     }
    // }
    // else {
    //     if(working_state.rec_state == RECSTATE_REQ_PAUSE) {
    //         // setWaitAlarm();
    //         working_state.rec_state = RECSTATE_WAIT;
    //         if(working_state.ble_state == BLESTATE_CONNECTED) {
    //             sendCmdOut(BCNOT_REC_STATE);
    //             sendCmdOut(BCNOT_FILEPATH);
    //         }
    //     }
    //     ready_to_sleep = false;
    // }

    // Final decision
    if(ready_to_sleep) goto SLEEP;
    else goto WORK;

#endif // ALWAYS_ON_MODE
}


/* setDefaultValues(void)
* ----------------------
*/
void setDefaultValues(void) {
    working_state.rec_state = RECSTATE_OFF;
    working_state.mon_state = MONSTATE_OFF;
    working_state.bt_state = BTSTATE_OFF;
    working_state.ble_state = BLESTATE_OFF;
    rec_window.duration.Second = RWIN_DUR_DEF_SEC;
    rec_window.duration.Minute = RWIN_DUR_DEF_MIN;
    rec_window.duration.Hour = RWIN_DUR_DEF_HOUR;
    rec_window.duration.Day = RWIN_DUR_DEF_DAY;
    rec_window.duration.Month = RWIN_DUR_DEF_DAY;
    rec_window.duration.Year = RWIN_DUR_DEF_DAY;
    rec_window.period.Second = RWIN_PER_DEF_SEC;
    rec_window.period.Minute = RWIN_PER_DEF_MIN;
    rec_window.period.Hour = RWIN_PER_DEF_HOUR;
    rec_window.period.Day = RWIN_PER_DEF_DAY;
    rec_window.period.Month = RWIN_PER_DEF_MON;
    rec_window.period.Year = RWIN_PER_DEF_YEAR;
    rec_window.occurences = RWIN_OCC_DEF;
    last_record.cnt = 0;
    last_record.gps_source = GPS_NONE;
    last_record.gps_lat = NULL;
    last_record.gps_long = NULL;
    next_record = last_record;
    BT_id_a2dp = 0;
    BT_id_avrcp = 0;
    BLE_conn_id = 0;
    BT_peer_address = "";
    BT_peer_name = "auto";
}


void helloWorld(void) {
    startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
    Alarm.delay(300);
    startLED(&leds[LED_MONITOR], LED_MODE_ON);
    stopLED(&leds[LED_BLUETOOTH]);
    Alarm.delay(150);
    startLED(&leds[LED_RECORD], LED_MODE_ON);
    stopLED(&leds[LED_MONITOR]);
    Alarm.delay(150);
    startLED(&leds[LED_MONITOR], LED_MODE_ON);
    stopLED(&leds[LED_RECORD]);
    Alarm.delay(300);
    startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
    stopLED(&leds[LED_MONITOR]);
    Alarm.delay(600);
    startLED(&leds[LED_MONITOR], LED_MODE_ON);
    stopLED(&leds[LED_BLUETOOTH]);
    Alarm.delay(300);
    startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
    startLED(&leds[LED_RECORD], LED_MODE_ON);
    stopLED(&leds[LED_MONITOR]);
    Alarm.delay(600);
    stopLED(&leds[LED_BLUETOOTH]);
    stopLED(&leds[LED_RECORD]);
}
