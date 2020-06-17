/*
 * BC127
 *
 * Miscellaneous functions to communicate with the Sierra Wireless BC127 module.
 *
 */
/*** IMPORTED EXTERNAL OBJECTS ***********************************************/
/*****************************************************************************/
#include "BC127.h"

/*** MODULE OBJECTS **********************************************************/
/*****************************************************************************/
/*** Constants ***************************************************************/
/*** Types *******************************************************************/
// BT devices information for connection
struct btDev {
  String address;
  String name;
  String capabilities;
  unsigned int strength;
};
struct btDev dev_list[DEVLIST_MAXLEN];

/*** Variables ***************************************************************/
// Amount of found BT devices on inquiry
unsigned int found_dev;
// Address of the connected BT device
String BT_peer_address = "";
// Name of the connected BT device
String BT_peer_name = "";
// ID's (A2DP&AVRCP) of the established BT connection
int BT_id_a2dp = 0;
int BT_id_avrcp = 0;
// ID of the established BLE connection
int BLE_conn_id = 0;
// Flag indicating that the BC127 device is ready
bool BC127_ready = false;
String notif, param1, param2, param3, param4, param5, param6, param7, param8,
    param9, trash;

/*** Function prototypes *****************************************************/
/*** Macros ******************************************************************/
/*** Constant objects ********************************************************/
/*** Functions implementation ************************************************/

/*****************************************************************************/
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
static void populateDevlist(String addr, String name, String caps,
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
}
/*****************************************************************************/

