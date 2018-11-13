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

// Audio connections definition
AudioRecordQueue              queueRec1;
AudioPlaySdRaw                playRaw1;
AudioPlaySdWav                playWav1;
AudioControlSGTL5000          sgtl5000_1;
const int                     audioInput = AUDIO_INPUT_LINEIN;

// Buttons definition
#define BUTTON_RECORD         0
#define BUTTON_MONITOR        2
#define BUTTON_BLUETOOTH      1
Bounce                        buttonRecord = Bounce(BUTTON_RECORD, 8);
Bounce                        buttonMonitor = Bounce(BUTTON_MONITOR, 8);
Bounce                        buttonBluetooth = Bounce(BUTTON_BLUETOOTH, 8);

// LED definition
#define LED_RECORD            25
#define LED_MONITOR           27
#define LED_BLUETOOTH         26

// SDcard pins definition (Audio Shield slot)
#define SDCARD_CS_PIN         10
#define SDCARD_MOSI_PIN       7
#define SDCARD_SCK_PIN        14

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

// File where the recorded data is saved
File                          frec;

// Teensy's working state: 0 -> stopped, 2 -> recording, 4 -> monitoring
enum wState {
  STATE_IDLE = 0,
  STATE_BLE_ADV,
  STATE_BT_INQ,
  STATE_RECORDING,
  STATE_MONITORING,
  STATE_MAX
};
enum wState                   workingState = STATE_IDLE;

void setup() {
  // Initialize both serial ports:
  Serial.begin(9600); // Serial monitor port
  Serial4.begin(9600); // BC127 communication port
  Serial2.begin(9600); // GPS port

  // Configure the pushbutton pins
  pinMode(BUTTON_RECORD, INPUT_PULLUP);
  pinMode(BUTTON_MONITOR, INPUT_PULLUP);
  pinMode(BUTTON_BLUETOOTH, INPUT_PULLUP);

  // Configure the output pins
  pinMode(LED_RECORD, OUTPUT);
  pinMode(LED_MONITOR, OUTPUT);
  pinMode(LED_BLUETOOTH, OUTPUT);

  // Memory buffer for the record queue
  AudioMemory(60);

  // Enable the audio shield, select input, enable output
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(audioInput);
  sgtl5000_1.volume(0.5);

  // Initialize the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if(!(SD.begin(SDCARD_CS_PIN))) {
    while(1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  // Reset BC127 module
  sendCmdOut(BCCMD_GEN_RESET);
}

void loop() {
  buttonRecord.update();
  buttonMonitor.update();
  buttonBluetooth.update();
  if(buttonRecord.fallingEdge()) {
    Serial.println("Record button pressed");
    if(workingState == STATE_IDLE) startRecording();
    if(workingState == STATE_RECORDING) stopRecording();
  }
//  if(buttonMonitor.fallingEdge()) {
//    Serial.println("Play button pressed");
//    if(workingState == STATE_MONITORING) stopPlaying();
//    if(workingState == STATE_IDLE) startPlaying();
//  }
//  if(buttonBluetooth.fallingEdge()) {
//    Serial.println("Bluetooth button pressed");
//    if(workingState == STATE_MONITORING) stopBluetooth();
//    else startBluetooth();
//  }
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

void startRecording() {
  Serial.println("Starting recording...");
  if(SD.exists("RECORD.RAW")) {
    SD.remove("RECORD.RAW");
  }
  frec = SD.open("RECORD.RAW", FILE_WRITE);
  if(frec) {
    queueRec1.begin();
    workingState = STATE_RECORDING;
  }
}

void continueRecording() {
  if(queueRec1.available() >= 2) {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queueRec1.readBuffer(), 256);
    queueRec1.freeBuffer();
    memcpy(buffer+256, queueRec1.readBuffer(), 256);
    queueRec1.freeBuffer();
    // write all 512 bytes to the SD card
//    elapsedMicros usec = 0;
    frec.write(buffer, 512);
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print("SD write, us=");
    //Serial.println(usec);
  }
}

void stopRecording() {
  Serial.println("Stop recording");
  queueRec1.end();
  if(workingState == STATE_RECORDING) {
    while(queueRec1.available() > 0) {
      frec.write((byte*)queueRec1.readBuffer(), 256);
      queueRec1.freeBuffer();
    }
    frec.close();
  }
  workingState = STATE_IDLE;
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
    return BCCMD_BLE_ADVERTISE;
  }
  else if(part1.equalsIgnoreCase("OK\n")) {
    Serial.println("OK");
  }
  else if(part1.equalsIgnoreCase("ERROR\n")) {
    Serial.println("ERROR");
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
