/*
  AudioShield firmware for SoundingSoil
  -------------------------------------
  Mixing different code bases:
  - Serial_monitor (ICST)
  - Teensy_recorder (PJRC)
*/

#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "gpsRoutines.h"
#include "SDutils.h"
#include "BC127.h"
#include "IOutils.h"

// Audio connections definition
// GUItool: begin automatically generated code
AudioInputI2S                 i2sRec;         //xy=172,125
AudioRecordQueue              queueSdc;       //xy=512,120
AudioAnalyzePeak              peak;          //xy=512,169
AudioPlaySdWav                playWav;        //xy=172,225
AudioMixer4                   mixer;         //xy=350,225
AudioOutputI2S                i2sPlayMon;     //xy=512,228
AudioConnection               patchCord1(playWav, 0, mixer, 1);
AudioConnection               patchCord2(i2sRec, 0, queueSdc, 0);
AudioConnection               patchCord3(i2sRec, 0, peak, 0);
AudioConnection               patchCord4(i2sRec, 0, mixer, 0);
AudioConnection               patchCord5(mixer, 0, i2sPlayMon, 0);
AudioConnection               patchCord6(mixer, 0, i2sPlayMon, 1);
AudioControlSGTL5000          sgtl5000;       //xy=172,323
// GUItool: end automatically generated code
const int                     audioInput = AUDIO_INPUT_LINEIN;

// SD card file handle
// File                          frec;

// Buttons definition
#define BUTTON_RECORD         24
#define BUTTON_MONITOR        25
#define BUTTON_BLUETOOTH      29
Bounce                        buttonRecord = Bounce(BUTTON_RECORD, 8);
Bounce                        buttonMonitor = Bounce(BUTTON_MONITOR, 8);
Bounce                        buttonBluetooth = Bounce(BUTTON_BLUETOOTH, 8);

// LED definition
#define LED_RECORD            26
#define LED_MONITOR           27
#define LED_BLUETOOTH         28
#define LED_OFF               HIGH
#define LED_ON                LOW







// Working states
enum btState {
  BTSTATE_IDLE,
  BTSTATE_INQ,
  BTSTATE_CONN,
  BTSTATE_PLAY
} bt_state;

enum bleState {
  BLESTATE_IDLE,
  BLESTATE_ADV,
  BLESTATE_CONN
} ble_state;

volatile struct wState {
  bool rec_state;
  bool mon_state;
  enum btState bt_state;
  enum bleState ble_state;
} workingState;

#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1

void setup() {
  // Initialize both serial ports:
  Serial.begin(9600); // Serial monitor port
  Serial4.begin(9600); // BC127 communication port
  Serial1.begin(9600); // GPS port

  Serial.println("AudioShield v1.0");
  Serial.println("----------------");
  
  // Configure the pushbutton pins
  pinMode(BUTTON_RECORD, INPUT_PULLUP);
  pinMode(BUTTON_MONITOR, INPUT_PULLUP);
  pinMode(BUTTON_BLUETOOTH, INPUT_PULLUP);

  // Configure the output pins
  pinMode(LED_RECORD, OUTPUT);
  digitalWrite(LED_RECORD, LED_ON);
  pinMode(LED_MONITOR, OUTPUT);
  digitalWrite(LED_MONITOR, LED_ON);
  pinMode(LED_BLUETOOTH, OUTPUT);
  digitalWrite(LED_BLUETOOTH, LED_ON);

	initAudio();

	initSDcard();

	initWaveHeader();

  // Initializing states
  workingState.rec_state = false;
  workingState.mon_state = false;
  workingState.bt_state = BTSTATE_IDLE;
  workingState.ble_state = BLESTATE_IDLE;
	
	initBC127();
}

void loop() {
  // Read the buttons
  buttonRecord.update();
  buttonMonitor.update();
  buttonBluetooth.update();
  
  // Button press actions
  if(buttonRecord.fallingEdge()) {
    Serial.print("Record button pressed: rec_state = "); Serial.println(workingState.rec_state);
    if(!workingState.rec_state) {
			struct gps_rmc_tag rmc_tag = fetchGPS();
      recPath = createSDpath(rmc_tag);
      startRecording(recPath);
    }
    else {
      stopRecording(recPath);
    }
  }
  if(buttonMonitor.fallingEdge()) {
    Serial.print("Play button pressed: mon_state = "); Serial.println(workingState.mon_state);
	if(workingState.mon_state) stopMonitoring();
	else startMonitoring();
  }
  if(buttonBluetooth.fallingEdge()) {
    Serial.print("Bluetooth button pressed: ble_state = "); Serial.println(workingState.ble_state);
  }
  // State actions
  if(workingState.rec_state) {
    continueRecording();
  }
  if(workingState.mon_state) {
    
  }
  
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

void startRecording(String path) {
  Serial.println("Start recording");
  
  frec = SD.open(path, FILE_WRITE);
  if(frec) {
    queueSdc.begin();
    totRecBytes = 0;
    // mixer.gain(MIXER_CH_REC, 1);
    workingState.rec_state = true;
  }
  else {
    Serial.println("file opening error");
    while(1);
  }
}

void continueRecording() {
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
    totRecBytes += 512;
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

void stopRecording(String path) {
  Serial.println("Stop recording:");
  queueSdc.end();
  if(workingState.rec_state) {
    while(queueSdc.available() > 0) {
      frec.write((byte*)queueSdc.readBuffer(), 256);
      queueSdc.freeBuffer();
      totRecBytes += 256;
    }
    frec.close();

    writeWaveHeader(path, totRecBytes);
  }
  workingState.rec_state = false;
}

void startMonitoring() {
	mixer.gain(MIXER_CH_REC, 1);
	workingState.mon_state = true;
}

void stopMonitoring() {
	mixer.gain(MIXER_CH_REC, 0);
	workingState.mon_state = false;
}