/*****************************************************************************/
static unsigned int countParams(String input) {
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
/*****************************************************************************/

/*****************************************************************************/
/* searchDevlist(String)
 * ---------------------
 * Search for the requested address into the previously
 * filled device list, in order to open a A2DP connection.
 * IN:	- device name (String)
 * OUT:	- device found (bool)
 */
static bool searchDevlist(String name) {
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
/*****************************************************************************/

// ---------------------
// RECEIVED BLE MESSAGES
// ---------------------
/*****************************************************************************/
static enum serialMsg msgAbsVol(String p1, String p2) {
  vol_value = (float)p2.toInt() / ABS_VOL_MAX_VAL;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_VOL_LEVEL;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgA2dpInfo(String p1, String p2) {
  static float old_vol = 0;
  enum serialMsg toRet = BCCMD__NOTHING;

  if (p1.equalsIgnoreCase("A2DP")) {
    float new_vol = (float)strtol(p2.c_str(), NULL, 16) / VOL_MAX_VAL_HEX;
    if (new_vol != old_vol) {
      vol_value = new_vol;
      old_vol = new_vol;
      toRet = BCNOT_VOL_LEVEL;
    }
  }

  return toRet;
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgAvrcpPlay(void) {
  working_state.mon_state = MONSTATE_REQ_ON;
  return BCCMD__NOTHING;
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgAvrcpPause(void) {
  working_state.mon_state = MONSTATE_REQ_OFF;
  return BCCMD__NOTHING;
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgCloseOk(String p1, String p2, String p3) {
  if (p1.toInt() == BT_id_a2dp) {
    if (working_state.bt_state != BTSTATE_OFF)
      working_state.bt_state = BTSTATE_REQ_DISC;
  } else if (p1.toInt() == BT_id_avrcp) {
    if (working_state.bt_state != BTSTATE_OFF)
      working_state.bt_state = BTSTATE_REQ_DISC;
  } else if (p1.toInt() == BLE_conn_id) {
    if (working_state.ble_state != BLESTATE_OFF)
      working_state.ble_state = BLESTATE_REQ_DISC;
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgInquiry1(String p1, String p2, String p3, String p4) {
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
/*****************************************************************************/
static enum serialMsg msgInquiry2(String p1, String p2, String p3, String p4,
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
/*****************************************************************************/
static enum serialMsg msgInquiry3(String p1, String p2, String p3, String p4,
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
/*****************************************************************************/
static enum serialMsg msgInquiry4(String p1, String p2, String p3, String p4,
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
/*****************************************************************************/
static enum serialMsg msgInquiry5(String p1, String p2, String p3, String p4,
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
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgLink1(String p1, String p2, String p3, String p4,
                               String p5) {
  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d, state: %s\n",
                        BT_peer_address.c_str(), BT_id_a2dp, p5.c_str());
    if (p5.equalsIgnoreCase("STREAMING")) {
      snooze_usb.printf("Streaming state\n");
      // Alarm.free(alarm_req_vol_id);
      // alarm_req_vol_id =
      //     Alarm.timerRepeat(REQ_VOL_INTERVAL_SEC, timerReqVolDone);
      working_state.bt_state = BTSTATE_PLAY;
    } else {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
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
/*****************************************************************************/
static enum serialMsg msgLink2(String p1, String p2, String p3, String p4,
                               String p5, String p6) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d, state: %s\n",
                        BT_peer_address.c_str(), BT_id_a2dp, p5.c_str());
    if (p5.equalsIgnoreCase("STREAMING")) {
      snooze_usb.printf("Streaming state\n");
      // Alarm.free(alarm_req_vol_id);
      // alarm_req_vol_id =
      //     Alarm.timerRepeat(REQ_VOL_INTERVAL_SEC, timerReqVolDone);
      working_state.bt_state = BTSTATE_PLAY;
    } else {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
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
/*****************************************************************************/
static enum serialMsg msgLink3(String p1, String p2, String p3, String p4,
                               String p5, String p6, String p7) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d, state: %s\n",
                        BT_peer_address.c_str(), BT_id_a2dp, p5.c_str());
    if (p5.equalsIgnoreCase("STREAMING")) {
      snooze_usb.printf("Streaming state\n");
      // Alarm.free(alarm_req_vol_id);
      // alarm_req_vol_id =
      //     Alarm.timerRepeat(REQ_VOL_INTERVAL_SEC, timerReqVolDone);
      working_state.bt_state = BTSTATE_PLAY;
    } else {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
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
/*****************************************************************************/
static enum serialMsg msgLink4(String p1, String p2, String p3, String p4,
                               String p5, String p6, String p7, String p8) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d, state: %s\n",
                        BT_peer_address.c_str(), BT_id_a2dp, p5.c_str());
    if (p5.equalsIgnoreCase("STREAMING")) {
      snooze_usb.printf("Streaming state\n");
      // Alarm.free(alarm_req_vol_id);
      // alarm_req_vol_id =
      //     Alarm.timerRepeat(REQ_VOL_INTERVAL_SEC, timerReqVolDone);
      working_state.bt_state = BTSTATE_PLAY;
    } else {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
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
/*****************************************************************************/
static enum serialMsg msgLink5(String p1, String p2, String p3, String p4,
                               String p5, String p6, String p7, String p8,
                               String p9) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p3.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p4;
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d, state: %s\n",
                        BT_peer_address.c_str(), BT_id_a2dp, p5.c_str());
    if (p5.equalsIgnoreCase("STREAMING")) {
      snooze_usb.printf("Streaming state\n");
      // Alarm.free(alarm_req_vol_id);
      // alarm_req_vol_id =
      //     Alarm.timerRepeat(REQ_VOL_INTERVAL_SEC, timerReqVolDone);
      working_state.bt_state = BTSTATE_PLAY;
    } else {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
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
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgLinkLoss(String p1, String p2) {
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
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgName1(String p1, String p2) {
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
/*****************************************************************************/
static enum serialMsg msgName2(String p1, String p2, String p3) {
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
/*****************************************************************************/
static enum serialMsg msgName3(String p1, String p2, String p3, String p4) {
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
/*****************************************************************************/
static enum serialMsg msgName4(String p1, String p2, String p3, String p4,
                               String p5) {
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
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgOpenOk(String p1, String p2, String p3) {
  if (p2.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p1.toInt();
    BT_peer_address = p3;
    if (debug)
      snooze_usb.printf(
          "Info:    A2DP connection opened. Conn ID: %d, peer address = %s\n",
          BT_id_a2dp, BT_peer_address.c_str());
    working_state.bt_state = BTSTATE_REQ_CONN;
    return BCCMD_BT_NAME;
  } else if (p2.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p1.toInt();
    BT_peer_address = p3;
    if (debug)
      snooze_usb.printf("Info:    AVRCP connection opened. Conn ID: %d, peer "
                        "address (check) = %s\n",
                        BT_id_avrcp, BT_peer_address.c_str());

    if (working_state.mon_state == MONSTATE_ON) {
      return BCCMD_MON_START;
    } else {
      return BCCMD__NOTHING;
    }
  } else if (p2.equalsIgnoreCase("BLE")) {
    BLE_conn_id = p1.toInt();
    working_state.ble_state = BLESTATE_REQ_CONN;
    // Request time update from phone
    alarm_request_id = Alarm.timerOnce(REQ_TIME_INTERVAL_SEC, alarmRequestDone);
    return BCCMD__NOTHING;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgRecv1(String p1, String p2, String p3) {
  // - "inq"
  // - "disc"
  // - "latlong"
  enum serialMsg ret = BCCMD__NOTHING;

  if (p1.toInt() == BLE_conn_id) {
    if (p3.equalsIgnoreCase("inq")) {
      ret = BCCMD_INQUIRY;
    } else if (p3.equalsIgnoreCase("disc")) {
      working_state.bt_state = BTSTATE_REQ_DISC;
    } else if (p3.equalsIgnoreCase("latlong")) {
      if (debug)
        snooze_usb.println("Info:    Receiving latlong without values");
    }
  }

  return ret;
}
/*****************************************************************************/
static enum serialMsg msgRecv2(String p1, String p2, String p3, String p4) {
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
  if (p1.toInt() == BLE_conn_id) {
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
      return BCREQ_LATLONG;
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
/*****************************************************************************/
static enum serialMsg msgRecv3(String p1, String p2, String p3, String p4,
                               String p5) {
  // - "latlong {lat long}"
  if (p1.toInt() == BLE_conn_id) {
    if (p3.equalsIgnoreCase("latlong")) {
      if (debug)
        snooze_usb.printf("Received latlong info: %f, %f\n", atof(p4.c_str()),
                          atof(p5.c_str()));

      if ((p4.c_str() == NULL) || (p5.c_str() == NULL)) {
        next_record.gps_source = GPS_NONE;
        next_record.gps_lat = 1000.0;
        next_record.gps_long = 1000.0;
      } else {
        next_record.gps_lat = atof(p4.c_str());
        next_record.gps_long = atof(p5.c_str());
        next_record.gps_source = GPS_PHONE;
      }
    }
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/
static enum serialMsg msgRecv4(String p1, String p2, String p3, String p4,
                               String p5, String p6) {
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
/*****************************************************************************/
/*****************************************************************************/
static enum serialMsg msgRoleOk(String p1, String p2) { return BCCMD__NOTHING; }
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgState(String p1, String p2, String p3, String p4) {
  bool connState = p1.substring(p1.length() - 2, p1.length() - 1).toInt();
  if (debug)
    snooze_usb.printf("CONNECTED state: %d, A2DP ID: %d\n", connState,
                      BT_id_a2dp);
  if (!connState) {
    return BCNOT_BT_STATE;
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/

// ---------------
// OUTPUT COMMANDS
// ---------------
/*****************************************************************************/
static String cmdDevConnect(void) {
  if (searchDevlist(BT_peer_name)) {
    if (debug)
      snooze_usb.printf("Info:    Opening BT connection @%s (%s)\n",
                        BT_peer_address.c_str(), BT_peer_name.c_str());
    return ("OPEN " + BT_peer_address + " A2DP\r");
  } else {
    return ("SEND " + String(BLE_conn_id) + " CONN ERR NO BT DEVICE!\r");
  }
}
/*****************************************************************************/
/*****************************************************************************/
static String cmdInquiry(void) {
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
/*****************************************************************************/
/*****************************************************************************/
static String cmdMonPause(void) {
  working_state.mon_state = MONSTATE_REQ_OFF;
  if (working_state.bt_state == BTSTATE_CONNECTED) {
    return ("MUSIC " + String(BT_id_a2dp) + " PAUSE\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String cmdMonStart(void) {
  // working_state.mon_state = MONSTATE_REQ_ON;
  // if(working_state.bt_state == BTSTATE_CONNECTED) {
  return ("MUSIC " + String(BT_id_a2dp) + " PLAY\r");
  // }
  // else return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String cmdMonStop(void) {
  // working_state.mon_state = MONSTATE_REQ_OFF;
  if ((working_state.bt_state == BTSTATE_CONNECTED) ||
      (working_state.bt_state == BTSTATE_PLAY)) {
    return ("MUSIC " + String(BT_id_a2dp) + " STOP\r");
  } else
    return "";
}
/*****************************************************************************/

// --------------------
// OUTPUT NOTIFICATIONS
// --------------------
/*****************************************************************************/
static String notBtState(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id);
    if ((working_state.bt_state == BTSTATE_CONNECTED) ||
        (working_state.bt_state == BTSTATE_PLAY)) {
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
/*****************************************************************************/
/*****************************************************************************/
static String notFilepath(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " FP " + rec_path + "\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notInqDone(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " INQ DONE\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notInqStart(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " INQ START\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notLatlong(void) {
  String ret;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    ret = "SEND " + String(BLE_conn_id) + " LATLONG ";
    if ((next_record.gps_lat < 1000.0) && (next_record.gps_long < 1000.0)) {
      ret += String(next_record.gps_lat) + " " + String(next_record.gps_long);
    }
    ret += "\r";
  } else
    ret = "";
  return ret;
}
/*****************************************************************************/
/*****************************************************************************/
static String notMonState(void) {
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
/*****************************************************************************/
/*****************************************************************************/
static String notRecNb(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_NB " + (next_record.cnt + 1) +
            "\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notRecNext(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_NEXT " + next_record.tss +
            "\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notRecRem(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_REM " + rec_rem + "\r");
  } else {
    return "";
  }
}
/*****************************************************************************/
/*****************************************************************************/
static String notRecState(void) {
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
/*****************************************************************************/
/*****************************************************************************/
static String notRecTs(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " REC_TS " + next_record.tss +
            "\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notRwinOk(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN PARAMS OK\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String notRwinVals(void) {
  unsigned int l, p, o;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    l = rec_window.duration.Second +
        (rec_window.duration.Minute * SECS_PER_MIN) +
        (rec_window.duration.Hour * SECS_PER_HOUR);
    if (debug)
      snooze_usb.printf("Info:    Sending RWIN values. Duration in s = %ld --> "
                        "%dh%02dm%02ds\n",
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
/*****************************************************************************/
/*****************************************************************************/
static String notVolLevel(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " VOL " + String(vol_value) + "\r");
  } else
    return "";
}
/*****************************************************************************/

// ---------------
// OUTPUT REQUESTS
// ---------------
/*****************************************************************************/
static String reqTime(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " TIME ?\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String reqLatLong(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " LATLONG ?\r");
  } else
    return "";
}
/*****************************************************************************/

// ---------------------
// OUTPUT ERROR MESSAGES
// ---------------------
/*****************************************************************************/
static String errRwinBadReq(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN ERR BAD REQUEST!\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String errRwinWrongParams(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " RWIN ERR WRONG PARAMS!\r");
  } else
    return "";
}
/*****************************************************************************/
/*****************************************************************************/
static String errVolBtDis(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " VOL ERR NO BT DEVICE!\r");
  } else
    return "";
}
/*****************************************************************************/

/*** EXPORTED OBJECTS ********************************************************/
/*****************************************************************************/
/*** Functions ***************************************************************/

/*****************************************************************************/
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
  // Alarm.delay(200);
  // bc127BlueOff();
  // Alarm.delay(500);
}
/*****************************************************************************/

/*****************************************************************************/
/* bc127BlueOn(void)
 * -----------------
 * Switch on the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOn(void) { sendCmdOut(BCCMD_BLUE_ON); }
/*****************************************************************************/

/*****************************************************************************/
/* bc127BlueOff(void)
 * -------------------
 * Switch off the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOff(void) { sendCmdOut(BCCMD_BLUE_OFF); }
/*****************************************************************************/

/*****************************************************************************/
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
/*****************************************************************************/

/*****************************************************************************/
/* bc127AdvStart(void)
 * -------------------
 * Start advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStart(void) { sendCmdOut(BCCMD_ADV_ON); }
/*****************************************************************************/

/*****************************************************************************/
/* bc127AdvStop(void)
 * ------------------
 * Stop advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStop(void) {
  Alarm.disable(alarm_adv_id);
  Alarm.free(alarm_adv_id);
  sendCmdOut(BCCMD_ADV_OFF);
  Alarm.delay(100);
}
/*****************************************************************************/

/*****************************************************************************/
void bc127BleDisconnect(void) {
  sendCmdOut(BCCMD_BLE_DISCONNECT);
  Alarm.delay(200);
}
/*****************************************************************************/

/*****************************************************************************/
/* parseSerialIn(String)
 * ---------------------
 * Parse received string and read information along
 * minimal keywords.
 * IN:	- received input (String)
 * OUT:	- message to send back (int)
 */
enum serialMsg parseSerialIn(String input) {
  if (debug)
    snooze_usb.printf("BC127->: %s\n", input.c_str());

  unsigned int nb_params = countParams(input);

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
    else if (notif.equalsIgnoreCase("ROLE_OK"))
      return msgRoleOk(param1, param2);
    else if (notif.equalsIgnoreCase(String(BT_id_a2dp)))
      return msgA2dpInfo(param1, param2);
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
/*****************************************************************************/

/*****************************************************************************/
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
  // Start BLE advertising
  case BCCMD_ADV_ON:
    cmdLine = "ADVERTISING ON\r";
    break;
  // Stop BLE advertising
  case BCCMD_ADV_OFF:
    cmdLine = "ADVERTISING OFF\r";
    break;
  // Send BLE disconnect command
  case BCCMD_BLE_DISCONNECT:
    cmdLine = "CLOSE " + String(BLE_conn_id) + "\r";
    break;
  // Switch off device (serial)
  case BCCMD_BLUE_OFF:
    cmdLine = "POWER OFF\r";
    break;
  // Switch on device (serial)
  case BCCMD_BLUE_ON:
    cmdLine = "POWER ON\r";
    break;
  // Ask for friendly name of connected BT device
  case BCCMD_BT_NAME:
    cmdLine = "NAME " + String(BT_peer_address) + "\r";
    break;
  // Open A2DP connection with 'BT_peer_address'
  case BCCMD_DEV_CONNECT:
    cmdLine = cmdDevConnect();
    break;
  // Close A2DP connection with BT device
  case BCCMD_DEV_A2DP_DISCONNECT:
    cmdLine = "CLOSE " + String(BT_id_a2dp) + "\r";
    break;
  // Close AVRCP connection with BT device
  case BCCMD_DEV_AVRCP_DISCONNECT:
    cmdLine = "CLOSE " + String(BT_id_avrcp) + "\r";
    break;
  // Start inquiry on BT for 10 s, clearing the device list first
  case BCCMD_INQUIRY:
    cmdLine = cmdInquiry();
    break;
  // Pause monitoring -> AVRCP pause
  case BCCMD_MON_PAUSE:
    cmdLine = cmdMonPause();
    break;
  // Start monitoring -> AVRCP play
  case BCCMD_MON_START:
    cmdLine = cmdMonStart();
    break;
  // Stop monitoring -> AVRCP pause
  case BCCMD_MON_STOP:
    cmdLine = cmdMonStop();
    break;
  // Start recording
  case BCCMD_REC_START:
    working_state.rec_state = RECSTATE_REQ_ON;
    break;
  // Stop recording
  case BCCMD_REC_STOP:
    working_state.rec_state = RECSTATE_REQ_OFF;
    next_record.man_stop = true;
    break;
  // Reset module
  case BCCMD_RESET:
    cmdLine = "RESET\r";
    break;
  // Device connection status
  case BCCMD_STATUS:
    if (BT_id_a2dp != 0)
      cmdLine = "STATUS " + String(BT_id_a2dp) + "\r";
    else
      cmdLine = "STATUS\r";
    break;
  // Volume level
  case BCCMD_VOL_A2DP:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " " +
              String((int)((vol_value * VOL_MAX_VAL_HEX) + 0.5), HEX) + "\r";
    break;
  // Volume up -> AVRCP volume up
  case BCCMD_VOL_UP:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " UP\r";
    break;
  case BCCMD_VOL_REQ:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + "\r";
    break;
  // Volume down -> AVRCP volume down
  case BCCMD_VOL_DOWN:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " DOWN\r";
    break;
  /* -------------
   * NOTIFICATIONS
   * ------------- */
  // BT state
  case BCNOT_BT_STATE:
    cmdLine = notBtState();
    break;
  // Filepath
  case BCNOT_FILEPATH:
    cmdLine = notFilepath();
    break;
  // Inquiry sequence done -> send notification
  case BCNOT_INQ_DONE:
    cmdLine = notInqDone();
    break;
  // Starting inquiry sequences -> send notification
  case BCNOT_INQ_START:
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
  // GPS latlong values
  case BCNOT_LATLONG:
    cmdLine = notLatlong();
    break;
  // MON state
  case BCNOT_MON_STATE:
    cmdLine = notMonState();
    break;
  // REC state
  case BCNOT_REC_STATE:
    cmdLine = notRecState();
    break;
  // REC_NEXT
  case BCNOT_REC_NEXT:
    cmdLine = notRecNext();
    break;
  // REC_NB
  case BCNOT_REC_NB:
    cmdLine = notRecNb();
    break;
  // REC_REM
  case BCNOT_REC_REM:
    cmdLine = notRecRem();
    break;
  // REC_TS
  case BCNOT_REC_TS:
    cmdLine = notRecTs();
    break;
  // RWIN command
  case BCNOT_RWIN_OK:
    cmdLine = notRwinOk();
    break;
  // RWIN values
  case BCNOT_RWIN_VALS:
    cmdLine = notRwinVals();
    break;
  // VOL level
  case BCNOT_VOL_LEVEL:
    cmdLine = notVolLevel();
    break;
  /* --------
   * REQUESTS
   * -------- */
  // Latitude/longitude
  case BCREQ_LATLONG:
    cmdLine = reqLatLong();
    break;
  // Current time
  case BCREQ_TIME:
    cmdLine = reqTime();
    break;
  /* ------
   * ERRORS
   * ------ */
  // RWIN bad request
  case BCERR_RWIN_BAD_REQ:
    cmdLine = errRwinBadReq();
    break;
  // RWIN wrong parameters
  case BCERR_RWIN_WRONG_PARAMS:
    cmdLine = errRwinWrongParams();
    break;
  // VOL no device connected
  case BCERR_VOL_BT_DIS:
    cmdLine = errVolBtDis();
    break;
  // No recognised command -> send negative confirmation
  default:
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
  Alarm.delay(BC127_CMD_WAIT_MS);
  // Send positive confirmation
  return true;
}
/*****************************************************************************/
