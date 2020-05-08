/*
 * BC127
 *
 * Miscellaneous functions to communicate with BC127.
 *
 */

#include "BC127.h"

// BT device informations for connection
struct btDev {
  String address;
  String name;
  String capabilities;
  unsigned int strength;
};
struct btDev dev_list[DEVLIST_MAXLEN];

// Amount of found BT devices on inquiry
unsigned int found_dev;
// Address of the connected BT device
String BT_peer_address;
// Name of the connected BT device
String BT_peer_name;
// ID's (A2DP&AVRCP) of the established BT connection
int BT_id_a2dp;
int BT_id_avrcp;
// ID of the established BLE connection
int BLE_conn_id;
// Flag indicating that the BC127 device is ready
bool BC127_ready = false;
String notif, param1, param2, param3, param4, param5, param6, param7, param8,
    param9, trash;

/* initBc127(void)
 * ---------------
 * Initialize BC127 by sending a reset command
 * IN:	- none
 * OUT:	- none
 */
void initBc127(void) {
  pinMode(BC127_RST_PIN, OUTPUT);
  digitalWrite(BC127_RST_PIN, HIGH);
  bc127Reset();
  BC127_ready = false;
  while (!BC127_ready) {
    if (BLUEPORT.available()) {
      String inMsg = BLUEPORT.readStringUntil('\r');
      parseSerialIn(inMsg);
    }
  };
  bc127BlueOff();
}

/* bc127BlueOn(void)
 * -----------------
 * Switch on the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOn(void) { sendCmdOut(BCCMD_BLUE_ON); }

/* bc1267BlueOff(void)
 * -------------------
 * Switch off the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOff(void) { sendCmdOut(BCCMD_BLUE_OFF); }

/* bc127Reset(void)
 * ----------------
 * Reset the BC127 device
 * IN:	- none
 * OUT:	- none
 */
void bc127Reset(void) {
  Alarm.delay(10);
  digitalWrite(BC127_RST_PIN, LOW);
  Alarm.delay(30);
  digitalWrite(BC127_RST_PIN, HIGH);
}

/* bc127AdvStart(void)
 * -------------------
 * Start advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStart(void) { sendCmdOut(BCCMD_ADV_ON); }

/* bc127AdvStop(void)
 * ------------------
 * Stop advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStop(void) { sendCmdOut(BCCMD_ADV_OFF); }

/* parseSerialIn(String)
 * ---------------------
 * Parse received string and read information along
 * minimal keywords.
 * IN:	- received input (String)
 * OUT:	- message to send back (int)
 */
enum serialMsg parseSerialIn(String input) {
  if (debug)
    snooze_usb.printf("<-BC127: %s\n", input.c_str());
  unsigned int nb_params = countParams(input);
  double tofloat;

