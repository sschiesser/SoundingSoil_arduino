/*
AudioShield firmware for SoundingSoil
-------------------------------------
Mixing different code bases:
- Serial_monitor (ICST)
- Teensy_recorder (PJRC)
*/
#include "main.h"

#define ALWAYS_ON_MODE 0

// install driver into SnoozeBlock
// SnoozeBlock
// snooze_config(button_wakeup);

bool debug = true;
#if (ALWAYS_ON_MODE == 1)
bool sleeping_permitted = false;
#else
bool sleeping_permitted = true;
#endif

volatile struct wState working_state;
struct rWindow rec_window;
struct recInfo last_record;
struct recInfo next_record;

struct sfState sleep_flags;
bool rts;

enum gpsSource gps_source;

void setup() {
  // Initialize serial ports:
  // if(debug) snooze_usb.begin(115200);
  // // Serial monitor port
  if (debug) {
    while (!snooze_usb)
      ;
    delay(100);
  }
  BLUEPORT.begin(9600); // BC127 communication port
  GPSPORT.begin(9600);  // GPS port

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

  // Debug...
  if (debug)
    snooze_usb.printf("Info:    SoundingSoil firmware version 1.0 build "
                      "%02d%02d%02d%02d%02d\n",
                      (tm.Year - 30), tm.Month, tm.Day, tm.Hour, tm.Minute);

  working_state.ble_state = BLESTATE_REQ_ADV;
  sleep_flags.rec_ready = true;
  sleep_flags.mon_ready = true;
  sleep_flags.bt_ready = true;
  sleep_flags.ble_ready = false;
  rts = setRts(sleep_flags);
}

