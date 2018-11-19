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
#include "waveHeader.h"

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

// SDcard pins definition (Audio Shield slot)
#define SDCARD_CS_PIN         10
#define SDCARD_MOSI_PIN       7
#define SDCARD_SCK_PIN        14

// Total amount of recorded bytes
unsigned long                 totRecBytes = 0;
String                        recPath = "";

// Bluetooth audio devices definition
struct BTdev {
  String                      address;
  String                      capabilities;
  unsigned int                strength;
};
#define DEVLIST_MAXLEN        6
struct BTdev                  devList[DEVLIST_MAXLEN];
unsigned int                  foundDevices;
String                        peerAddress;

// Serial command messages enumeration
enum outputMsg {
  BCCMD_NOTHING = 0,
  BCCMD_GEN_RESET,
  BCCMD_BLE_ADVERTISE,
  BCCMD_BLE_SEND,
  BCCMD_BT_INQUIRY,
  BCCMD_BT_LIST,
  BCCMD_BT_OPEN,
  BCCMD_BT_STATUS,
  BCCMD_BT_VOLUP,
  BCCMD_BT_VOLDOWN,
  BCCMD_BT_STARTMON,
  BCCMD_BT_STOPMON,
  BCCMD_BT_STARTREC,
  BCCMD_BT_STOPREC,
  ANNOT_INQ_STATE,
  MAX_OUTPUTS
};

// SD card file variables
File                          frec;