  switch (nb_params) {
  // INQU_OK
  // READY
  case 0: {
    if (notif.equalsIgnoreCase("INQU_OK"))
      return BCNOT_INQ_DONE;
    else if (notif.equalsIgnoreCase("READY"))
      BC127_ready = true;
    break;
  }

  // AVRCP_PLAY [link_ID]
  // AVRCP_PAUSE [link_ID]
  case 1: {
    if (notif.equalsIgnoreCase("AVRCP_PLAY"))
      return msgAvrcpPlay();
    else if (notif.equalsIgnoreCase("AVRCP_PAUSE"))
      return msgAvrcpPause();
    break;
  }

  // ABS_VOL [link_ID](value)
  // LINK_LOSS [link_ID] (status)
  // NAME [addr] {"name" || name}
  case 2: {
    if (notif.equalsIgnoreCase("ABS_VOL"))
      return msgAbsVol(param1, param2);
    else if (notif.equalsIgnoreCase("LINK_LOSS"))
      return msgLinkLoss(param1, param2);
    else if (notif.equalsIgnoreCase("NAME"))
      return msgName1(param1, param2);
    break;
  }

  // CLOSE_OK [link_ID] (profile) (Bluetooth address)
  // NAME [addr] {"n1 n2" || n1 n2}
  // OPEN_OK [link_ID] (profile) (Bluetooth address)
  // RECV [link_ID] (size) (report data)
  case 3: {
    if (notif.equalsIgnoreCase("CLOSE_OK"))
      return msgCloseOk(param1, param2, param3);
    else if (notif.equalsIgnoreCase("NAME"))
      return msgName2(param1, param2, param3);
    else if (notif.equalsIgnoreCase("OPEN_OK"))
      return msgOpenOk(param1, param2, param3);
    else if (notif.equalsIgnoreCase("RECV"))
      return msgRecv1(param1, param2, param3);
    break;
  }

  // INQUIRY(BTADDR) {"name" || name} (COD) (RSSI)
  // NAME [addr] {"n1 n2 n3" || n1 n2 n3}
  // STATE (connected) (connectable) (discoverable) (ble)
  // RECV [link_ID] (size) (report data) <-- (report data) with 2 parameters
  case 4: {
    if (notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry1(param1, param2, param3, param4);
    else if (notif.equalsIgnoreCase("NAME"))
      return msgName3(param1, param2, param3, param4);
    else if (notif.equalsIgnoreCase("STATE"))
      return msgState(param1, param2, param3, param4);
    else if (notif.equalsIgnoreCase("RECV"))
      return msgRecv2(param1, param2, param3, param4);
    break;
  }

  // INQUIRY (BTADDR) {"n1 n2" || n1 n2} (COD) (RSSI)
  // LINK [link_ID] (STATE) (PROFILE) (BTADDR) (INFO)
  // NAME [addr] {"n1 n2 n3 n4" || n1 n2 n3 n4}
  // RECV [link_ID] (size) (report data) <-- (report data) with 3 parameters
  case 5: {
    if (notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry2(param1, param2, param3, param4, param5);
    else if (notif.equalsIgnoreCase("LINK"))
      return msgLink1(param1, param2, param3, param4, param5);
    else if (notif.equalsIgnoreCase("NAME"))
      return msgName4(param1, param2, param3, param4, param5);
    else if (notif.equalsIgnoreCase("RECV"))
      return msgRecv3(param1, param2, param3, param4, param5);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3" || n1 n2 n3} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2)
  // RECV [link_ID] (size) (report data) <-- (report data) with 4 parameters
  case 6: {
    if (notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry3(param1, param2, param3, param4, param5, param6);
    else if (notif.equalsIgnoreCase("RECV"))
      return msgRecv4(param1, param2, param3, param4, param5, param6);
    else if (notif.equalsIgnoreCase("LINK"))
      return msgLink2(param1, param2, param3, param4, param5, param6);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3 n4" || n1 n2 n3 n4} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3)
  case 7: {
    if (notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry4(param1, param2, param3, param4, param5, param6,
                         param7);
    else if (notif.equalsIgnoreCase("LINK"))
      return msgLink3(param1, param2, param3, param4, param5, param6, param7);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3 n4 n5" || n1 n2 n3 n4 n5} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4)
  case 8: {
    if (notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry5(param1, param2, param3, param4, param5, param6, param7,
                         param8);
    else if (notif.equalsIgnoreCase("LINK"))
      return msgLink4(param1, param2, param3, param4, param5, param6, param7,
                      param8);
    break;
  }

  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4)
  // (info5)
  case 9: {
    if (notif.equalsIgnoreCase("LINK"))
      return msgLink5(param1, param2, param3, param4, param5, param6, param7,
                      param8, param9);
    break;
  }

  default:
    break;
  }

  return BCCMD__NOTHING;
}

/* sendCmdOut(int)
 * ---------------
 * Send specific commands to the BC127 UART
 * IN:	- message (int)
 * OUT:	- command confirmation (bool)
 */
bool sendCmdOut(int msg) {
  String devString = "";
  String cmdLine = "";

  switch (msg) {
  /* --------
   * COMMANDS
   * -------- */
  case BCCMD__NOTHING:
    break;
  case BCCMD_ADV_ON: // Start advertising on BLE
    cmdLine = "ADVERTISING ON\r";
    break;
  case BCCMD_ADV_OFF: // Stop advertising
    cmdLine = "ADVERTISING OFF\r";
    break;
  case BCCMD_BLUE_OFF: // Power-off
    cmdLine = "POWER OFF\r";
    break;
  case BCCMD_BLUE_ON: // Power-on
    cmdLine = "POWER ON\r";
    break;
  case BCCMD_BT_NAME: // Ask for friendly name of connected BT device
    cmdLine = "NAME " + String(BT_peer_address) + "\r";
    break;
  case BCCMD_DEV_CONNECT: // Open A2DP connection with 'BT_peer_address'
    cmdLine = cmdDevConnect();
    break;
  case BCCMD_DEV_DISCONNECT1: // Close connection with BT device
    cmdLine = "CLOSE " + String(BT_id_a2dp) + "\r";
    break;
  case BCCMD_DEV_DISCONNECT2:
    cmdLine = "CLOSE " + String(BT_id_avrcp) + "\r";
    break;
  case BCCMD_INQUIRY: // Start inquiry on BT for 10 s, first clear the device
                      // list
    cmdLine = cmdInquiry();
    break;
  case BCCMD_MON_PAUSE: // Pause monitoring -> AVRCP pause
    cmdLine = cmdMonPause();
    break;
  case BCCMD_MON_START: // Start monitoring -> AVRCP play
    cmdLine = cmdMonStart();
    break;
  case BCCMD_MON_STOP: // Stop monitoring -> AVRCP pause
    cmdLine = cmdMonStop();
    break;
  case BCCMD_REC_START: // Start recording
    working_state.rec_state = RECSTATE_REQ_ON;
    break;
  case BCCMD_REC_STOP: // Stop recording
    working_state.rec_state = RECSTATE_REQ_OFF;
    next_record.man_stop = true;
    break;
  case BCCMD_RESET: // Reset
    cmdLine = "RESET\r";
    break;
  case BCCMD_STATUS: // Status
    cmdLine = "STATUS\r";
    break;
  case BCCMD_VOL_A2DP: // Volume level request
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " 7\r";
    break;
  case BCCMD_VOL_UP: // Volume up -> AVRCP volume up
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " UP\r";
    break;
  case BCCMD_VOL_DOWN: // Volume down -> AVRCP volume down
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " DOWN\r";
    break;
  /* -------------
   * NOTIFICATIONS
   * ------------- */
  case BCNOT_BT_STATE: // BT state notification
    cmdLine = notBtState();
    break;
  case BCNOT_FILEPATH: // FILEPATH notification
    cmdLine = notFilepath();
    break;
  case BCNOT_INQ_DONE: // Inquiry sequence done -> send notification
    cmdLine = notInqDone();
    break;
  case BCNOT_INQ_START: // Starting inquiry sequences -> send notification
    cmdLine = notInqStart();
    break;
  // Results of the inquiry -> store devices with address & signal strength
  case BCNOT_INQ_STATE: {
    for (unsigned int i = 0; i < found_dev; i++) {
      devString = dev_list[i].name;
      devString.concat(" ");
      devString.concat(String(dev_list[i].strength));
      cmdLine = "SEND " + String(BLE_conn_id) + " INQ " + devString + "\r";
      BLUEPORT.print(cmdLine);
      Alarm.delay(80);
      if (debug)
        snooze_usb.printf("Info:   %s\n", cmdLine.c_str());
    }
    cmdLine = "";
    break;
  }

  case BCNOT_LATLONG: // GPS
    cmdLine = notLatlong();
    break;
  case BCNOT_MON_STATE: // MON state notification
    cmdLine = notMonState();
    break;
  case BCNOT_REC_STATE: // REC state notification
    cmdLine = notRecState();
    break;
  case BCNOT_REC_NEXT: // REC_NEXT notification
    cmdLine = notRecNext();
    break;
  case BCNOT_REC_NB: // REC_NB notification
    cmdLine = notRecNb();
    break;
  case BCNOT_REC_TS: // REC_TS notification
    cmdLine = notRecTs();
    break;
  case BCNOT_RWIN_OK: // RWIN command confirmation
    cmdLine = notRwinOk();
    break;
  case BCNOT_RWIN_VALS: // RWIN values notification
    cmdLine = notRwinVals();
    break;
  case BCNOT_VOL_LEVEL: // VOL level notification
    cmdLine = notVolLevel();
    break;
  /* ------
   * ERRORS
   * ------ */
  case BCERR_RWIN_BAD_REQ: // RWIN ERR BAD REQUEST
    cmdLine = errRwinBadReq();
    break;
  case BCERR_RWIN_WRONG_PARAMS:
    cmdLine = errRwinWrongParams();
    break;
  case BCERR_VOL_BT_DIS: // VOL ERR NO DEVICE
    cmdLine = errVolBtDis();
    break;
  default: // No recognised command -> send negative confirmation
    return false;
    break;
  }
  if (cmdLine != "") {
    if (debug)
      snooze_usb.printf("->BC127: %s\n", cmdLine.c_str());
  }
  // Send the prepared command line to UART
  BLUEPORT.print(cmdLine);
  // Wait some time to let the sent line finish
  Alarm.delay(80);
  // Send positive confirmation
  return true;
}

/* populateDevlist(String, String, unsigned int)
 * ---------------------------------------------
 * Search if received device address is already existing.
 * If not, fill the list at the next empty place, otherwise
 * just update the signal strength value.
 * IN:	- device address (String)
 *			- device name (String)
 *			- device capabilities (String)
 *			- absolute signal stregth value (unsigned int)
 * OUT:	- none
 */
void populateDevlist(String addr, String name, String caps,
                     unsigned int stren) {
  bool found = false;
  int lastPos = 0;
  for (int i = 0; i < DEVLIST_MAXLEN; i++) {
    // Received address matches to an already existing one.
    if (addr.equals(dev_list[i].address)) {
      dev_list[i].strength = stren;
      found = true;
      break;
    }
    // No address matched and the current position is empty.
    else if (dev_list[i].address.equals("")) {
      lastPos = i;
      break;
    }
  }

  // If not matching device has been found, fill the next emplacement.
  if (!found) {
    dev_list[lastPos].address = addr;
    dev_list[lastPos].name = name;
    dev_list[lastPos].capabilities = caps;
    dev_list[lastPos].strength = stren;
    found_dev += 1;
  }

  // if(debug) snooze_usb.println("Info:    Device list:");
  // for(unsigned int i = 0; i < found_dev; i++) {
  // if(debug) snooze_usb.print(i); if(debug) snooze_usb.print(": ");
  // if(debug) snooze_usb.print(dev_list[i].address); if(debug)
  // snooze_usb.print(", "); if(debug)
  // snooze_usb.print(dev_list[i].capabilities); if(debug) snooze_usb.print(",
  // "); if(debug) snooze_usb.println(dev_list[i].strength);
  // }
}

/* searchDevlist(String)
 * ---------------------
 * Search for the requested address into the previously
 * filled device list, in order to open a A2DP connection.
 * IN:	- device name (String)
 * OUT:	- device found (bool)
 */
bool searchDevlist(String name) {
  for (int i = 0; i < DEVLIST_MAXLEN; i++) {
    if (dev_list[i].name.equalsIgnoreCase(name)) {
      if (debug)
        snooze_usb.println("Info:    Device found in list!");
      BT_peer_address = dev_list[i].address;
      return true;
    }
  }
  if (debug)
    snooze_usb.println("Info:    Nothing found in list");
  return false;
}

unsigned int countParams(String input) {
  // ================================================================
  // Slicing input string.
  // Currently 3 usefull schemes:
  // - 2 words   --> e.g. "OPEN_OK AVRCP"
  // - RECV BLE  --> commands received from phone over BLE
  // - other     --> 3+ words inputs like "INQUIRY xxxx 240404 -54dB"
  // ================================================================
  int slice1 = input.indexOf(" ");
  int slice2 = input.indexOf(" ", slice1 + 1);
  int slice3 = input.indexOf(" ", slice2 + 1);
  int slice4 = input.indexOf(" ", slice3 + 1);
  int slice5 = input.indexOf(" ", slice4 + 1);
  int slice6 = input.indexOf(" ", slice5 + 1);
  int slice7 = input.indexOf(" ", slice6 + 1);
  int slice8 = input.indexOf(" ", slice7 + 1);
  int slice9 = input.indexOf(" ", slice8 + 1);
  int slice10 = input.indexOf(" ", slice9 + 1);

  unsigned int ret = 0;
  // no space found -> notification without parameter
  if (slice1 == -1) {
    notif = input;
    ret = 0;
  }
  // 1+ parameter
  else {
    notif = input.substring(0, slice1);
    if (slice2 == -1) {
      param1 = input.substring(slice1 + 1);
      ret = 1;
    }
    // 2+ parameters
    else {
      param1 = input.substring(slice1 + 1, slice2);
      if (slice3 == -1) {
        param2 = input.substring(slice2 + 1);
        ret = 2;
      }
      // 3+ parameters
      else {
        param2 = input.substring(slice2 + 1, slice3);
        if (slice4 == -1) {
          param3 = input.substring(slice3 + 1);
          ret = 3;
        }
        // 4+ parameters
        else {
          param3 = input.substring(slice3 + 1, slice4);
          if (slice5 == -1) {
            param4 = input.substring(slice4 + 1);
            ret = 4;
          }
          // 5+ parameters
          else {
            param4 = input.substring(slice4 + 1, slice5);
            if (slice6 == -1) {
              param5 = input.substring(slice5 + 1);
              ret = 5;
            }
            // 6+ parameters
            else {
              param5 = input.substring(slice5 + 1, slice6);
              if (slice7 == -1) {
                param6 = input.substring(slice6 + 1);
                ret = 6;
              }
              // 7+ parameters
              else {
                param6 = input.substring(slice6 + 1, slice7);
                if (slice8 == -1) {
                  trash = input.substring(slice7 + 1);
                  ret = 7;
                }
                // 8+ paramters
                else {
                  param7 = input.substring(slice7 + 1, slice8);
                  if (slice8 == -1) {
                    trash = input.substring(slice8 + 1);
                    ret = 8;
                  }
                  // 9+ parameters
                  else {
                    param8 = input.substring(slice8 + 1, slice9);
                    if (slice9 == -1) {
                      trash = input.substring(slice9 + 1);
                      ret = 9;
                    }
                    // 10+ parameters
                    else {
                      param9 = input.substring(slice9 + 1, slice10);
                      if (slice10 == -1) {
                        trash = input.substring(slice10 + 1);
                        ret = 10;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  // if(debug) snooze_usb.printf("#params: %d\n", nb_params);
  return ret;
}

enum serialMsg msgAvrcpPlay(void) {
  working_state.mon_state = MONSTATE_REQ_ON;
  return BCCMD__NOTHING;
}
enum serialMsg msgAvrcpPause(void) {
  working_state.mon_state = MONSTATE_REQ_OFF;
  return BCCMD__NOTHING;
}
enum serialMsg msgAbsVol(String p1, String p2) {
  vol_value = (float)p2.toInt() / ABS_VOL_MAX_VAL;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_VOL_LEVEL;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgLinkLoss(String p1, String p2) {
  if (debug)
    snooze_usb.printf("Info:    link_ID: %s, status: %s\n", p1.c_str(),
                      p2.c_str());
  if (p1.toInt() == BT_id_a2dp) {
    if (p2.toInt() == 1) {
      working_state.mon_state = MONSTATE_REQ_OFF;
      working_state.bt_state = BTSTATE_DISCONNECTED;
    } else if (p2.toInt() == 0) {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgName1(String p1, String p2) {
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p2.length();
    BT_peer_name = p2.substring(1, (strlen - 1));
  } else {
    BT_peer_name = p2;
  }
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgCloseOk(String p1, String p2, String p3) {
  if (param1.toInt() == BT_id_a2dp) {
    if (working_state.bt_state != BTSTATE_OFF)
      working_state.bt_state = BTSTATE_REQ_DISC;
  } else if (param1.toInt() == BLE_conn_id) {
    if (working_state.ble_state != BLESTATE_OFF)
      working_state.ble_state = BLESTATE_REQ_DISC;
  }
  return BCCMD__NOTHING;
}
enum serialMsg msgName2(String p1, String p2, String p3) {
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p3.length();
    BT_peer_name = p2.substring(1) + "_" + p3.substring(0, (strlen - 1));
  } else {
    BT_peer_name = p2 + "_" + p3;
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgOpenOk(String p1, String p2, String p3) {
  if (param2.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = param1.toInt();
    BT_peer_address = param3;
    if (debug)
      snooze_usb.printf(
          "Info:    A2DP connection opened. Conn ID: %d, peer address = %s\n",
          BT_id_a2dp, BT_peer_address.c_str());
    working_state.bt_state = BTSTATE_REQ_CONN;
    return BCCMD_BT_NAME;
  } else if (param2.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = param1.toInt();
    BT_peer_address = param3;
    if (debug)
      snooze_usb.printf("Info:    AVRCP connection opened. Conn ID: %d, peer "
                        "address (check) = %s\n",
                        BT_id_avrcp, BT_peer_address.c_str());
    return BCCMD__NOTHING;
  } else if (param2.equalsIgnoreCase("BLE")) {
    BLE_conn_id = param1.toInt();
    // if(debug) snooze_usb.printf("Info:    BLE connection opened. Conn ID:
    // %d\n", BLE_conn_id);
    working_state.ble_state = BLESTATE_REQ_CONN;
    return BCCMD__NOTHING;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgRecv1(String p1, String p2, String p3) {
  // - "inq"
  // - "disc"
  // - "latlong"
  enum serialMsg ret = BCCMD__NOTHING;

  if (param1.toInt() == BLE_conn_id) {
    if (param3.equalsIgnoreCase("inq")) {
      ret = BCCMD_INQUIRY;
    } else if (param3.equalsIgnoreCase("disc")) {
      working_state.bt_state = BTSTATE_REQ_DISC;
    } else if (param3.equalsIgnoreCase("latlong")) {
      if (debug)
        snooze_usb.println("Info:    Receiving latlong without values");
    }
  }

  return ret;
}
enum serialMsg msgInquiry1(String p1, String p2, String p3, String p4) {
  String addr = p1;
  String name;
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    name = p2.substring(1, (p2.length() - 1));
  } else {
    name = p2;
  }
  String caps = p3;
  unsigned int stren = p4.substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
enum serialMsg msgName3(String p1, String p2, String p3, String p4) {
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p4.length();
    BT_peer_name =
        p2.substring(1) + "_" + p3 + "_" + p4.substring(0, (strlen - 1));
  } else {
    BT_peer_name = p2 + "_" + p3 + "_" + p4;
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgState(String p1, String p2, String p3, String p4) {
  if (!p1.substring(p1.length() - 2, p1.length() - 1).toInt()) {
    return BCNOT_BT_STATE;
  }
  return BCCMD__NOTHING;
}
enum serialMsg msgRecv2(String p1, String p2, String p3, String p4) {
  // - "bt {?}"
  // - "conn {address}"
  // - "filepath {?}"
  // - "latlong {?}"
  // - "mon {start/stop/?}"
  // - "rec {start/stop/?}"
  // - "rec_nb {?}"
  // - "rec_next {?}"
  // - "rec_ts {?}"
  // - "rwin {?}"
  // - "time {ts}"
  // - "vol {+/-/?}"
  enum serialMsg ret = BCCMD__NOTHING;
  if (param1.toInt() == BLE_conn_id) {
    if (p3.equalsIgnoreCase("conn")) {
      BT_peer_name = p4;
      return BCCMD_DEV_CONNECT;
    } else if (p3.equalsIgnoreCase("time")) {
      unsigned long rec_time = p4.toInt();
      if (rec_time > MIN_TIME_DEC) {
        setCurTime(rec_time, TSOURCE_PHONE);
        if (debug)
          snooze_usb.printf("Info:    Timestamp received: %ld\n", rec_time);
      } else {
        if (debug)
          snooze_usb.println("Error: Received time not correct!");
      }
    } else if (p3.equalsIgnoreCase("rec")) {
      if (p4.equalsIgnoreCase("start"))
        return BCCMD_REC_START;
      else if (p4.equalsIgnoreCase("stop"))
        return BCCMD_REC_STOP;
      else if (p4.equalsIgnoreCase("?"))
        return BCNOT_REC_STATE;
    } else if (p3.equalsIgnoreCase("rec_next")) {
      if (p4.equalsIgnoreCase("?"))
        return BCNOT_REC_NEXT;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_next command not listed");
      }
    } else if (p3.equalsIgnoreCase("rec_nb")) {
      if (p4.equalsIgnoreCase("?"))
        return BCNOT_REC_NB;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_nb command not listed");
      }
    } else if (p3.equalsIgnoreCase("rec_ts")) {
      if (p4.equalsIgnoreCase("?"))
        return BCNOT_REC_TS;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_ts command not listed");
      }
    } else if (p3.equalsIgnoreCase("mon")) {
      if (p4.equalsIgnoreCase("start"))
        working_state.mon_state = MONSTATE_REQ_ON;
      else if (p4.equalsIgnoreCase("stop"))
        working_state.mon_state = MONSTATE_REQ_OFF;
      else if (p4.equalsIgnoreCase("?"))
        return BCNOT_MON_STATE;
    } else if (p3.equalsIgnoreCase("vol")) {
      if ((working_state.bt_state == BTSTATE_CONNECTED) ||
          (working_state.bt_state == BTSTATE_PLAY)) {
        if (p4.equalsIgnoreCase("+")) {
          return BCCMD_VOL_UP;
        } else if (p4.equalsIgnoreCase("-")) {
          return BCCMD_VOL_DOWN;
        } else if (p4.equalsIgnoreCase("?")) {
          return BCNOT_VOL_LEVEL;
        }
      } else {
        return BCERR_VOL_BT_DIS;
      }
    } else if (p3.equalsIgnoreCase("bt")) {
      if (p4.equalsIgnoreCase("?")) {
        return BCCMD_STATUS;
      }
    } else if (p3.equalsIgnoreCase("rwin")) {
      if (p4.equalsIgnoreCase("?")) {
        return BCNOT_RWIN_VALS;
      } else {
        return BCERR_RWIN_BAD_REQ;
      }
    } else if (p3.equalsIgnoreCase("filepath")) {
      if (p4.equalsIgnoreCase("?")) {
        return BCNOT_FILEPATH;
      }
    } else if (p3.equalsIgnoreCase("latlong")) {
      if (p4.equalsIgnoreCase("?")) {
        return BCNOT_LATLONG;
      }
    }
  }
  return ret;
}
enum serialMsg msgInquiry2(String p1, String p2, String p3, String p4,
                           String p5) {
  String addr = p1;
  String name;
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    name = p2.substring(1) + "_" + p3.substring(0, (p3.length() - 1));
  } else {
    name = p2 + "_" + p3;
  }
  String caps = p4;
  unsigned int stren = p5.substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
enum serialMsg msgLink1(String p1, String p2, String p3, String p4, String p5) {
  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p3.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return BCCMD__NOTHING;
}
enum serialMsg msgName4(String p1, String p2, String p3, String p4, String p5) {
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p5.length();
    BT_peer_name = p2.substring(1) + "_" + p3 + "_" + p4 + "_" +
                   p5.substring(0, (strlen - 1));
  } else {
    BT_peer_name = p2 + "_" + p3 + "_" + p4 + "_" + p5;
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
enum serialMsg msgRecv3(String p1, String p2, String p3, String p4, String p5) {
  // - "latlong {lat long}"
  if (param1.toInt() == BLE_conn_id) {
    if (param3.equalsIgnoreCase("latlong")) {
      snooze_usb.printf("Received latlong info: %f, %f", atof(param4.c_str()),
                        atof(param5.c_str()));
      if ((param4.c_str() == NULL) || (param5.c_str() == NULL)) {
        next_record.gps_source = GPS_NONE;
        next_record.gps_lat = NULL;
        next_record.gps_long = NULL;
      } else {
        next_record.gps_lat = atof(param4.c_str());
        next_record.gps_long = atof(param5.c_str());
        next_record.gps_source = GPS_PHONE;
      }
    }
  }
  return BCCMD__NOTHING;
}
enum serialMsg msgInquiry3(String p1, String p2, String p3, String p4,
                           String p5, String p6) {
  String addr = p1;
  String name;
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    name =
        p2.substring(1) + "_" + p3 + "_" + p4.substring(0, (p4.length() - 1));
  } else {
    name = p2 + "_" + p3 + "_" + p4;
  }
  String caps = p5;
  unsigned int stren = p6.substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
enum serialMsg msgRecv4(String p1, String p2, String p3, String p4, String p5,
                        String p6) {
  enum serialMsg ret = BCCMD__NOTHING;

  // - "rwin {duration} {period} {occurences}"
  if (p1.toInt() == BLE_conn_id) {
    if (p3.equalsIgnoreCase("rwin")) {
      unsigned int d, p;
      d = p4.toInt();
      p = p5.toInt();
      if (d < p) {
        breakTime(d, rec_window.duration);
        breakTime(p, rec_window.period);
        rec_window.occurences = p6.toInt();
        return BCNOT_RWIN_OK;
      } else {
        return BCERR_RWIN_WRONG_PARAMS;
      }
    }
  }
  return ret;
}
enum serialMsg msgLink2(String p1, String p2, String p3, String p4, String p5,
                        String p6) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p3.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
enum serialMsg msgInquiry4(String p1, String p2, String p3, String p4,
                           String p5, String p6, String p7) {
  String addr = p1;
  String name;
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    name = p2.substring(1) + "_" + p3 + "_" + p4 + "_" +
           p5.substring(0, (p5.length() - 1));
  } else {
    name = p2 + "_" + p3 + "_" + p4 + "_" + p5;
  }
  String caps = p6;
  unsigned int stren = p7.substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
enum serialMsg msgLink3(String p1, String p2, String p3, String p4, String p5,
                        String p6, String p7) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p3.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
enum serialMsg msgInquiry5(String p1, String p2, String p3, String p4,
                           String p5, String p6, String p7, String p8) {
  String addr = p1;
  String name;
  if (p2.substring(0, 1).equalsIgnoreCase("\"")) {
    name = p2.substring(1) + "_" + p3 + "_" + p4 + "_" + p5 + "_" +
           p6.substring(0, (p6.length() - 1));
  } else {
    name = p2 + "_" + p3 + "_" + p4 + "_" + p5 + "_" + p6;
  }
  String caps = p7;
  unsigned int stren = p8.substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
enum serialMsg msgLink4(String p1, String p2, String p3, String p4, String p5,
                        String p6, String p7, String p8) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p3.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
enum serialMsg msgLink5(String p1, String p2, String p3, String p4, String p5,
                        String p6, String p7, String p8, String p9) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p3.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}

String cmdDevConnect(void) {
  if (searchDevlist(BT_peer_name)) {
    if (debug)
      snooze_usb.printf("Info:    Opening BT connection @%s (%s)\n",
                        BT_peer_address.c_str(), BT_peer_name.c_str());
    return ("OPEN " + BT_peer_address + " A2DP\r");
  } else {
    return ("SEND " + String(BLE_conn_id) + " CONN ERR NO BT DEVICE!\r");
  }
}
String cmdInquiry(void) {
  String cmd, devString;
  for (int i = 0; i < DEVLIST_MAXLEN; i++) {
    dev_list[i].address = "";
    dev_list[i].capabilities = "";
    dev_list[i].strength = 0;
  }
  found_dev = 0;
  BT_peer_address = "";
  devString = "";

  cmd = "SEND " + String(BLE_conn_id) + " INQ START" + "\r";
  BLUEPORT.print(cmd);
  Alarm.delay(50);
  return ("INQUIRY 10\r");
}
String cmdMonPause(void) {
  working_state.mon_state = MONSTATE_REQ_OFF;
  if (working_state.bt_state == BTSTATE_CONNECTED) {
    return ("MUSIC " + String(BT_id_a2dp) + " PAUSE\r");
  } else
    return "";
}
String cmdMonStart(void) {
  // working_state.mon_state = MONSTATE_REQ_ON;
  // if(working_state.bt_state == BTSTATE_CONNECTED) {
  return ("MUSIC " + String(BT_id_a2dp) + " PLAY\r");
  // }
  // else return "";
}
String cmdMonStop(void) {
  // working_state.mon_state = MONSTATE_REQ_OFF;
  if ((working_state.bt_state == BTSTATE_CONNECTED) ||
      (working_state.bt_state == BTSTATE_PLAY)) {
    return ("MUSIC " + String(BT_id_a2dp) + " STOP\r");
  } else
    return "";
}

String notBtState(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id);
    if (working_state.bt_state == BTSTATE_CONNECTED) {
      ret += " BT " + BT_peer_name + "\r";
    } else if (working_state.bt_state == BTSTATE_INQUIRY) {
      ret += " BT INQ\r";
    } else {
      ret += " BT disconnected\r";
    }
  } else
    ret = "";
  return ret;
}
String notFilepath(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " FP " + rec_path + "\r");
  } else
    return "";
}
String notInqDone(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " INQ DONE\r");
  } else
    return "";
}
String notInqStart(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " INQ START\r");
  } else
    return "";
}
String notLatlong(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id) + " LATLONG ";
    if ((next_record.gps_lat != NULL) && (next_record.gps_long != NULL)) {
      ret += String(next_record.gps_lat) + " " + String(next_record.gps_long);
    }
    ret += "\r";
  } else
    ret = "";
  return ret;
}
String notMonState(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id);
    if (working_state.mon_state == MONSTATE_ON)
      ret += " MON ON\r";
    else
      ret += " MON OFF\r";
  } else
    ret = "";
  return ret;
}
String notRecNb(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_NB " + (next_record.cnt + 1) +
            "\r");
  } else
    return "";
}
String notRecNext(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_NEXT " + next_record.tss +
            "\r");
  } else
    return "";
}
String notRecState(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id);
    if (working_state.rec_state == RECSTATE_ON) {
      ret += " REC ON\r";
    } else if ((working_state.rec_state == RECSTATE_WAIT) ||
               (working_state.rec_state == RECSTATE_IDLE)) {
      ret += " REC WAIT\r";
    } else {
      ret += " REC OFF\r";
    }
  } else
    ret = "";
  return ret;
}
String notRecTs(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_TS " + next_record.tss +
            "\r");
  } else
    return "";
}
String notRwinOk(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN PARAMS OK\r");
  } else
    return "";
}
String notRwinVals(void) {
  unsigned int l, p, o;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    l = rec_window.duration.Second +
        (rec_window.duration.Minute * SECS_PER_MIN) +
        (rec_window.duration.Hour * SECS_PER_HOUR);
    if (debug)
      snooze_usb.printf(
          "Info:    Sending RWIN values. Duration in s = %ld --> %dh%dm%ds\n",
          l, rec_window.duration.Hour, rec_window.duration.Minute,
          rec_window.duration.Second);
    p = rec_window.period.Second + (rec_window.period.Minute * SECS_PER_MIN) +
        (rec_window.period.Hour * SECS_PER_HOUR);
    o = rec_window.occurences;
    return ("SEND " + String(BLE_conn_id) + " RWIN " + String(l) + " " +
            String(p) + " " + String(o) + "\r");
  } else
    return "";
}
String notVolLevel(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " VOL " + String(vol_value) + "\r");
  } else
    return "";
}

String errRwinBadReq(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN ERR BAD REQUEST!\r");
  } else
    return "";
}
String errRwinWrongParams(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN ERR WRONG PARAMS!\r");
  } else
    return "";
}
String errVolBtDis(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " VOL ERR NO BT DEVICE!\r");
  } else
    return "";
}
