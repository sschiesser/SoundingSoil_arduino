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
typedef struct {
  String notif;
  String p[10];
  String trash;
}serialParams_t;

/*** Variables ***************************************************************/
// Amount of found BT devices on inquiry
unsigned int found_dev;
// Address of the connected BT device
String BT_peer_address = "";
// Name of the connected BT device
String BT_peer_name = "auto";
// ID's (A2DP&AVRCP) of the established BT connection
int BT_id_a2dp = 0;
int BT_id_avrcp = 0;
// ID of the established BLE connection
int BLE_conn_id = 0;
// Flag indicating that the BC127 device is ready
bool BC127_ready = false;
// String notif, param1, param2, param3, param4, param5, param6, param7, param8,
//     param9, trash;

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
static unsigned int countParams(String input, serialParams_t *output) {
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
    output->notif = input;
    ret = 0;
  }
  // 1+ parameter
  else {
    output->notif = input.substring(0, slice1);
    if (slice2 == -1) {
      output->p[0] = input.substring(slice1 + 1);
      ret = 1;
    }
    // 2+ parameters
    else {
      output->p[0] = input.substring(slice1 + 1, slice2);
      if (slice3 == -1) {
        output->p[1] = input.substring(slice2 + 1);
        ret = 2;
      }
      // 3+ parameters
      else {
        output->p[1] = input.substring(slice2 + 1, slice3);
        if (slice4 == -1) {
          output->p[2] = input.substring(slice3 + 1);
          ret = 3;
        }
        // 4+ parameters
        else {
          output->p[2] = input.substring(slice3 + 1, slice4);
          if (slice5 == -1) {
            output->p[3] = input.substring(slice4 + 1);
            ret = 4;
          }
          // 5+ parameters
          else {
            output->p[3] = input.substring(slice4 + 1, slice5);
            if (slice6 == -1) {
              output->p[4] = input.substring(slice5 + 1);
              ret = 5;
            }
            // 6+ parameters
            else {
              output->p[4] = input.substring(slice5 + 1, slice6);
              if (slice7 == -1) {
                output->p[5] = input.substring(slice6 + 1);
                ret = 6;
              }
              // 7+ parameters
              else {
                output->p[5] = input.substring(slice6 + 1, slice7);
                if (slice8 == -1) {
                  output->trash = input.substring(slice7 + 1);
                  ret = 7;
                }
                // 8+ paramters
                else {
                  output->p[6] = input.substring(slice7 + 1, slice8);
                  if (slice8 == -1) {
                    output->trash = input.substring(slice8 + 1);
                    ret = 8;
                  }
                  // 9+ parameters
                  else {
                    output->p[7] = input.substring(slice8 + 1, slice9);
                    if (slice9 == -1) {
                      output->trash = input.substring(slice9 + 1);
                      ret = 9;
                    }
                    // 10+ parameters
                    else {
                      output->p[8] = input.substring(slice9 + 1, slice10);
                      if (slice10 == -1) {
                        output->trash = input.substring(slice10 + 1);
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
static enum serialMsg msgAbsVol(serialParams_t *p) {
  vol_value = (float)p->p[1].toInt() / ABS_VOL_MAX_VAL;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_VOL_LEVEL;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgLinkLoss(serialParams_t *p) {
  if (debug)
    snooze_usb.printf("Info:    link_ID: %s, status: %s\n", p->p[0].c_str(),
                      p->p[1].c_str());
  if (p->p[0].toInt() == BT_id_a2dp) {
    if (p->p[1].toInt() == 1) {
      working_state.mon_state = MONSTATE_REQ_OFF;
      working_state.bt_state = BTSTATE_DISCONNECTED;
    } else if (p->p[1].toInt() == 0) {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgName1(serialParams_t *p) {
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p->p[1].length();
    BT_peer_name = p->p[1].substring(1, (strlen - 1));
  } else {
    BT_peer_name = p->p[1];
  }
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgCloseOk(serialParams_t *p) {
  if (p->p[0].toInt() == BT_id_a2dp) {
    if (working_state.bt_state != BTSTATE_OFF)
      working_state.bt_state = BTSTATE_REQ_DISC;
  } else if (p->p[0].toInt() == BLE_conn_id) {
    if (working_state.ble_state != BLESTATE_OFF)
      working_state.ble_state = BLESTATE_REQ_DISC;
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgName2(serialParams_t *p) {
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p->p[2].length();
    BT_peer_name = p->p[1].substring(1) + "_" + p->p[2].substring(0, (strlen - 1));
  } else {
    BT_peer_name = p->p[1] + "_" + p->p[2];
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgOpenOk(serialParams_t *p) {
  if (p->p[1].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[2];
    if (debug)
      snooze_usb.printf(
          "Info:    A2DP connection opened. Conn ID: %d, peer address = %s\n",
          BT_id_a2dp, BT_peer_address.c_str());
    working_state.bt_state = BTSTATE_REQ_CONN;
    return BCCMD_BT_NAME;
  } else if (p->p[1].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[2];
    if (debug)
      snooze_usb.printf("Info:    AVRCP connection opened. Conn ID: %d, peer "
                        "address (check) = %s\n",
                        BT_id_avrcp, BT_peer_address.c_str());
    return BCCMD__NOTHING;
  } else if (p->p[1].equalsIgnoreCase("BLE")) {
    BLE_conn_id = p->p[0].toInt();
    // if(debug) snooze_usb.printf("Info:    BLE connection opened. Conn ID:
    // %d\n", BLE_conn_id);
    working_state.ble_state = BLESTATE_REQ_CONN;
    return BCCMD__NOTHING;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgRecv1(serialParams_t *p) {
  // - "inq"
  // - "disc"
  // - "latlong"
  enum serialMsg ret = BCCMD__NOTHING;

  if (p->p[0].toInt() == BLE_conn_id) {
    if (p->p[2].equalsIgnoreCase("inq")) {
      ret = BCCMD_INQUIRY;
    } else if (p->p[2].equalsIgnoreCase("disc")) {
      working_state.bt_state = BTSTATE_REQ_DISC;
    } else if (p->p[2].equalsIgnoreCase("latlong")) {
      if (debug)
        snooze_usb.println("Info:    Receiving latlong without values");
    }
  }

  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgInquiry1(serialParams_t *p) {
  String addr = p->p[0];
  String name;
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    name = p->p[1].substring(1, (p->p[1].length() - 1));
  } else {
    name = p->p[1];
  }
  String caps = p->p[2];
  unsigned int stren = p->p[3].substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgName3(serialParams_t *p) {
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p->p[3].length();
    BT_peer_name =
        p->p[1].substring(1) + "_" + p->p[2] + "_" + p->p[3].substring(0, (strlen - 1));
  } else {
    BT_peer_name = p->p[1] + "_" + p->p[2] + "_" + p->p[3];
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgState(serialParams_t *p) {
  if (!p->p[0].substring(p->p[0].length() - 2, p->p[0].length() - 1).toInt()) {
    return BCNOT_BT_STATE;
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgRecv2(serialParams_t *p) {
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
  if (p->p[0].toInt() == BLE_conn_id) {
    if (p->p[2].equalsIgnoreCase("conn")) {
      BT_peer_name = p->p[3];
      return BCCMD_DEV_CONNECT;
    } else if (p->p[2].equalsIgnoreCase("time")) {
      unsigned long rec_time = p->p[3].toInt();
      if (rec_time > MIN_TIME_DEC) {
        setCurTime(rec_time, TSOURCE_PHONE);
        if (debug)
          snooze_usb.printf("Info:    Timestamp received: %ld\n", rec_time);
      } else {
        if (debug)
          snooze_usb.println("Error: Received time not correct!");
      }
    } else if (p->p[2].equalsIgnoreCase("rec")) {
      if (p->p[3].equalsIgnoreCase("start"))
        return BCCMD_REC_START;
      else if (p->p[3].equalsIgnoreCase("stop"))
        return BCCMD_REC_STOP;
      else if (p->p[3].equalsIgnoreCase("?"))
        return BCNOT_REC_STATE;
    } else if (p->p[3].equalsIgnoreCase("rec_next")) {
      if (p->p[3].equalsIgnoreCase("?"))
        return BCNOT_REC_NEXT;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_next command not listed");
      }
    } else if (p->p[2].equalsIgnoreCase("rec_nb")) {
      if (p->p[3].equalsIgnoreCase("?"))
        return BCNOT_REC_NB;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_nb command not listed");
      }
    } else if (p->p[2].equalsIgnoreCase("rec_ts")) {
      if (p->p[3].equalsIgnoreCase("?"))
        return BCNOT_REC_TS;
      else {
        if (debug)
          snooze_usb.println("Error: BLE rec_ts command not listed");
      }
    } else if (p->p[2].equalsIgnoreCase("mon")) {
      if (p->p[3].equalsIgnoreCase("start"))
        working_state.mon_state = MONSTATE_REQ_ON;
      else if (p->p[3].equalsIgnoreCase("stop"))
        working_state.mon_state = MONSTATE_REQ_OFF;
      else if (p->p[3].equalsIgnoreCase("?"))
        return BCNOT_MON_STATE;
    } else if (p->p[2].equalsIgnoreCase("vol")) {
      if ((working_state.bt_state == BTSTATE_CONNECTED) ||
          (working_state.bt_state == BTSTATE_PLAY)) {
        if (p->p[3].equalsIgnoreCase("+")) {
          return BCCMD_VOL_UP;
        } else if (p->p[3].equalsIgnoreCase("-")) {
          return BCCMD_VOL_DOWN;
        } else if (p->p[3].equalsIgnoreCase("?")) {
          return BCNOT_VOL_LEVEL;
        }
      } else {
        return BCERR_VOL_BT_DIS;
      }
    } else if (p->p[2].equalsIgnoreCase("bt")) {
      if (p->p[3].equalsIgnoreCase("?")) {
        return BCCMD_STATUS;
      }
    } else if (p->p[2].equalsIgnoreCase("rwin")) {
      if (p->p[3].equalsIgnoreCase("?")) {
        return BCNOT_RWIN_VALS;
      } else {
        return BCERR_RWIN_BAD_REQ;
      }
    } else if (p->p[2].equalsIgnoreCase("filepath")) {
      if (p->p[3].equalsIgnoreCase("?")) {
        return BCNOT_FILEPATH;
      }
    } else if (p->p[2].equalsIgnoreCase("latlong")) {
      if (p->p[3].equalsIgnoreCase("?")) {
        return BCNOT_LATLONG;
      }
    }
  }
  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgInquiry2(serialParams_t *p) {
  String addr = p->p[0];
  String name;
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    name = p->p[1].substring(1) + "_" + p->p[2].substring(0, (p->p[2].length() - 1));
  } else {
    name = p->p[1] + "_" + p->p[2];
  }
  String caps = p->p[3];
  unsigned int stren = p->p[4].substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgLink1(serialParams_t *p) {
  if (p->p[2].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p->p[2].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgName4(serialParams_t *p) {
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    int strlen = p->p[4].length();
    BT_peer_name = p->p[1].substring(1) + "_" + p->p[2] + "_" + p->p[3] + "_" +
                   p->p[4].substring(0, (strlen - 1));
  } else {
    BT_peer_name = p->p[1] + "_" + p->p[2] + "_" + p->p[3] + "_" + p->p[4];
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD__NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgRecv3(serialParams_t *p) {
  // - "latlong {lat long}"
  if (p->p[0].toInt() == BLE_conn_id) {
    if (p->p[2].equalsIgnoreCase("latlong")) {
      snooze_usb.printf("Received latlong info: %f, %f", atof(p->p[3].c_str()),
                        atof(p->p[4].c_str()));
      if ((p->p[3].c_str() == NULL) || (p->p[4].c_str() == NULL)) {
        next_record.gps_source = GPS_NONE;
        next_record.gps_lat = 1000.0;
        next_record.gps_long = 1000.0;
      } else {
        next_record.gps_lat = atof(p->p[3].c_str());
        next_record.gps_long = atof(p->p[4].c_str());
        next_record.gps_source = GPS_PHONE;
      }
    }
  }
  return BCCMD__NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgInquiry3(serialParams_t *p) {
  String addr = p->p[0];
  String name;
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    name =
        p->p[1].substring(1) + "_" + p->p[2] + "_" + p->p[3].substring(0, (p->p[3].length() - 1));
  } else {
    name = p->p[1] + "_" + p->p[2] + "_" + p->p[3];
  }
  String caps = p->p[4];
  unsigned int stren = p->p[5].substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgRecv4(serialParams_t *p) {
  enum serialMsg ret = BCCMD__NOTHING;

  // - "rwin {duration} {period} {occurences}"
  if (p->p[0].toInt() == BLE_conn_id) {
    if (p->p[2].equalsIgnoreCase("rwin")) {
      unsigned int duration, period;
      duration = p->p[3].toInt();
      period = p->p[4].toInt();
      if (duration < period) {
        breakTime(duration, rec_window.duration);
        breakTime(period, rec_window.period);
        rec_window.occurences = p->p[5].toInt();
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
static enum serialMsg msgLink2(serialParams_t *p) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p->p[2].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p->p[2].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgInquiry4(serialParams_t *p) {
  String addr = p->p[0];
  String name;
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    name = p->p[1].substring(1) + "_" + p->p[2] + "_" + p->p[3] + "_" +
           p->p[4].substring(0, (p->p[4].length() - 1));
  } else {
    name = p->p[1] + "_" + p->p[2] + "_" + p->p[3] + "_" + p->p[4];
  }
  String caps = p->p[5];
  unsigned int stren = p->p[6].substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgLink3(serialParams_t *p) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p->p[2].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p->p[2].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgInquiry5(serialParams_t *p) {
  String addr = p->p[0];
  String name;
  if (p->p[1].substring(0, 1).equalsIgnoreCase("\"")) {
    name = p->p[1].substring(1) + "_" + p->p[2] + "_" + p->p[3] + "_" + p->p[4] + "_" +
           p->p[5].substring(0, (p->p[5].length() - 1));
  } else {
    name = p->p[1] + "_" + p->p[2] + "_" + p->p[3] + "_" + p->p[4] + "_" + p->p[5];
  }
  String caps = p->p[6];
  unsigned int stren = p->p[7].substring(1, 3).toInt();
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgLink4(serialParams_t *p) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p->p[2].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p->p[2].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsg msgLink5(serialParams_t *p) {
  enum serialMsg ret = BCCMD__NOTHING;

  if (p->p[2].equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_a2dp);
    working_state.bt_state = BTSTATE_CONNECTED;
    return BCCMD_BT_NAME;
  } else if (p->p[2].equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p[0].toInt();
    BT_peer_address = p->p[3];
    if (debug)
      snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                        BT_peer_address.c_str(), BT_id_avrcp);
    working_state.bt_state = BTSTATE_CONNECTED;
  }
  return ret;
}
/*****************************************************************************/

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

/*****************************************************************************/
static String notBtState(void) {
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
/*****************************************************************************/

/*****************************************************************************/
static String notVolLevel(void) {
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return ("SEND " + String(BLE_conn_id) + " VOL " + String(vol_value) + "\r");
  } else
    return "";
}
/*****************************************************************************/

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
  bc127BlueOff();
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
/* bc1267BlueOff(void)
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
void bc127AdvStop(void) { sendCmdOut(BCCMD_ADV_OFF); }
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
    snooze_usb.printf("<-BC127: %s\n", input.c_str());

  serialParams_t params;
  unsigned int nb_params = countParams(input, &params);

  switch (nb_params) {
  // INQU_OK
  // READY
  case 0: {
    if (params.notif.equalsIgnoreCase("INQU_OK"))
      return BCNOT_INQ_DONE;
    else if (params.notif.equalsIgnoreCase("READY"))
      BC127_ready = true;
    break;
  }

  // AVRCP_PLAY [link_ID]
  // AVRCP_PAUSE [link_ID]
  case 1: {
    if (params.notif.equalsIgnoreCase("AVRCP_PLAY"))
      return msgAvrcpPlay();
    else if (params.notif.equalsIgnoreCase("AVRCP_PAUSE"))
      return msgAvrcpPause();
    break;
  }

  // ABS_VOL [link_ID](value)
  // LINK_LOSS [link_ID] (status)
  // NAME [addr] {"name" || name}
  case 2: {
    if (params.notif.equalsIgnoreCase("ABS_VOL"))
      return msgAbsVol(&params);
    else if (params.notif.equalsIgnoreCase("LINK_LOSS"))
      return msgLinkLoss(&params);
    else if (params.notif.equalsIgnoreCase("NAME"))
      return msgName1(&params);
    break;
  }

  // CLOSE_OK [link_ID] (profile) (Bluetooth address)
  // NAME [addr] {"n1 n2" || n1 n2}
  // OPEN_OK [link_ID] (profile) (Bluetooth address)
  // RECV [link_ID] (size) (report data)
  case 3: {
    if (params.notif.equalsIgnoreCase("CLOSE_OK"))
      return msgCloseOk(&params);
    else if (params.notif.equalsIgnoreCase("NAME"))
      return msgName2(&params);
    else if (params.notif.equalsIgnoreCase("OPEN_OK"))
      return msgOpenOk(&params);
    else if (params.notif.equalsIgnoreCase("RECV"))
      return msgRecv1(&params);
    break;
  }

  // INQUIRY(BTADDR) {"name" || name} (COD) (RSSI)
  // NAME [addr] {"n1 n2 n3" || n1 n2 n3}
  // STATE (connected) (connectable) (discoverable) (ble)
  // RECV [link_ID] (size) (report data) <-- (report data) with 2 parameters
  case 4: {
    if (params.notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry1(&params);
    else if (params.notif.equalsIgnoreCase("NAME"))
      return msgName3(&params);
    else if (params.notif.equalsIgnoreCase("STATE"))
      return msgState(&params);
    else if (params.notif.equalsIgnoreCase("RECV"))
      return msgRecv2(&params);
    break;
  }

  // INQUIRY (BTADDR) {"n1 n2" || n1 n2} (COD) (RSSI)
  // LINK [link_ID] (STATE) (PROFILE) (BTADDR) (INFO)
  // NAME [addr] {"n1 n2 n3 n4" || n1 n2 n3 n4}
  // RECV [link_ID] (size) (report data) <-- (report data) with 3 parameters
  case 5: {
    if (params.notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry2(&params);
    else if (params.notif.equalsIgnoreCase("LINK"))
      return msgLink1(&params);
    else if (params.notif.equalsIgnoreCase("NAME"))
      return msgName4(&params);
    else if (params.notif.equalsIgnoreCase("RECV"))
      return msgRecv3(&params);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3" || n1 n2 n3} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2)
  // RECV [link_ID] (size) (report data) <-- (report data) with 4 parameters
  case 6: {
    if (params.notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry3(&params);
    else if (params.notif.equalsIgnoreCase("RECV"))
      return msgRecv4(&params);
    else if (params.notif.equalsIgnoreCase("LINK"))
      return msgLink2(&params);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3 n4" || n1 n2 n3 n4} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3)
  case 7: {
    if (params.notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry4(&params);
    else if (params.notif.equalsIgnoreCase("LINK"))
      return msgLink3(&params);
    break;
  }

  // INQUIRY [addr] {"n1 n2 n3 n4 n5" || n1 n2 n3 n4 n5} (COD) (RSSI)
  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4)
  case 8: {
    if (params.notif.equalsIgnoreCase("INQUIRY"))
      return msgInquiry5(&params);
    else if (params.notif.equalsIgnoreCase("LINK"))
      return msgLink4(&params);
    break;
  }

  // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4)
  // (info5)
  case 9: {
    if (params.notif.equalsIgnoreCase("LINK"))
      return msgLink5(&params);
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
    cmdLine = "STATUS\r";
    break;
  // Volume level
  case BCCMD_VOL_A2DP:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " 7\r";
    break;
  // Volume up -> AVRCP volume up
  case BCCMD_VOL_UP:
    cmdLine = "VOLUME " + String(BT_id_a2dp) + " UP\r";
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
