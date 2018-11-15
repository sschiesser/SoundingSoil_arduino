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
AudioInputI2S                 i2sRec;
AudioAnalyzePeak              peak1;
AudioRecordQueue              queueRec;
AudioPlaySdRaw                playRaw;
AudioOutputI2S                i2sPlayRaw;
AudioConnection               patchCord1(i2sRec, 0, queueRec, 0);
AudioConnection               patchCord2(i2sRec, 0, peak1, 0);
AudioConnection               patchCord3(playRaw, 0, i2sPlayRaw, 0);
AudioConnection               patchCord4(playRaw, 0, i2sPlayRaw, 1);
AudioControlSGTL5000          sgtl5000;     //xy=265,212
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

void setup() {
  // Initialize both serial ports:
  Serial.begin(9600); // Serial monitor port
  Serial4.begin(9600); // BC127 communication port
  Serial1.begin(9600); // GPS port

  // Configure the pushbutton pins
  pinMode(BUTTON_RECORD, INPUT_PULLUP);
  pinMode(BUTTON_MONITOR, INPUT_PULLUP);
  pinMode(BUTTON_BLUETOOTH, INPUT_PULLUP);

  // Configure the output pins
  pinMode(LED_RECORD, OUTPUT);
  digitalWrite(LED_RECORD, LED_OFF);
  pinMode(LED_MONITOR, OUTPUT);
  digitalWrite(LED_MONITOR, LED_OFF);
  pinMode(LED_BLUETOOTH, OUTPUT);
  digitalWrite(LED_BLUETOOTH, LED_OFF);

  // Memory buffer for the record queue
  AudioMemory(60);

  // Enable the audio shield, select input, enable output
  sgtl5000.enable();
  sgtl5000.inputSelect(audioInput);
  sgtl5000.volume(0.8);

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
//    frec.write((byte*)&wavehd, 44);
    queueRec.begin();
    totRecBytes = 0;
    workingState.rec_state = true;
  }
  else {
    Serial.println("file opening error");
    while(1);
  }
}

void continueRecording() {
  if(queueRec.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queueRec.readBuffer(), 256);
    queueRec.freeBuffer();
    memcpy(buffer+256, queueRec.readBuffer(), 256);
    queueRec.freeBuffer();
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
    // wear leveling.  The queueRec object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
//    Serial.print("SD write, us=");
//    Serial.println(usec);
  }
}

void stopRecording(String path) {
  Serial.print("Stop recording: rec_state = "); Serial.println(workingState.rec_state);
  queueRec.end();
  if(workingState.rec_state) {
    while(queueRec.available() > 0) {
      frec.write((byte*)queueRec.readBuffer(), 256);
      queueRec.freeBuffer();
      totRecBytes += 256;
    }
    frec.close();

    writeWaveHeader(path);
  }
  workingState.rec_state = false;
}

