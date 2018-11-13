/*
  Multiple Serial test

  Receives from the main serial port, sends to the others.
  Receives from serial port 1, sends to the main serial (Serial 0).

  This example works only with boards with more than one serial like Arduino Mega, Due, Zero etc.

  The circuit:
  - any serial device attached to Serial port 1
  - Serial Monitor open on Serial port 0

  created 30 Dec 2008
  modified 20 May 2012
  by Tom Igoe & Jed Roach
  modified 27 Nov 2015
  by Arturo Guadalupi

  This example code is in the public domain.
*/

//#include "gpsRoutines.h"
//
//bool musicPlaying = false;
//String inCmd, bcResp;

#define DEVLIST_MAXLEN 4
struct BTdev {
  String address;
  String capabilities;
  unsigned int strength;
};

struct BTdev devList[DEVLIST_MAXLEN];
unsigned int foundDevices;
unsigned int devCounter;
String peerAddress;

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

enum inputMsg {
  ANCMD_BT_INQUIRY,
  ANCMD_BT_STARTMON,
  ANCMD_BT_STOPMON,
  ANCMD_BT_STARTREC,
  ANCMD_BT_STOPREC,
  ANCMD_BT_SEND_DT,
  MAX_INPUTS
};

void setup() {
  // initialize both serial ports:
  Serial.begin(9600); // Serial monitor port
  Serial4.begin(9600); // BC127 communication port
//  Serial2.begin(9600); // GPS communication port
//  Serial2.setTimeout(100);

  delay(200);

  sendOutput(BCCMD_GEN_RESET);
}

void loop() {
  // read from port 4, send to port 0:
  if (Serial4.available()) {
    String inMsg = Serial4.readStringUntil('\r');
    int outMsg = parseInput(inMsg);
    if(!sendOutput(outMsg)) {
      Serial.println("Sending command error!!");
    }
//    if(bcResp == "OPEN_OK AVRCP\n") {
//      Serial4.write("MUSIC PLAY\r");
//      musicPlaying = true;
//    }
//    else if(bcResp == "RECV BLE +\n") {
//      Serial4.write("VOLUME UP\r");
//    }
//    else if(bcResp == "RECV BLE -\n") {
//      Serial4.write("VOLUME DOWN\r");
//    }
//    else if((bcResp == "RECV BLE s\n") || (bcResp == "RECV BLE S\n")) {
//      if(!musicPlaying) {
//        Serial4.write("MUSIC PLAY\r");
//        musicPlaying = true;
//      }
//    }
//    else if((bcResp == "RECV BLE p\n") || (bcResp == "RECV BLE P\n")) {
//      if(musicPlaying) {
//        Serial4.write("MUSIC STOP\r");
//        musicPlaying = false;
//      }
//    }
//    else if((bcResp == "RECV BLE inq\n") || (bcResp == "RECT BLE INQ\n")) {
//      Serial4.write("INQUIRY 10\r");
//    }
//    else if(bcResp.substring(0,7) == "INQUIRY") {
//      String toSend = bcResp.substring(8, 19);
//      Serial4.write("SEND BLE ");
//      Serial4.print(toSend + '\r');
//    }
//    else {
//      Serial.print(bcResp.substring(0, 7)+'\n');
//    }
  }
//
  // read from port 0, send to port 4:
  if (Serial.available()) {
    String manInput = Serial.readStringUntil('\n');
    int len = manInput.length() - 1;
    Serial4.print(manInput.substring(0, len)+'\r');
//    if(inCmd == "+") {
//      Serial4.write("VOLUME UP\r");
//    }
//    else if(inCmd == "-") {
//      Serial4.write("VOLUME DOWN\r");
//    }
//    else if((inCmd == "s") || (inCmd == "S")) {
//      if(musicPlaying) {
//        Serial4.write("MUSIC STOP\r");
//        musicPlaying = false;
//      }
//    }
//    else if((inCmd == "p") || (inCmd == "P")) {
//      if(!musicPlaying) {
//        Serial4.write("MUSIC PLAY\r");
//        musicPlaying = true;
//      }
//    }
//    else if((inCmd == "G") || (inCmd == "g")) {
//      fetchGPS();
//    }
//    else {
//      Serial4.print(inCmd+"\r");
//    }
  }
}

int parseInput(String input)
{
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
    devCounter = 0;
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

bool sendOutput(int msg)
{
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
  for(int i = 0; i < foundDevices; i++) {
    Serial.print(i); Serial.print(": ");
    Serial.print(devList[i].address); Serial.print(", ");
    Serial.print(devList[i].capabilities); Serial.print(", ");
    Serial.println(devList[i].strength);
  }
}