void loop() {
#if (ALWAYS_ON_MODE == 1)
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
  // REDUCED_CPU_BLOCK(snooze_cpu) {
  // returns module that woke up processor
  who = Snooze.hibernate(snooze_config);
  // }

  // Re-adjust time, since Snooze doesn't keep it
  setTimeSource();

  if (who == WAKESOURCE_RTC) {
    // Snooze wake-up -> remove alarm and re-start recording
    removeIdleSnooze();
    working_state.rec_state = RECSTATE_REQ_RESTART;
  } else {
    // Button wake-up -> debounce and notify which button was pressed
    Bounce cur_bt = Bounce(who, BUTTON_BOUNCE_TIME_MS);
    elapsedMillis timeout = 0;
    while (timeout < (BUTTON_BOUNCE_TIME_MS + 1))
      cur_bt.update();
    button_call = (enum bCalls)who;
  }
  // if not sleeping anymore, re-enable i2s clock
  SIM_SCGC6 |= SIM_SCGC6_I2S;
  Alarm.delay(50);
#endif // ALWAYS_ON_MODE

WORK:
  Alarm.delay(0);    // needed for TimeAlarms timers
  but_rec.update();  // }
  but_mon.update();  // } needed for button bounces
  but_blue.update(); // }

  // Button edge detection -> notification
  if (but_rec.fallingEdge())
    button_call = (enum bCalls)BUTTON_RECORD_PIN;
  if (but_mon.fallingEdge())
    button_call = (enum bCalls)BUTTON_MONITOR_PIN;
  if (but_blue.fallingEdge())
    button_call = (enum bCalls)BUTTON_BLUETOOTH_PIN;

  // Centralized button actions from WORK or SLEEP mode...
  // #if(ALWAYS_ON_MODE==1)
  // #else
  // ...dealing with IDLE and WAIT modes
  if ((button_call == BUTTON_MONITOR_PIN) ||
      (button_call == BUTTON_BLUETOOTH_PIN)) {
    // button interruption during REC IDLE mode -> need to set an new alarm &
    // change to WAIT mode
    if (working_state.rec_state == RECSTATE_IDLE) {
      removeIdleSnooze();
      setWaitAlarm();
      if (working_state.rec_state != RECSTATE_REQ_OFF) {
        working_state.rec_state = RECSTATE_WAIT;
      }
    }
    // button interruption during REC WAIT mode -> need to set a new snooze &
    // change to IDLE mode
    else if ((working_state.rec_state == RECSTATE_WAIT) &&
             (sleeping_permitted == true)) {
      removeWaitAlarm();
      setIdleSnooze();
      working_state.rec_state = RECSTATE_IDLE;
    }
  }
  // #endif // ALWAYS_ON_MODE

  // ...standard button actions
  if (button_call == BUTTON_RECORD_PIN) {
    // if(debug) snooze_usb.printf("Info:    REC! Current state (R/M/BLE/BT):
    // %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state,
    // working_state.ble_state, working_state.bt_state);
    if (working_state.rec_state == RECSTATE_OFF) {
      working_state.rec_state = RECSTATE_REQ_ON;
    } else {
      snooze_config -= snooze_rec;
      // Alarm.free(alarm_wait_id);
      // Alarm.free(alarm_rec_id);
      working_state.rec_state = RECSTATE_REQ_OFF;
    }
    button_call = (enum bCalls)BCALL_NONE;
  }
  if (button_call == BUTTON_MONITOR_PIN) {
    // if(debug) snooze_usb.printf("Info:    MON! Current state (R/M/BLE/BT):
    // %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state,
    // working_state.ble_state, working_state.bt_state);
    if (working_state.mon_state == MONSTATE_OFF) {
      working_state.mon_state = MONSTATE_REQ_ON;
    } else {
      working_state.mon_state = MONSTATE_REQ_OFF;
    }
    button_call = (enum bCalls)BCALL_NONE;
  }
  if (button_call == BUTTON_BLUETOOTH_PIN) {
    // if(debug) snooze_usb.printf("Info:    BLUE! Current state (R/M/BLE/BT):
    // %d/%d/%d/%d\n", working_state.rec_state, working_state.mon_state,
    // working_state.ble_state, working_state.bt_state);
    if (working_state.ble_state == BLESTATE_OFF) {
      working_state.ble_state = BLESTATE_REQ_ADV;
    } else {
      if (working_state.ble_state == BLESTATE_ADV) {
        Alarm.disable(alarm_adv_id);
      }
      working_state.ble_state = BLESTATE_REQ_OFF;
    }
    button_call = (enum bCalls)BCALL_NONE;
  }

  // REC state actions
  switch (working_state.rec_state) {
  case RECSTATE_REQ_ON: {
    // Debug...
    if (debug)
      snooze_usb.printf(
          "Info:    Requesting REC ON. States: BT %d, BLE %d, REC %d, MON %d\n",
          working_state.bt_state, working_state.ble_state,
          working_state.rec_state, working_state.mon_state);

    // Doing things
    next_record.cnt = 0;
    prepareRecording(true);
    working_state.rec_state = RECSTATE_ON;
    startRecording(next_record.rpath);

    // Checking other states
    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_REC_TS);
      sendCmdOut(BCNOT_LATLONG);
      sendCmdOut(BCNOT_RWIN_VALS);
      sendCmdOut(BCNOT_FILEPATH);
      sendCmdOut(BCNOT_REC_NB);
      sendCmdOut(BCNOT_REC_STATE);
    }

    // Setting sleep flag
    sleep_flags.rec_ready = false;

    break;
  }

  case RECSTATE_REQ_RESTART: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting REC RESTART. States: BT %d, BLE "
                        "%d, REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    // Doing things
    prepareRecording(true);
    working_state.rec_state = RECSTATE_ON;
    startRecording(next_record.rpath);

    // Checking other states
    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_REC_TS);
      sendCmdOut(BCNOT_LATLONG);
      sendCmdOut(BCNOT_RWIN_VALS);
      sendCmdOut(BCNOT_FILEPATH);
      sendCmdOut(BCNOT_REC_NB);
      sendCmdOut(BCNOT_REC_STATE);
    }

    // Setting sleep flag
    sleep_flags.rec_ready = false;

    break;
  }

  case RECSTATE_REQ_PAUSE: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting REC PAUSE. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    // Doing things
    stopRecording(next_record.rpath);
    pauseRecording();
    if ((working_state.mon_state != MONSTATE_OFF) ||
        (working_state.ble_state != BLESTATE_OFF) ||
        (working_state.bt_state != BTSTATE_OFF) ||
        (sleeping_permitted == false)) {
      setWaitAlarm();
      working_state.rec_state = RECSTATE_WAIT;
      sleep_flags.rec_ready = false;
    } else {
      setIdleSnooze();
      working_state.rec_state = RECSTATE_IDLE;
      sleep_flags.rec_ready = true;
    }

    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_REC_NEXT);
      sendCmdOut(BCNOT_REC_STATE);
      sendCmdOut(BCNOT_FILEPATH);
    }
    break;
  }

  case RECSTATE_ON: {
    continueRecording();
    detectPeaks();
    sleep_flags.rec_ready = false;
    break;
  }

  case RECSTATE_REQ_OFF: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting REC OFF. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    Alarm.free(alarm_wait_id);
    Alarm.free(alarm_rec_id);
    stopRecording(next_record.rpath);
    finishRecording();
    stopLED(&leds[LED_PEAK]);
    working_state.rec_state = RECSTATE_OFF;
    sleep_flags.rec_ready = true;
    sleep_flags.mon_ready = (working_state.mon_state == MONSTATE_OFF ? true : false);
    sleep_flags.ble_ready = (working_state.ble_state == BLESTATE_OFF ? true : false);
    sleep_flags.bt_ready = (working_state.bt_state == BTSTATE_OFF ? true : false);

    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_REC_STATE);
      sendCmdOut(BCNOT_FILEPATH);
    }
    break;
  }

  case RECSTATE_OFF: {
    sleep_flags.rec_ready = true;
    sleep_flags.mon_ready = (working_state.mon_state == MONSTATE_OFF ? true : false);
    sleep_flags.ble_ready = (working_state.ble_state == BLESTATE_OFF ? true : false);
    sleep_flags.bt_ready = (working_state.bt_state == BTSTATE_OFF ? true : false);
    break;
  }

  default:
    break;
  }

  // MON state actions
  switch (working_state.mon_state) {
  case MONSTATE_REQ_ON: {
    // Debug...
    if (debug)
      snooze_usb.printf(
          "Info:    Requesting MON ON. States: BT %d, BLE %d, REC %d, MON %d\n",
          working_state.bt_state, working_state.ble_state,
          working_state.rec_state, working_state.mon_state);

    // Doing things
    startLED(&leds[LED_MONITOR], LED_MODE_ON);
    startMonitoring();
    working_state.mon_state = MONSTATE_ON;
    sleep_flags.mon_ready = false;

    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      sendCmdOut(BCCMD_MON_START);
      sendCmdOut(BCCMD_VOL_A2DP);
      working_state.bt_state = BTSTATE_PLAY;
    }
    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_MON_STATE);
    }
    break;
  }

  case MONSTATE_ON: {
    if (hpgain_interval > HPGAIN_INTERVAL_MS) {
      setHpGain();
      hpgain_interval = 0;
    }
    if (peak_interval > PEAK_INTERVAL_MS) {
      detectPeaks();
      peak_interval = 0;
    }
    sleep_flags.mon_ready = false;
    break;
  }

  case MONSTATE_REQ_OFF: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting MON OFF. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    stopMonitoring();
    stopLED(&leds[LED_MONITOR]);
    stopLED(&leds[LED_PEAK]);
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      sendCmdOut(BCCMD_MON_STOP);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    working_state.mon_state = MONSTATE_OFF;

    if ((working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF)) {
      if ((working_state.rec_state == RECSTATE_OFF) || (working_state.rec_state == RECSTATE_IDLE)) {
        sleep_flags.mon_ready = true;
      }
      // else if(working_state.rec_state == RECSTATE_WAIT) {
      //     // setIdleSnooze();
      //     // working_state.rec_state = RECSTATE_IDLE;
      //     rts = true;
      // }
      else {
        sleep_flags.mon_ready = false;
      }
    }

    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCNOT_MON_STATE);
    }
    break;
  }

  case MONSTATE_OFF: {
    // if( (working_state.ble_state == BLESTATE_OFF) && (working_state.bt_state
    // == BTSTATE_OFF) ) {
    //     if(working_state.rec_state == RECSTATE_OFF) {
    //         rts = true;
    //     }
    //     else if(working_state.rec_state == RECSTATE_WAIT) {
    //         setIdleSnooze();
    //         working_state.rec_state = RECSTATE_IDLE;
    //         rts = true;
    //     }
    //     else {
    //         rts = false;
    //     }
    // }
    sleep_flags.mon_ready = true;
    break;
  }

  default:
    break;
  }

  // BLE state actions
  switch (working_state.ble_state) {
  case BLESTATE_REQ_ADV: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BLE ADV. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    // BT state check
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
    } else {
      bc127BlueOn();
      delay(500);
      startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
      alarm_adv_id = Alarm.timerOnce(BLEADV_TIMEOUT_S, alarmAdvTimeout);
    }

    bc127AdvStart();
    working_state.ble_state = BLESTATE_ADV;
    sleep_flags.ble_ready = false;
    break;
  }

  case BLESTATE_REQ_CONN: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BLE CONN. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    Alarm.free(alarm_adv_id);

    // BT state check
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
    } else {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
    }

    // Setting states
    working_state.ble_state = BLESTATE_CONNECTED;
    sleep_flags.ble_ready = false;

    break;
  }

  case BLESTATE_CONNECTED: {
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
    }
    sleep_flags.ble_ready = false;
    break;
  }

  case BLESTATE_REQ_DISC: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BLE DISC. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    BLE_conn_id = 0;
    // If a BT classic device (headset) is connected, re-advertise for 30 seconds. Then just switch off the bluetooth link
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
      working_state.ble_state = BLESTATE_REQ_ADV;
      sleep_flags.ble_ready = false;
    } else {
      working_state.ble_state = BLESTATE_REQ_OFF;
      sleep_flags.ble_ready = true;
    }

    break;
  }

  case BLESTATE_REQ_OFF: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BLE OFF. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    if (working_state.ble_state == BLESTATE_ADV) {
      Alarm.disable(alarm_adv_id);
      // Alarm.free(alarm_adv_id);
      bc127AdvStop();
      Alarm.delay(100);
    }
    if (working_state.bt_state != BTSTATE_OFF) {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
    } else {
      bc127BlueOff();
      stopLED(&leds[LED_BLUETOOTH]);
    }
    working_state.ble_state = BLESTATE_OFF;
    sleep_flags.ble_ready = true;

    if (working_state.bt_state == BTSTATE_OFF) {
      if ((working_state.rec_state == RECSTATE_WAIT) &&
          (sleeping_permitted == true)) {
        removeWaitAlarm();
        setIdleSnooze();
        working_state.rec_state = RECSTATE_IDLE;
        sleep_flags.rec_ready = true;
      } else if (working_state.rec_state == RECSTATE_ON) {
        sleep_flags.rec_ready = false;
      } else if (working_state.mon_state != MONSTATE_OFF) {
        working_state.mon_state = MONSTATE_REQ_OFF;
        sleep_flags.mon_ready = false;
      } else {
        sleep_flags.mon_ready = true;
      }
    } else {
      sleep_flags.bt_ready = false;
    }
    break;
  }

  default:
    break;
  }

  // BT state actions
  switch (working_state.bt_state) {
  case BTSTATE_REQ_CONN: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BT CONN. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    // Alarm.free(alarm_adv_id);
    working_state.bt_state = BTSTATE_CONNECTED;
    sleep_flags.bt_ready = false;

    if (working_state.ble_state == BLESTATE_CONNECTED) {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
      sendCmdOut(BCNOT_BT_STATE);
    } else {
      startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
    }
    break;
  }

  case BTSTATE_REQ_DISC: {
    // Debug...
    if (debug)
      snooze_usb.printf("Info:    Requesting BT DISC. States: BT %d, BLE %d, "
                        "REC %d, MON %d\n",
                        working_state.bt_state, working_state.ble_state,
                        working_state.rec_state, working_state.mon_state);

    if (working_state.ble_state == BLESTATE_CONNECTED) {
      sendCmdOut(BCCMD_DEV_DISCONNECT1);
      sendCmdOut(BCCMD_DEV_DISCONNECT2);
      startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
      sendCmdOut(BCNOT_BT_STATE);
      sleep_flags.ble_ready = false;
    } else {
      bc127BlueOff();
      stopLED(&leds[LED_BLUETOOTH]);
      working_state.ble_state = BLESTATE_OFF;
      sleep_flags.ble_ready = true;
    }

    working_state.mon_state = MONSTATE_REQ_OFF;
    working_state.bt_state = BTSTATE_OFF;
    sleep_flags.bt_ready = true;

    BT_id_a2dp = 0;
    BT_id_avrcp = 0;
    BT_peer_address = "";
    BT_peer_name = "auto";

    sleep_flags.rec_ready = (working_state.rec_state == RECSTATE_OFF ? true : false);
    sleep_flags.mon_ready = (working_state.mon_state == MONSTATE_OFF ? true : false);
    sleep_flags.ble_ready = (working_state.ble_state == BLESTATE_OFF ? true : false);

    break;
  }

  default:
    break;
  }

  // Serial messaging
  if (BLUEPORT.available()) {
    String inMsg = BLUEPORT.readStringUntil('\r');
    int outMsg = parseSerialIn(inMsg);
    if (!sendCmdOut(outMsg)) {
      if (debug)
        snooze_usb.println("Error: Sending command error!!");
    }
  }
  if (snooze_usb.available()) {
    String manInput = snooze_usb.readStringUntil('\n');
    int len = manInput.length() - 1;
    BLUEPORT.print(manInput.substring(0, len) + '\r');
    snooze_usb.printf("Sent to BLUEPORT: %s\n", manInput.c_str());
  }

