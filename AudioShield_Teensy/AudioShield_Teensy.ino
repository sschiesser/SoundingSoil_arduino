/*
  AudioShield firmware for SoundingSoil
  -------------------------------------
  Mixing different code bases:
  - Serial_monitor (ICST)
  - Teensy_recorder (PJRC)
*/
#include "main.h"

// Load drivers
SnoozeDigital 								digitalWakeup;
// Available digital pins for wakeup on Teensy 3.6: 2,4,6,7,9,10,11,13,16,21,22,26,30,33


// install driver into SnoozeBlock
SnoozeBlock 									config_teensy3x(digitalWakeup);

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

struct wState 								working_state;

void setup() {
    // // Configure pin 2 for bounce library
    // pinMode(21, INPUT_PULLUP);
    // debug led
    // pinMode(LED_BUILTIN, OUTPUT);
    // while (!Serial);
    // delay(100);
    // Serial.println("start...");
    // delay(20);
    //pin, mode, type
  // Initialize both serial ports:
  Serial.begin(9600);															// Serial monitor port
  Serial4.begin(9600);														// BC127 communication port
  gpsPort.begin(9600);														// GPS port

	// Say hello
	delay(100);
  Serial.println("AudioShield v1.0");
  Serial.println("----------------");
	delay(20);
  
	// Initialize peripheral & variables
	initLEDButtons();
	digitalWakeup.pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP, FALLING);
	digitalWakeup.pinMode(BUTTON_MONITOR_PIN, INPUT_PULLUP, FALLING);
  digitalWakeup.pinMode(BUTTON_BLUETOOTH_PIN, INPUT_PULLUP, FALLING);

	// initAudio();
	// initSDcard();
	// initWaveHeader();
	// initStates();
	// bc127Init();
}