// GPS tag definition (in 'gpsRoutines.h')
extern struct gps_rmc_tag     gps_tag;

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

  // Memory buffer for the record queue
  AudioMemory(60);

  // Enable the audio shield, select input, enable output
  sgtl5000.enable();
  sgtl5000.inputSelect(audioInput);
  sgtl5000.volume(0.5);
  mixer.gain(MIXER_CH_REC, 0);
  mixer.gain(MIXER_CH_SDC, 0);

  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if(!(SD.begin(SDCARD_CS_PIN))) {
    while(1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  // Initialize the wave header
  strncpy(wavehd.riff,"RIFF",4);
  strncpy(wavehd.wave,"WAVE",4);
  strncpy(wavehd.fmt,"fmt ",4);
  // <- missing here the filesize
  wavehd.chunk_size = WAVE_FMT_CHUNK_SIZE;
  wavehd.format_tag = WAVE_FORMAT_PCM;
  wavehd.num_chans = WAVE_NUM_CHANNELS;
  wavehd.srate = WAVE_SAMPLING_RATE;
  wavehd.bytes_per_sec = WAVE_BYTES_PER_SEC;
  wavehd.bytes_per_samp = WAVE_BYTES_PER_SAMP;
  wavehd.bits_per_samp = WAVE_BITS_PER_SAMP;
  strncpy(wavehd.data,"data",4);
  // <- missing here the data size

  // Initializing states
  workingState.rec_state = false;
  workingState.mon_state = false;
  workingState.bt_state = BTSTATE_IDLE;
  workingState.ble_state = BLESTATE_IDLE;

  // Reset BC127 module
//  sendCmdOut(BCCMD_GEN_RESET);
}

void loop() {
  // Read the buttons
  buttonRecord.update();
  buttonMonitor.update();
  buttonBluetooth.update();
  // Button press actions
  if(buttonRecord.fallingEdge()) {
    Serial.print("Record button pressed: rec_state = "); Serial.println(workingState.rec_state);
    if(workingState.rec_state) {
      stopRecording(recPath);
    }
    else {
      fetchGPS();
      recPath = createSDpath();
      startRecording(recPath);
    }
  }
  if(buttonMonitor.fallingEdge()) {
    Serial.print("Play button pressed: mon_state = "); Serial.println(workingState.mon_state);
//    if(workingState.mon_state) stopMonitoring();
//    else startMonitoring();
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

String createSDpath() {
  Serial.println("Creating new folder/file on the SD card");
  String dirName = "";
  String fileName = "";
  String path = "/";
  char buf[12];
  sprintf(buf, "%02d", gps_tag.date.year);
  dirName.concat(buf);
  sprintf(buf, "%02d", gps_tag.date.month);
  dirName.concat(buf);
  sprintf(buf, "%02d", gps_tag.date.day);
  dirName.concat(buf);

  sprintf(buf, "%02d", gps_tag.time.h);
  fileName.concat(buf);
  sprintf(buf, "%02d", gps_tag.time.min);
  fileName.concat(buf);
  sprintf(buf, "%02d", gps_tag.time.sec);
  fileName.concat(buf);
  
  path.concat(dirName);
  path.concat("/");
  path.concat(fileName);
  path.concat(".wav");

  if(SD.exists(dirName)) {
    if(SD.exists(path)) {
      SD.remove(path);
    }
  }
  else {
    SD.mkdir(dirName);
  }
  return path;
}

void startRecording(String path) {
  Serial.print("Start recording: rec_state = "); Serial.println(workingState.rec_state);
  
  frec = SD.open(path, FILE_WRITE);
  if(frec) {
    queueSdc.begin();
    totRecBytes = 0;
    mixer.gain(MIXER_CH_REC, 1);
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
  Serial.print("Stop recording: rec_state = "); Serial.println(workingState.rec_state);
  queueSdc.end();
  if(workingState.rec_state) {
    while(queueSdc.available() > 0) {
      frec.write((byte*)queueSdc.readBuffer(), 256);
      queueSdc.freeBuffer();
      totRecBytes += 256;
    }
    frec.close();

    writeWaveHeader(path);
  }
  mixer.gain(MIXER_CH_REC, 0);
  workingState.rec_state = false;
}

void writeWaveHeader(String path) {
  wavehd.dlength = totRecBytes;
  wavehd.flength = totRecBytes + 36;
  
  frec = SD.open(path, O_WRITE);
  frec.seek(0);
  frec.write((byte*)&wavehd, 44);
  frec.close();
}

int parseSerialIn(String input) {
  // ================================================================
  // Slicing input string.
  // Currently 3 usefull schemes:
  // - 2 words   --> e.g. "OPEN_OK AVRCP"
  // - RECV BLE  --> commands received from phone over BLE
  // - other     --> 3+ words inputs like "INQUIRY xxxx 240404 -54dB"
  // ================================================================
  String part1, part2;
  int slice1 = input.indexOf(" ");
  int slice2 = input.indexOf(" ", slice1+1);
  // 2-words notifications
  if(slice2 == -1) {
    part1 = input.substring(0, slice1);
    part2 = input.substring(slice1 + 1);
  }
  else {
    part1 = input.substring(0, slice1);
    part2 = input.substring(slice1 + 1, slice2);
    // "RECV BLE" case
    if(part1.equals("RECV") && part2.equals("BLE")) {
      part1.concat(" BLE");
      part2 = input.substring(slice2+1);
    }
    // other cases
    else {
      part2 = input.substring(slice1+1);
    }
  }

  // ================================================================
  // Parsing & interpreting input
  // ================================================================
  if(part1.equalsIgnoreCase("Ready\n")) {
//    return BCCMD_BLE_ADVERTISE;
  }
  else if(part1.equalsIgnoreCase("OK\n")) {
//    Serial.println("OK");
  }
  else if(part1.equalsIgnoreCase("ERROR\n")) {
    Serial.println("!!ERROR!!");
  }
  // Notifications from BC127!!
  else if(part1.equalsIgnoreCase("OPEN_OK")) {
    if(part2.equalsIgnoreCase("BLE\n")) {
      Serial.println("BLE connection done!");
    }
    else if(part2.equalsIgnoreCase("AVRCP\n")) {
      Serial.println("AVRCP protocol open!");
    }
  }
  else if(part1.equalsIgnoreCase("INQUIRY")) {
    int len = part2.length()-1;
    String addr = part2.substring(0,12);
    String caps = part2.substring(13, 19);
    unsigned int stren = part2.substring(len-4, len-2).toInt();
    populateDevlist(addr, caps, stren);
    return ANNOT_INQ_STATE;
  }
  // Commands from Android
  else if(part1.equalsIgnoreCase("RECV BLE")) {
    if(part2.equalsIgnoreCase("inq\n")) {
      Serial.println("Received BT inquiry command");
      return BCCMD_BT_INQUIRY;
    }
    else if(part2.equalsIgnoreCase("start_mon\n")) {
      return BCCMD_BT_STARTMON;
    }
    else if(part2.equalsIgnoreCase("stop_mon\n")) {
      return BCCMD_BT_STOPMON;
    }
    else if(part2.equalsIgnoreCase("start_rec\n")) {
      return BCCMD_BT_STARTREC;
    }
    else if(part2.equalsIgnoreCase("stop_rec\n")) {
      return BCCMD_BT_STOPREC;
    }
    else if(part2.equalsIgnoreCase("vol+\n")) {
      return BCCMD_BT_VOLUP;
    }
    else if(part2.equalsIgnoreCase("vol-\n")) {
      return BCCMD_BT_VOLDOWN;
    }
    else if(part2.substring(0, 4).equalsIgnoreCase("conn")) {
      int len = part2.substring(5).length()-1;
      peerAddress = part2.substring(5, (5+len));
      Serial.print("peerAddress: "); Serial.println(peerAddress);
      return BCCMD_BT_OPEN;
    }
  }
  else {
    Serial.print(input);
  }
  
  return BCCMD_NOTHING;
}

bool sendCmdOut(int msg) {
  bool retVal = true;
  String devString = "";
  String cmdLine = "";
  
  switch(msg) {
    case BCCMD_NOTHING:
    break;

    case BCCMD_GEN_RESET:
    Serial.println("Resetting BC127...");
    cmdLine = "RESET\r";
    break;
    
    case BCCMD_BLE_ADVERTISE:
    Serial.println("Starting BLE advertising");
    cmdLine = "ADVERTISING ON\r";
    break;
    
    case BCCMD_BT_INQUIRY:
    Serial.println("Start inquiry");
    for(int i = 0; i < DEVLIST_MAXLEN; i++) {
      devList[i].address = "";
      devList[i].capabilities = "";
      devList[i].strength = 0;
    }
    foundDevices = 0;
    peerAddress = "";
    devString = "";
    cmdLine = "INQUIRY 10\r";
    break;

    case BCCMD_BT_OPEN:
    Serial.print("Opening BT connection @"); Serial.println(peerAddress);
    cmdLine = "OPEN " + peerAddress + " A2DP\r";
    break;

    case BCCMD_BT_STARTMON:
    cmdLine = "MUSIC PLAY\r";
    break;

    case BCCMD_BT_STOPMON:
    cmdLine = "MUSIC PAUSE\r";
    break;

    case BCCMD_BT_STARTREC:
    break;

    case BCCMD_BT_STOPREC:
    break;

    case BCCMD_BT_VOLUP:
    cmdLine = "VOLUME UP\r";
    break;

    case BCCMD_BT_VOLDOWN:
    cmdLine = "VOLUME DOWN\r";
    break;
    
    case ANNOT_INQ_STATE:
    for(unsigned int i = 0; i < foundDevices; i++) {
      devString = devList[i].address;
      devString.concat(" ");
      devString.concat(devList[i].strength);
      cmdLine = "SEND BLE " + devString + "\r";
      Serial4.print(cmdLine);
    }
    cmdLine = "";
    break;

    default:
    retVal = false;
    break;
  }

  Serial4.print(cmdLine);
  
  return retVal;
}

void populateDevlist(String addr, String caps, unsigned int stren) {
  bool found = false;
  int lastPos = 0;
  for(int i = 0; i < DEVLIST_MAXLEN; i++) {
    if(addr.equals(devList[i].address)) {
      devList[i].strength = stren;
      found = true;
      break;
    }
    else if(devList[i].address.equals("")) {
      lastPos = i;
      break;
    }
  }

  if(!found) {
    devList[lastPos].address = addr;
    devList[lastPos].capabilities = caps;
    devList[lastPos].strength = stren;
    foundDevices += 1;
  }
  
  Serial.println("Device list:");
  for(unsigned int i = 0; i < foundDevices; i++) {
    Serial.print(i); Serial.print(": ");
    Serial.print(devList[i].address); Serial.print(", ");
    Serial.print(devList[i].capabilities); Serial.print(", ");
    Serial.println(devList[i].strength);
  }
}
