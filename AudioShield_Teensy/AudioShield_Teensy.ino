/*
  AudioShield firmware for SoundingSoil
  -------------------------------------
  Mixing different code bases:
  - Serial_monitor (ICST)
  - Teensy_recorder (PJRC)
*/
#include "main.h"

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
// GUItool: end automatically generated code
const int                     audioInput = AUDIO_INPUT_LINEIN;

struct wState 								working_state;

void setup() {
  // Initialize both serial ports:
  Serial.begin(9600);															// Serial monitor port
  Serial4.begin(9600);														// BC127 communication port
  gpsPort.begin(9600);														// GPS port

	// Say hello
  Serial.println("AudioShield v1.0");
  Serial.println("----------------");
  
	// Initialize peripheral & variables
	initLEDButtons();
	initAudio();
	initSDcard();
	initWaveHeader();
	initStates();
	bc127Init();
}

void loop() {
	bool ret;
  // Read the buttons
  but_rec.update();
  but_mon.update();
  but_blue.update();
  
  // Button press actions
  if(but_rec.fallingEdge()) {
    Serial.print("Record button pressed: rec_state = "); Serial.println(working_state.rec_state);
    if(working_state.rec_state == RECSTATE_OFF) {
			working_state.rec_state = RECSTATE_REQ_ON;
    }
    else {
			working_state.rec_state = RECSTATE_REQ_OFF;
    }
  }
  if(but_mon.fallingEdge()) {
    Serial.print("Monitor button pressed: mon_state = "); Serial.println(working_state.mon_state);
		if(working_state.mon_state == MONSTATE_OFF) {
			working_state.mon_state = MONSTATE_REQ_ON;
		}
		else {
			working_state.mon_state = MONSTATE_REQ_OFF;
		}
	}
  if(but_blue.fallingEdge()) {
    Serial.print("Bluetooth button pressed: BLE = "); Serial.print(working_state.ble_state);
		Serial.print(", BT = "); Serial.println(working_state.bt_state);
		if(working_state.ble_state == BLESTATE_IDLE) {
			working_state.ble_state = BLESTATE_REQ_ADV;
		}
		else {
			working_state.ble_state = BLESTATE_REQ_OFF;
		}
  }

  // REC state actions
	switch(working_state.rec_state) {
		case RECSTATE_REQ_ON:
			startLED(&leds[LED_RECORD], LED_MODE_WAITING);
			ret = fetchGPS();
			if(!ret) startLED(&leds[LED_RECORD], LED_MODE_WARNING);
			rec_path = createSDpath(ret);
			working_state.rec_state = RECSTATE_ON;
			startRecording(rec_path);
			break;
		
		case RECSTATE_ON:
			continueRecording();
			break;
		
		case RECSTATE_REQ_OFF:
			stopRecording(rec_path);
			stopLED(&leds[LED_RECORD]);
			working_state.rec_state = RECSTATE_OFF;
			break;
		
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