void loop() {
    // if not held for 3 sec go back here to sleep.
SLEEP:
    // you need to update before sleeping.
		but_rec.update();
		but_mon.update();
    but_blue.update();
    
		SIM_SCGC6 &= ~SIM_SCGC6_I2S;
		delay(10);
    // returns module that woke processor after waking from low power mode.
    int who = Snooze.hibernate( config_teensy3x );
    Bounce cur_bt = Bounce(who, BUTTON_BOUNCE_TIME_MS);
		
    // indicate the button woke it up, hold led high for as long as the button
    // is held down.
    digitalWrite(LED_BUILTIN, HIGH);
    
    elapsedMillis timeout = 0;
    // bounce needs to call update longer than the debounce time = 8ms,
    // which is set in constructor.
    while (timeout < (BUTTON_BOUNCE_TIME_MS+2)) {
			cur_bt.update();
		}
    
		bool awake = threeSecondHold(cur_bt);
    
    // if not held for 3 seconds go back to sleep
    if (!awake) goto SLEEP;
		
		SIM_SCGC6 |= SIM_SCGC6_I2S;
    
    // the button was held for at least 3 seconds if
    // you get here do some stuff for 7 seconds then
    // go to sleep.
    elapsedMillis time = 0;
    
    while (1) {
        unsigned int t = time;
        Serial.printf("doin stuff for: %i milliseconds\n", t);

        // back to sleep after 7 seconds
        if (time > 7000) {
            Serial.println("sleeping now :)");
            
            // little delay so serial can finish sending
            delay(5);
            
            goto SLEEP;
        }
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
    }
	// bool ret;
  // Read the buttons
  // but_rec.update();
  // but_mon.update();
  // but_blue.update();
	  
  // Button press actions
  // if(but_rec.fallingEdge()) {
    // Serial.print("Record button pressed: rec_state = "); Serial.println(working_state.rec_state);
    // if(working_state.rec_state == RECSTATE_OFF) {
			// working_state.rec_state = RECSTATE_REQ_ON;
    // }
    // else {
			// working_state.rec_state = RECSTATE_REQ_OFF;
    // }
  // }
  // if(but_mon.fallingEdge()) {
    // Serial.print("Monitor button pressed: mon_state = "); Serial.println(working_state.mon_state);
		// if(working_state.mon_state == MONSTATE_OFF) {
			// working_state.mon_state = MONSTATE_REQ_ON;
		// }
		// else {
			// working_state.mon_state = MONSTATE_REQ_OFF;
		// }
	// }
  // if(but_blue.fallingEdge()) {
    // Serial.print("Bluetooth button pressed: BLE = "); Serial.print(working_state.ble_state);
		// Serial.print(", BT = "); Serial.println(working_state.bt_state);
		// if(working_state.ble_state == BLESTATE_IDLE) {
			// working_state.ble_state = BLESTATE_REQ_ADV;
		// }
		// else {
			// working_state.ble_state = BLESTATE_REQ_OFF;
		// }
  // }

  // // REC state actions
	// switch(working_state.rec_state) {
		// case RECSTATE_REQ_ON:
			// startLED(&leds[LED_RECORD], LED_MODE_WAITING);
			// ret = fetchGPS();
			// if(!ret) startLED(&leds[LED_RECORD], LED_MODE_WARNING);
			// rec_path = createSDpath(ret);
			// working_state.rec_state = RECSTATE_ON;
			// startRecording(rec_path);
			// break;
		
		// case RECSTATE_ON:
			// continueRecording();
			// break;
		
		// case RECSTATE_REQ_OFF:
			// stopRecording(rec_path);
			// stopLED(&leds[LED_RECORD]);
			// working_state.rec_state = RECSTATE_OFF;
			// break;
		
		// default:
			// break;
	// }
  // // MON state actions
	// switch(working_state.mon_state) {
		// case MONSTATE_REQ_ON:
			// startLED(&leds[LED_MONITOR], LED_MODE_ON);
			// startMonitoring();
			// working_state.mon_state = MONSTATE_ON;
			// break;
		
		// case MONSTATE_REQ_OFF:
			// stopMonitoring();
			// stopLED(&leds[LED_MONITOR]);
			// working_state.mon_state = MONSTATE_OFF;
			// break;
		
		// default:
			// break;
	// }
  // // BLE state actions
	// switch(working_state.ble_state) {
		// case BLESTATE_REQ_ADV:
			// startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			// bc127PowerOn();
			// delay(1000);
			// bc127AdvStart();
			// working_state.ble_state = BLESTATE_ADV;
			// break;
		
		// case BLESTATE_REQ_CONN:
			// if(working_state.bt_state == BTSTATE_CONNECTED) {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			// }
			// else {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			// }
			// working_state.ble_state = BLESTATE_CONNECTED;
			// break;
		
		// case BLESTATE_CONNECTED:
			// if(working_state.bt_state == BTSTATE_CONNECTED) {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_ON);
			// }
			// break;
		
		// case BLESTATE_REQ_DIS:
			// if(working_state.bt_state == BTSTATE_CONNECTED) {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			// }
			// else {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_WAITING);
			// }
			// sendCmdOut(BCCMD_BLE_ADV_ON);
			// working_state.ble_state = BLESTATE_ADV;
			// break;
		
		// case BLESTATE_REQ_OFF:
			// bc127AdvStop();
			// delay(200);
			// bc127PowerOff();
			// stopLED(&leds[LED_BLUETOOTH]);
			// working_state.ble_state = BLESTATE_IDLE;
			// break;
		
		// default:
		// break;
	// }
  // // BT state actions
	// switch(working_state.bt_state) {
		// case BTSTATE_REQ_CONN:
			// startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_FAST);
			// working_state.bt_state = BTSTATE_CONNECTED;
			// break;
		
		// case BTSTATE_REQ_DIS:
			// if(working_state.ble_state == BLESTATE_CONNECTED) {
				// startLED(&leds[LED_BLUETOOTH], LED_MODE_IDLE_SLOW);
			// }
			// else {
				// bc127PowerOff();
				// stopLED(&leds[LED_BLUETOOTH]);
				// working_state.ble_state = BLESTATE_IDLE;
			// }
			// working_state.bt_state = BTSTATE_IDLE;
			// break;
		
		// default:
			// break;
	// }
	
	// // Serial messaging
  // if (Serial4.available()) {
    // String inMsg = Serial4.readStringUntil('\r');
    // int outMsg = parseSerialIn(inMsg);
    // if(!sendCmdOut(outMsg)) {
      // Serial.println("Sending command error!!");
    // }
  // }	
  // if (Serial.available()) {
    // String manInput = Serial.readStringUntil('\n');
    // int len = manInput.length() - 1;
    // Serial4.print(manInput.substring(0, len)+'\r');
  // }
}

bool threeSecondHold(Bounce bt) {
	// this is the 3 sec button press check
	while (bt.duration() < 3000) {
		// get the current pin state, must have this!
		bt.update();
		
		// check the pin 2 state, if button not
		// pressed before 3 seconds go back to
		// sleep. We read 0 since pin 2 is
		// configured as INPUT_PULLUP.
		if (bt.read() != 0) {
			digitalWrite(LED_BUILTIN, LOW);
			// let go of button before 3 sec up
			return false;
		}
	}
	digitalWrite(LED_BUILTIN, LOW);
	
	// button was held for 3 seconds so now we are awake
	return true;
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

/* initStates(void)
 * ----------------
 */
void initStates(void) {
  working_state.rec_state = RECSTATE_OFF;
  working_state.mon_state = MONSTATE_OFF;
  working_state.bt_state = BTSTATE_IDLE;
  working_state.ble_state = BLESTATE_IDLE;
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
    // mixer.gain(MIXER_CH_REC, 1);
    // working_state.rec_state = true;
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
	// working_state.mon_state = true;
}

/* stopMonitoring(void)
 *
 */
void stopMonitoring(void) {
	mixer.gain(MIXER_CH_REC, 0);
	// working_state.mon_state = false;
}