void writeWaveHeader(String path) {
//  unsigned long dl = totRecBytes;
//  unsigned long fl = dl + 36;
//  byte b1, b2, b3, b4;
//  b1 = dl & 0xff;
//  b2 = (dl >> 8) & 0xff;
//  b3 = (dl >> 16) & 0xff;
//  b4 = (dl >> 24) & 0xff;
//  wavehd.dlength = ((unsigned long)b1 << 24) & 0xff000000;
//  wavehd.dlength |= ((unsigned long)b2 << 16) & 0xff0000;
//  wavehd.dlength |= ((unsigned long)b3 << 8) & 0xff00;
//  wavehd.dlength |= ((unsigned long)b4) & 0xff;
//  Serial.print("dl = 0x"); Serial.println(dl, HEX);
//  Serial.print("dlength = 0x"); Serial.println(wavehd.dlength, HEX);
//
//  b1 = fl & 0xff;
//  b2 = (fl >> 8) & 0xff;
//  b3 = (fl >> 16) & 0xff;
//  b4 = (fl >> 24) & 0xff;
//  wavehd.flength = ((unsigned long)b1 << 24) & 0xff000000;
//  wavehd.flength |= ((unsigned long)b2 << 16) & 0xff0000;
//  wavehd.flength |= ((unsigned long)b3 << 8) & 0xff00;
//  wavehd.flength |= ((unsigned long)b4) & 0xff;
//  Serial.print("fl = 0x"); Serial.println(fl, HEX);
//  Serial.print("flength = 0x"); Serial.println(wavehd.flength, HEX);

  wavehd.dlength = totRecBytes;
  wavehd.flength = totRecBytes + 36;
  
  frec = SD.open(path, O_WRITE);
  frec.seek(0);
  frec.write((byte*)&wavehd, 44);
  frec.close();

//  byte buf[4];
//  byte buff[44];
//  char printbuff[256];
//  wavehd.dlength = totRecBytes;
//  wavehd.flength = wavehd.dlength + 36;
//  strncpy(buff, wavehd.riff, 4);
//  frec.seek(0);
//  frec.write("RIFF");
//  b1 = wavehd.flength & 0xff;
//  b2 = (wavehd.flength >> 8) & 0xff;
//  b3 = (wavehd.flength >> 16) & 0xff;
//  b4 = (wavehd.flength >> 24) & 0xff;
//  frec.write(b1); frec.write(b2); frec.write(b3); frec.write(b4);
//  frec.write("WAVE");
//  frec.write("fmt ");
//  b1 = wavehd.chunk_size & 0xff;
//  b2 = (wavehd.chunk_size >> 8) & 0xff;
//  b3 = (wavehd.chunk_size >> 16) & 0xff;
//  b4 = (wavehd.chunk_size >> 24) & 0xff;
//  frec.write(b1); frec.write(b2); frec.write(b3); frec.write(b4);
//  b1 = wavehd.format_tag & 0xff;
//  b2 = (wavehd.format_tag >> 8) & 0xff;
//  frec.write(b1); frec.write(b2);
//  b1 = wavehd.num_chans & 0xff;
//  b2 = (wavehd.num_chans >> 8) & 0xff;
//  frec.write(b1); frec.write(b2);
//  b1 = wavehd.srate & 0xff;
//  b2 = (wavehd.srate >> 8) & 0xff;
//  b3 = (wavehd.srate >> 16) & 0xff;
//  b4 = (wavehd.srate >> 24) & 0xff;
//  frec.write(b1); frec.write(b2); frec.write(b3); frec.write(b4);
//  b1 = wavehd.bytes_per_sec & 0xff;
//  b2 = (wavehd.bytes_per_sec >> 8) & 0xff;
//  b3 = (wavehd.bytes_per_sec >> 16) & 0xff;
//  b4 = (wavehd.bytes_per_sec >> 24) & 0xff;
//  frec.write(b1); frec.write(b2); frec.write(b3); frec.write(b4);
//  b1 = wavehd.bytes_per_samp & 0xff;
//  b2 = (wavehd.bytes_per_samp >> 8) & 0xff;
//  frec.write(b1); frec.write(b2);
//  b1 = wavehd.bits_per_samp & 0xff;
//  b2 = (wavehd.bits_per_samp >> 8) & 0xff;
//  frec.write(b1); frec.write(b2);
//  frec.write("data");
//  b1 = wavehd.dlength & 0xff;
//  b2 = (wavehd.dlength >> 8) & 0xff;
//  b3 = (wavehd.dlength >> 16) & 0xff;
//  b4 = (wavehd.dlength >> 24) & 0xff;
//  frec.write(b1); frec.write(b2); frec.write(b3); frec.write(b4);
//  frec = SD.open(path, FILE_WRITE);
//  frec.write((byte*)&wavehd, 44);
//  if(frec.seek(4)) {
//    Serial.print("Current position: "); Serial.println(frec.position());
//    buf[0] = 0x10; //wavehd.flength & 0xff;
//    Serial.println(buf[0], HEX);
//    buf[1] = 0x13; //(wavehd.flength >> 8) & 0xff;
//    Serial.println(buf[1], HEX);
//    buf[2] = 0x33;//(wavehd.flength >> 16) & 0xff;
//    Serial.println(buf[2], HEX);
//    buf[3] = 0x63;//(wavehd.flength >> 24) & 0xff;
//    Serial.println(buf[3], HEX);
//    frec.write(buf, 4);
//  }
//  else {
//    Serial.println("Seek error!");
//  }
//  if(frec.seek(40)) {
//    buf[0] = wavehd.dlength & 0xff;
//    buf[1] = (wavehd.dlength >> 8) & 0xff;
//    buf[2] = (wavehd.dlength >> 16) & 0xff;
//    buf[3] = (wavehd.dlength >> 24) & 0xff;
//    frec.write(buf, 4);
//  }
//  else {
//    Serial.println("Seek error!");
//  }
//  frec.close();
//  frec.seek(0);
//  frec.read(buff, 52);
//  frec.close();
//  for(int i = 0; i < 52; i++) {
//    Serial.println(buff[i], HEX);
//  }
//  sprintf(printbuff, "%s", wavehd.riff);
//  sprintf(&printbuff[4], "%08x", (unsigned int)wavehd.flength);
//  Serial.println(printbuff);
//  Serial.println("Wave header:");
//  Serial.println(wavehd.riff);
//  Serial.print("flength = "); Serial.println(wavehd.flength);
//  Serial.println(wavehd.wave);
//  Serial.println(wavehd.fmt);
//  Serial.print("chunk size = "); Serial.println(wavehd.chunk_size);
//  Serial.print("format = "); Serial.println(wavehd.format_tag);
//  Serial.print("num chans = "); Serial.println(wavehd.num_chans);
//  Serial.print("sampling rate = "); Serial.println(wavehd.srate);
//  Serial.print("byte/sec = "); Serial.println(wavehd.bytes_per_sec);
//  Serial.print("byte/sample = "); Serial.println(wavehd.bytes_per_samp);
//  Serial.print("bit/sample = "); Serial.println(wavehd.bits_per_samp);
//  Serial.println(wavehd.data);
//  Serial.print("dlength = "); Serial.println(wavehd.dlength);
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