#if (ALWAYS_ON_MODE == 1)
  if (working_state.rec_state == RECSTATE_REQ_PAUSE) {
    setWaitAlarm();
    working_state.rec_state = RECSTATE_WAIT;
  }
  goto WORK;
#else
  // Sleeping or not sleeping... logical decision chain
  // 1. MON/BLE/BT off
  // 		1.1. if REC == REQ_PAUSE -> prepare the snooze alarm & SLEEP
  // (IDLE mode)
  //		1.2. if REC == OFF or REC == IDLE -> SLEEP
  //		1.3. else -> WORK
  // 2. else
  //		2.1. if REC == REQ_PAUSE -> prepare the time alarm & WORK (WAIT
  // mode) 		2.2. else -> WORK
  // if( (working_state.mon_state == MONSTATE_OFF) && (working_state.ble_state
  // == BLESTATE_OFF) && (working_state.bt_state == BTSTATE_OFF) ) {
  //     if(working_state.rec_state == RECSTATE_REQ_PAUSE) {
  //         setIdleSnooze();
  //         working_state.rec_state = RECSTATE_IDLE;
  //         rts = true;
  //     }
  //     else if((working_state.rec_state == RECSTATE_OFF) ||
  //     (working_state.rec_state == RECSTATE_IDLE)) {
  //         rts = true;
  //     }
  //     else {
  //         rts = false;
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
  //     rts = false;
  // }

  // Final decision
  rts = setRts(sleep_flags);
  if (rts)
    goto SLEEP;
  else
    goto WORK;

#endif // ALWAYS_ON_MODE
}

/* setDefaultValues(void)
 * ----------------------
 */
bool setRts(struct sfState sf) {
  static struct sfState sf_old;

  bool toRet = (sf.rec_ready & sf.mon_ready & sf.ble_ready & sf.bt_ready);
  if( (sf.rec_ready != sf_old.rec_ready) || (sf.mon_ready != sf_old.mon_ready) || (sf.ble_ready != sf_old.ble_ready) || (sf.bt_ready != sf_old.bt_ready) ) {
    if(debug) snooze_usb.printf("Info:    sf: %d %d %d %d -> rts: %d\n", sf.rec_ready, sf.mon_ready, sf.ble_ready, sf.bt_ready, toRet);
    sf_old.rec_ready = sf.rec_ready;
    sf_old.mon_ready = sf.mon_ready;
    sf_old.ble_ready = sf.ble_ready;
    sf_old.bt_ready = sf.bt_ready;
  }
  return toRet;
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
