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
  String p1;
  String p2;
  String p3;
  String p4;
  String p5;
  String p6;
  String p7;
  String p8;
  String p9;
  String trash;
} serialParams_t;
typedef struct {
  String cmd;
  String param;
} serialAnswer_t;

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
static void populateDevlist(String addr, String name, String caps,
                            unsigned int stren);
static unsigned int sliceParams(String input, serialParams_t *output);
static bool searchDevlist(String name);
static enum serialMsgs readMsg(String notif);
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
static unsigned int sliceParams(String input, serialParams_t *output) {
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
      output->p1 = input.substring(slice1 + 1);
      ret = 1;
    }
    // 2+ parameters
    else {
      output->p1 = input.substring(slice1 + 1, slice2);
      if (slice3 == -1) {
        output->p2 = input.substring(slice2 + 1);
        ret = 2;
      }
      // 3+ parameters
      else {
        output->p2 = input.substring(slice2 + 1, slice3);
        if (slice4 == -1) {
          output->p3 = input.substring(slice3 + 1);
          ret = 3;
        }
        // 4+ parameters
        else {
          output->p3 = input.substring(slice3 + 1, slice4);
          if (slice5 == -1) {
            output->p4 = input.substring(slice4 + 1);
            ret = 4;
          }
          // 5+ parameters
          else {
            output->p4 = input.substring(slice4 + 1, slice5);
            if (slice6 == -1) {
              output->p5 = input.substring(slice5 + 1);
              ret = 5;
            }
            // 6+ parameters
            else {
              output->p5 = input.substring(slice5 + 1, slice6);
              if (slice7 == -1) {
                output->p6 = input.substring(slice6 + 1);
                ret = 6;
              }
              // 7+ parameters
              else {
                output->p6 = input.substring(slice6 + 1, slice7);
                if (slice8 == -1) {
                  output->trash = input.substring(slice7 + 1);
                  ret = 7;
                }
                // 8+ paramters
                else {
                  output->p7 = input.substring(slice7 + 1, slice8);
                  if (slice8 == -1) {
                    output->trash = input.substring(slice8 + 1);
                    ret = 8;
                  }
                  // 9+ parameters
                  else {
                    output->p8 = input.substring(slice8 + 1, slice9);
                    if (slice9 == -1) {
                      output->trash = input.substring(slice9 + 1);
                      ret = 9;
                    }
                    // 10+ parameters
                    else {
                      output->p9 = input.substring(slice9 + 1, slice10);
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
static enum serialMsgs readMsg(String in) {
  if (in.equalsIgnoreCase("A2DP_STREAM_START"))
    return BCNOT_A2DP_STREAM_START;
  else if (in.equalsIgnoreCase("A2DP_STREAM_SUSPEND"))
    return BCNOT_A2DP_STREAM_SUSPEND;
  else if (in.equalsIgnoreCase("ABS_VOL"))
    return BCNOT_ABS_VOL;
  else if (in.equalsIgnoreCase("ASSOCIATION"))
    return BCNOT_ASSOCIATION;
  else if (in.equalsIgnoreCase("ASSOCIATION_IN_PROGRESS"))
    return BCNOT_ASSOCIATION_IN_PROGRESS;
  else if (in.equalsIgnoreCase("AT"))
    return BCNOT_AT;
  else if (in.equalsIgnoreCase("AVRCP_BACKWARD"))
    return BCNOT_AVRCP_BACKWARD;
  else if (in.equalsIgnoreCase("AVRCP_FORWARD"))
    return BCNOT_AVRCP_FORWARD;
  else if (in.equalsIgnoreCase("AVRCP_MEDIA"))
    return BCNOT_AVRCP_MEDIA;
  else if (in.equalsIgnoreCase("AVRCP_PAUSE"))
    return BCNOT_AVRCP_PAUSE;
  else if (in.equalsIgnoreCase("AVRCP_PLAY"))
    return BCNOT_AVRCP_PLAY;
  else if (in.equalsIgnoreCase("AVRCP_STOP"))
    return BCNOT_AVRCP_STOP;
  else if (in.equalsIgnoreCase("BA_BROADCASTER_START"))
    return BCNOT_BA_BROADCASTER_START;
  else if (in.equalsIgnoreCase("BA_BROADCASTER_STOP"))
    return BCNOT_BA_BROADCASTER_STOP;
  else if (in.equalsIgnoreCase("BA_RECEIVER_START"))
    return BCNOT_BA_RECEIVER_START;
  else if (in.equalsIgnoreCase("BA_RECEIVER_STOP"))
    return BCNOT_BA_RECEIVER_STOP;
  else if (in.equalsIgnoreCase("BC_SMART_CMD"))
    return BCNOT_BC_SMART_CMD;
  else if (in.equalsIgnoreCase("BC_SMART_CMD_RESP"))
    return BCNOT_BC_SMART_CMD_RESP;
  else if (in.equalsIgnoreCase("BLE_INDICATION"))
    return BCNOT_BLE_INDICATION;
  else if (in.equalsIgnoreCase("BLE_NOTIFICATION"))
    return BCNOT_BLE_NOTIFICATION;
  else if (in.equalsIgnoreCase("BLE_PAIR_ERROR"))
    return BCNOT_BLE_PAIR_ERROR;
  else if (in.equalsIgnoreCase("BLE_PAIR_OK"))
    return BCNOT_BLE_PAIR_OK;
  else if (in.equalsIgnoreCase("BLE_READ"))
    return BCNOT_BLE_READ;
  else if (in.equalsIgnoreCase("BLE_WRITE"))
    return BCNOT_BLE_WRITE;
  else if (in.equalsIgnoreCase("CALL_ACTIVE"))
    return BCNOT_CALL_ACTIVE;
  else if (in.equalsIgnoreCase("CALL_DIAL"))
    return BCNOT_CALL_DIAL;
  else if (in.equalsIgnoreCase("CALL_END"))
    return BCNOT_CALL_END;
  else if (in.equalsIgnoreCase("CALL_INCOMING"))
    return BCNOT_CALL_INCOMING;
  else if (in.equalsIgnoreCase("CALL_MEMORY"))
    return BCNOT_CALL_MEMORY;
  else if (in.equalsIgnoreCase("CALL_OUTGOING"))
    return BCNOT_CALL_OUTGOING;
  else if (in.equalsIgnoreCase("CALL_REDIAL"))
    return BCNOT_CALL_REDIAL;
  else if (in.equalsIgnoreCase("CALLER_NUMBER"))
    return BCNOT_CALLER_NUMBER;
  else if (in.equalsIgnoreCase("CHARGING_IN_PROGRESS"))
    return BCNOT_CHARGING_IN_PROGRESS;
  else if (in.equalsIgnoreCase("CHARGING_COMPLETE"))
    return BCNOT_CHARGING_COMPLETE;
  else if (in.equalsIgnoreCase("CHARGER_DISCONNECTED"))
    return BCNOT_CHARGER_DISCONNECTED;
  else if (in.equalsIgnoreCase("CLOSE_OK"))
    return BCNOT_CLOSE_OK;
  else if (in.equalsIgnoreCase("DTMF"))
    return BCNOT_DTMF;
  else if (in.equalsIgnoreCase("ERROR"))
    return BCNOT_ERROR;
  else if (in.equalsIgnoreCase("IAP_CLOSE_SESSION"))
    return BCNOT_IAP_CLOSE_SESSION;
  else if (in.equalsIgnoreCase("IAP_OPEN_SESSION"))
    return BCNOT_IAP_OPEN_SESSION;
  else if (in.equalsIgnoreCase("INBAND_RING"))
    return BCNOT_INBAND_RING;
  else if (in.equalsIgnoreCase("LINK_LOSS"))
    return BCNOT_LINK_LOSS;
  else if (in.equalsIgnoreCase("MAP_NEW_MSG"))
    return BCNOT_MAP_NEW_MSG;
  else if (in.equalsIgnoreCase("OPEN_OK"))
    return BCNOT_OPEN_OK;
  else if (in.equalsIgnoreCase("PAIR_ERROR"))
    return BCNOT_PAIR_ERROR;
  else if (in.equalsIgnoreCase("PAIR_OK"))
    return BCNOT_PAIR_OK;
  else if (in.equalsIgnoreCase("PAIR_PASSKEY"))
    return BCNOT_PAIR_PASSKEY;
  else if (in.equalsIgnoreCase("PAIR_PENDING"))
    return BCNOT_PAIR_PENDING;
  else if (in.equalsIgnoreCase("NOT_RECV"))
    return BCNOT_RECV;
  else if (in.equalsIgnoreCase("NOT_REMOTE_VOLUME"))
    return BCNOT_REMOTE_VOLUME;
  else if (in.equalsIgnoreCase("NOT_ROLE"))
    return BCNOT_ROLE;
  else if (in.equalsIgnoreCase("NOT_ROLE_OK"))
    return BCNOT_ROLE_OK;
  else if (in.equalsIgnoreCase("NOT_ROLE_NOT_ALLOWED"))
    return BCNOT_ROLE_NOT_ALLOWED;
  else if (in.equalsIgnoreCase("NOT_SCO_OPEN"))
    return BCNOT_SCO_OPEN;
  else if (in.equalsIgnoreCase("NOT_SCO_CLOSE"))
    return BCNOT_SCO_CLOSE;
  else if (in.equalsIgnoreCase("NOT_SR"))
    return BCNOT_SR;
  else
    return BCNOT_UNKNOWN;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs readBle(String in) {
  enum bc127Cmds ret = BLE_NOTHING;

  if(in.equalsIgnoreCase("inq"))
    ret = BLECMD_INQUIRY;
  else if(in.equalsIgnoreCase("disc"))
    ret = BLECMD_DISC;
  else if(in.equalsIgnoreCase("latlong"))
    ret = BLECMD_LATLONG;
  else if(in.equalsIgnoreCase("conn"))
    ret = BLECMD_CONN;
  else if(in.equalsIgnoreCase("time"))
    ret = BLECMD_TIME;
  else if(in.equalsIgnoreCase("rec"))
    ret = BLECMD_REC;
  else if(in.equalsIgnoreCase("rec_next"))
    ret = BLECMD_REC_NEXT;
  else if(in.equalsIgnoreCase("rec_nb"))
    ret = BLECMD_REC_NB;
  else if(in.equalsIgnoreCase("rec_ts"))
    ret = BLECMD_REC_TS;
  else if(in.equalsIgnoreCase("mon"))
    ret = BLECMD_MON;
  else if(in.equalsIgnoreCase("vol"))
    ret = BLECMD_VOL;
  else if(in.equalsIgnoreCase("bt"))
    ret = BLECMD_BT;
  else if(in.equalsIgnoreCase("rwin"))
    ret = BLECMD_RWIN;
  else if(in.equalsIgnoreCase("filepath"))
    ret = BLECMD_FILEPATH;
  else
    ret = BLECMD_UNKNOWN;

  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgName(serialParams_t *p, unsigned int len) {
  int strlen;
  switch (len) {
  case 2: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      strlen = p->p2.length();
      BT_peer_name = p->p2.substring(1, (strlen - 1));
    } else {
      BT_peer_name = p->p2;
    }
    break;
  }
  case 3: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      strlen = p->p3.length();
      BT_peer_name =
          p->p2.substring(1) + "_" + p->p3.substring(0, (strlen - 1));
    } else {
      BT_peer_name = p->p2 + "_" + p->p3;
    }
    break;
  }
  case 4: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      strlen = p->p4.length();
      BT_peer_name = p->p2.substring(1) + "_" + p->p3 + "_" +
                     p->p4.substring(0, (strlen - 1));
    } else {
      BT_peer_name = p->p2 + "_" + p->p3 + "_" + p->p4;
    }
    break;
  }
  case 5: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      strlen = p->p5.length();
      BT_peer_name = p->p2.substring(1) + "_" + p->p3 + "_" + p->p4 + "_" +
                     p->p5.substring(0, (strlen - 1));
    } else {
      BT_peer_name = p->p2 + "_" + p->p3 + "_" + p->p4 + "_" + p->p5;
    }
    break;
  }
  default:
    break;
  }

  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BCNOT_BT_STATE;
  } else {
    return BCCMD_NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs bcnotRecv(serialParams_t *p, unsigned int len, serialAnswer_t *answer) {
  enum serialMsgs ret = BLENOT_NOTHING;
  if (p->p1.toInt() == BLE_conn_id) {
    enum bc127Ble cmd = readBle(p->p3);
    switch(cmd) {
      case BLECMD_INQUIRY: {
        ret = BCCMD_INQUIRY;
        break;
      }
      case BLECMD_DISC: {
        working_state.bt_state = BTSTATE_REQ_DISC;
        ret = BLENOT_OK;
        break;
      }
      case BLECMD_CONN: {
        BT_peer_name = p->p4;
        ret = BCCMD_OPEN;
        break;
      }
      case BLECMD_LATLONG: {
        if ((len == 4) && (p->p4.equalsIgnoreCase("?"))) {
          ret = BLENOT_LATLONG;
        }
        else if(len == 5) {
          if((p->p4.c_str() != NULL) && (p->p5.c_str() != NULL)) {
            next_record.gps_lat = atof(p->p4.c_str());
            next_record.gps_long = atof(p->p5.c_str());
            next_record.gps_source = GPS_PHONE;
            ret = BLENOT_OK;
          }
          else {
            next_record.gps_lat = 1000.0;
            next_record.gps_long = 1000.0;
            next_record.gps_source = GPS_NONE;
            ret = BLEERR_WRONG_PARAMS;
          }
        }
        else {
          ret = BLEERR_WRONG_PARAMS;
        }
        break;
      }
      case BLECMD_TIME: {
        unsigned long rec_time = p->p4.toInt();
        if (rec_time > MIN_TIME_DEC) {
          setCurTime(rec_time, TSOURCE_PHONE);
          if (debug)
            snooze_usb.printf("Info:    Timestamp received: %ld\n", rec_time);
        } else {
          if (debug)
            snooze_usb.println("Error: Received time not correct!");
        }
        ret = BLENOT_OK;
        break;
      }
      case BLECMD_REC: {
        if(p->p4.equalsIgnoreCase("start")) {
          working_state.rec_state = RECSTATE_REQ_ON;
          ret = BLENOT_OK;
        }
        else if(p->p4.equalsIgnoreCase("stop")) {
          working_state.rec_state = RECSTATE_REQ_OFF;
          ret = BLENOT_OK;
        }
        else if(p->p4.equalsIgnoreCase("?")) {
          ret = BLENOT_REC_STATE;
        }
        else {
          ret = BLEERR_WRONG_PARAMS;
        }
        break;
      }
      case BLECMD_REC_NB: {
        if(p->p4.equalsIgnoreCase("?")) ret = BLENOT_REC_NB;
        else ret = BLEERR_WRONG_PARAMS;
        break;
      }
      case BLECMD_REC_NEXT: {
        if(p->p4.equalsIgnoreCase("?")) ret = BLENOT_REC_NEXT;
        else ret = BLEERR_WRONG_PARAMS;
        break;
      }
      case BLECMD_REC_TS: {
        if(p->p4.equalsIgnoreCase("?")) ret = BLENOT_REC_TS;
        else ret = BLEERR_WRONG_PARAMS;
        break;
      }
      case BLECMD_MON: {
        if(p->p4.equalsIgnoreCase("start")) {
          working_state.mon_state = MONSTATE_REQ_ON;
          ret = BLENOT_OK;
        }
        else if(p->p4.equalsIgnoreCase("stop")) {
          working_state.mon_state = MONSTATE_REQ_OFF;
          ret = BLENOT_OK;
        }
        else if(p->p4.equalsIgnoreCase("?")) {
          ret = BLENOT_MON_STATE;
        }
        else {
          ret = BLEERR_WRONG_PARAMS;
        }
        break;
      }
      case BLECMD_VOL: {
        if ((working_state.bt_state == BTSTATE_CONNECTED) ||
            (working_state.bt_state == BTSTATE_PLAY)) {
          if (p->p4.equalsIgnoreCase("+")) ret = BCCMD_VOLUME;
          else if (p->p4.equalsIgnoreCase("-")) ret = BCCMD_VOLUME;
          else if (p->p4.equalsIgnoreCase("?")) ret = BLENOT_VOL;
          else ret = BLEERR_WRONG_PARAMS;
        } else {
          ret = BLEERR_VOL_BT_DIS;
        }
        break;
      }
      case BLECMD_BT: {
        if (p->p4.equalsIgnoreCase("?")) ret = BCCMD_STATUS;
        else ret = BLEERR_WRONG_PARAMS;
        break;
      }
      case BLECMD_RWIN: {
        if (p->p4.equalsIgnoreCase("?")) {
          ret = BLENOT_RWIN;
        }
        else if(len == 6) {
          unsigned int duration, period;
          duration = p->p4.toInt();
          period = p->p5.toInt();
          if (duration < period) {
            breakTime(duration, rec_window.duration);
            breakTime(period, rec_window.period);
            rec_window.occurences = p->p6.toInt();
            ret = BLENOT_OK;
          } else {
            ret = BLEERR_RWIN_BAD_REQ;
          }
        }
        else {
          ret = BLEERR_RWIN_WRONG_PARAMS;
        }
        break;
      }
      case BLECMD_FILEPATH: {
        if (p->p4.equalsIgnoreCase("?")) ret = BLENOT_FILEPATH;
        else ret = BLEERR_WRONG_PARAMS;
        break;
      }
      case BLECMD_NOTHING:
      case BLECMD_UNKNOWN:
      default:
        break;
    }

    return ret;
  }
}
    // switch (len) {
    // case 3: {
    //   if (p->p3.equalsIgnoreCase("inq")) {
    //     ret = BCCMD_INQUIRY;
    //   } else if (p->p3.equalsIgnoreCase("disc")) {
    //     working_state.bt_state = BTSTATE_REQ_DISC;
    //   } else if (p->p3.equalsIgnoreCase("latlong")) {
    //     if (debug)
    //       snooze_usb.println("Info:    Receiving latlong without values");
    //   }
    //   break;
    // }
    // case 4: {
    //   if (p->p3.equalsIgnoreCase("conn")) {
    //     BT_peer_name = p->p4;
    //     return BCCMD_DEV_CONNECT;
      // } else if (p->p3.equalsIgnoreCase("time")) {
      //   unsigned long rec_time = p->p4.toInt();
      //   if (rec_time > MIN_TIME_DEC) {
      //     setCurTime(rec_time, TSOURCE_PHONE);
      //     if (debug)
      //       snooze_usb.printf("Info:    Timestamp received: %ld\n", rec_time);
      //   } else {
      //     if (debug)
      //       snooze_usb.println("Error: Received time not correct!");
      //   }
      // } else if (p->p3.equalsIgnoreCase("rec")) {
      //   if (p->p4.equalsIgnoreCase("start"))
      //     return BCCMD_REC_START;
      //   else if (p->p4.equalsIgnoreCase("stop"))
      //     return BCCMD_REC_STOP;
      //   else if (p->p4.equalsIgnoreCase("?"))
      //     return BCNOT_REC_STATE;
      // } else if (p->p3.equalsIgnoreCase("rec_next")) {
      //   if (p->p4.equalsIgnoreCase("?"))
      //     return BCNOT_REC_NEXT;
      //   else {
      //     if (debug)
      //       snooze_usb.println("Error: BLE rec_next command not listed");
      //   }
      // } else if (p->p3.equalsIgnoreCase("rec_nb")) {
      //   if (p->p4.equalsIgnoreCase("?"))
      //     return BCNOT_REC_NB;
      //   else {
      //     if (debug)
      //       snooze_usb.println("Error: BLE rec_nb command not listed");
      //   }
      // } else if (p->p3.equalsIgnoreCase("rec_ts")) {
      //   if (p->p4.equalsIgnoreCase("?"))
      //     return BCNOT_REC_TS;
      //   else {
      //     if (debug)
      //       snooze_usb.println("Error: BLE rec_ts command not listed");
      //   }
      // } else if (p->p3.equalsIgnoreCase("mon")) {
      //   if (p->p4.equalsIgnoreCase("start"))
      //     working_state.mon_state = MONSTATE_REQ_ON;
      //   else if (p->p4.equalsIgnoreCase("stop"))
      //     working_state.mon_state = MONSTATE_REQ_OFF;
      //   else if (p->p4.equalsIgnoreCase("?"))
      //     return BCNOT_MON_STATE;
      // } else if (p->p3.equalsIgnoreCase("vol")) {
      //   if ((working_state.bt_state == BTSTATE_CONNECTED) ||
      //       (working_state.bt_state == BTSTATE_PLAY)) {
      //     if (p->p4.equalsIgnoreCase("+")) {
      //       return BCCMD_VOL_UP;
      //     } else if (p->p4.equalsIgnoreCase("-")) {
      //       return BCCMD_VOL_DOWN;
      //     } else if (p->p4.equalsIgnoreCase("?")) {
      //       return BCNOT_VOL_LEVEL;
      //     }
      //   } else {
      //     return BCERR_VOL_BT_DIS;
      //   }
      // } else if (p->p3.equalsIgnoreCase("bt")) {
      //   if (p->p4.equalsIgnoreCase("?")) {
      //     return BCCMD_STATUS;
      //   }
      // } else if (p->p3.equalsIgnoreCase("rwin")) {
      //   if (p->p4.equalsIgnoreCase("?")) {
      //     return BCNOT_RWIN_VALS;
      //   } else {
      //     return BCERR_RWIN_BAD_REQ;
      //   }
      // } else if (p->p3.equalsIgnoreCase("filepath")) {
      //   if (p->p4.equalsIgnoreCase("?")) {
      //     return BCNOT_FILEPATH;
      //   }
      // } else if (p->p3.equalsIgnoreCase("latlong")) {
      //   if (p->p4.equalsIgnoreCase("?")) {
      //     return BCNOT_LATLONG;
      //   }
      // }
      // break;
    // }
    // case 5: {
    //   if (p->p3.equalsIgnoreCase("latlong")) {
    //     snooze_usb.printf("Received latlong info: %f, %f", atof(p->p4.c_str()),
    //                       atof(p->p5.c_str()));
    //     if ((p->p4.c_str() == NULL) || (p->p5.c_str() == NULL)) {
    //       next_record.gps_source = GPS_NONE;
    //       next_record.gps_lat = 1000.0;
    //       next_record.gps_long = 1000.0;
    //     } else {
    //       next_record.gps_lat = atof(p->p4.c_str());
    //       next_record.gps_long = atof(p->p5.c_str());
    //       next_record.gps_source = GPS_PHONE;
    //     }
    //   }
    //   break;
    // }
    // case 6: {
    //   if (p->p3.equalsIgnoreCase("rwin")) {
    //     unsigned int dur, per;
    //     dur = p->p4.toInt();
    //     per = p->p5.toInt();
    //     if (dur < per) {
    //       breakTime(dur, rec_window.duration);
    //       breakTime(per, rec_window.period);
    //       rec_window.occurences = p->p6.toInt();
    //       ret = BCNOT_RWIN_OK;
    //     } else {
    //       ret = BCERR_RWIN_WRONG_PARAMS;
    //     }
    //   }
    //   break;
    // }
    // default:
    //   break;
    // }
  //   return ret;
  // }
// }
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgInquiry(serialParams_t *p, unsigned int len) {
  String addr = p->p1;
  String name, caps;
  unsigned int stren;
  switch (len) {
  case 4: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      name = p->p2.substring(1, (p->p2.length() - 1));
    } else {
      name = p->p2;
    }
    caps = p->p3;
    stren = p->p4.substring(1, 3).toInt();
    break;
  }
  case 5: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      name =
          p->p2.substring(1) + "_" + p->p3.substring(0, (p->p3.length() - 1));
    } else {
      name = p->p2 + "_" + p->p3;
    }
    caps = p->p4;
    stren = p->p5.substring(1, 3).toInt();
    break;
  }
  case 6: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      name = p->p2.substring(1) + "_" + p->p3 + "_" +
             p->p4.substring(0, (p->p4.length() - 1));
    } else {
      name = p->p2 + "_" + p->p3 + "_" + p->p4;
    }
    caps = p->p5;
    stren = p->p6.substring(1, 3).toInt();
    break;
  }
  case 7: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      name = p->p2.substring(1) + "_" + p->p3 + "_" + p->p4 + "_" +
             p->p5.substring(0, (p->p5.length() - 1));
    } else {
      name = p->p2 + "_" + p->p3 + "_" + p->p4 + "_" + p->p5;
    }
    caps = p->p6;
    stren = p->p7.substring(1, 3).toInt();
    break;
  }
  case 8: {
    if (p->p2.substring(0, 1).equalsIgnoreCase("\"")) {
      name = p->p2.substring(1) + "_" + p->p3 + "_" + p->p4 + "_" + p->p5 +
             "_" + p->p6.substring(0, (p->p6.length() - 1));
    } else {
      name = p->p2 + "_" + p->p3 + "_" + p->p4 + "_" + p->p5 + "_" + p->p6;
    }
    caps = p->p7;
    stren = p->p8.substring(1, 3).toInt();
    break;
  }
  default:
    break;
  }
  populateDevlist(addr, name, caps, stren);
  return BCNOT_INQ_STATE;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgLink(serialParams_t *p, unsigned int len) {
  enum serialMsgs ret = BCCMD_NOTHING;

  switch (len) {
  case 4: {
    if (p->p3.equalsIgnoreCase("A2DP")) {
      BT_id_a2dp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_a2dp);
      working_state.bt_state = BTSTATE_CONNECTED;
      return CMD_NAME;
    } else if (p->p3.equalsIgnoreCase("AVRCP")) {
      BT_id_avrcp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_avrcp);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    break;
  }
  case 5: {
    if (p->p3.equalsIgnoreCase("A2DP")) {
      BT_id_a2dp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_a2dp);
      working_state.bt_state = BTSTATE_CONNECTED;
      return CMD_NAME;
    } else if (p->p3.equalsIgnoreCase("AVRCP")) {
      BT_id_avrcp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_avrcp);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    break;
  }
  case 6: {
    if (p->p3.equalsIgnoreCase("A2DP")) {
      BT_id_a2dp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_a2dp);
      working_state.bt_state = BTSTATE_CONNECTED;
      return CMD_NAME;
    } else if (p->p3.equalsIgnoreCase("AVRCP")) {
      BT_id_avrcp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_avrcp);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    break;
  }
  case 7: {
    if (p->p3.equalsIgnoreCase("A2DP")) {
      BT_id_a2dp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_a2dp);
      working_state.bt_state = BTSTATE_CONNECTED;
      return CMD_NAME;
    } else if (p->p3.equalsIgnoreCase("AVRCP")) {
      BT_id_avrcp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_avrcp);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    break;
  }
  case 8: {
    if (p->p3.equalsIgnoreCase("A2DP")) {
      BT_id_a2dp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    A2DP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_a2dp);
      working_state.bt_state = BTSTATE_CONNECTED;
      return CMD_NAME;
    } else if (p->p3.equalsIgnoreCase("AVRCP")) {
      BT_id_avrcp = p->p1.toInt();
      BT_peer_address = p->p4;
      if (debug)
        snooze_usb.printf("Info:    AVRCP address: %s, ID: %d\n",
                          BT_peer_address.c_str(), BT_id_avrcp);
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    break;
  }
  default: {
    break;
  }
  }
  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs bcnotAvrcpPlay(void) {
  working_state.mon_state = MONSTATE_REQ_ON;
  return BLENOT_NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs bcnotAvrcpPause(void) {
  working_state.mon_state = MONSTATE_REQ_OFF;
  return BLENOT_NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs bcnotAbsVol(serialParams_t *p) {
  vol_value = (float)p->p2.toInt() / ABS_VOL_MAX_VAL;
  if (working_state.ble_state == BLESTATE_CONNECTED) {
    return BLENOT_VOL;
  } else {
    return BLENOT_NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgLinkLoss(serialParams_t *p) {
  if (debug)
    snooze_usb.printf("Info:    link_ID: %s, status: %s\n", p->p1.c_str(),
                      p->p2.c_str());

  if (p->p1.toInt() == BT_id_a2dp) {
    if (p->p2.toInt() == 1) {
      working_state.mon_state = MONSTATE_REQ_OFF;
      working_state.bt_state = BTSTATE_DISCONNECTED;
    } else if (p->p2.toInt() == 0) {
      working_state.bt_state = BTSTATE_CONNECTED;
    }
    return BCNOT_BT_STATE;
  } else {
    return BCCMD_NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs bcnotCloseOk(serialParams_t *p) {
  if (p->p1.toInt() == BT_id_a2dp) {
    if (working_state.bt_state != BTSTATE_OFF)
      working_state.bt_state = BTSTATE_REQ_DISC;
  } else if (p->p1.toInt() == BLE_conn_id) {
    if (working_state.ble_state != BLESTATE_OFF)
      working_state.ble_state = BLESTATE_REQ_DISC;
  }
  return BLENOT_NOTHING;
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgOpenOk(serialParams_t *p) {
  if (p->p2.equalsIgnoreCase("A2DP")) {
    BT_id_a2dp = p->p1.toInt();
    BT_peer_address = p->p3;
    if (debug)
      snooze_usb.printf(
          "Info:    A2DP connection opened. Conn ID: %d, peer address = %s\n",
          BT_id_a2dp, BT_peer_address.c_str());
    working_state.bt_state = BTSTATE_REQ_CONN;
    return CMD_NAME;
  } else if (p->p2.equalsIgnoreCase("AVRCP")) {
    BT_id_avrcp = p->p1.toInt();
    BT_peer_address = p->p3;
    if (debug)
      snooze_usb.printf("Info:    AVRCP connection opened. Conn ID: %d, peer "
                        "address (check) = %s\n",
                        BT_id_avrcp, BT_peer_address.c_str());
    return BCCMD_NOTHING;
  } else if (p->p2.equalsIgnoreCase("BLE")) {
    BLE_conn_id = p->p1.toInt();
    // if(debug) snooze_usb.printf("Info:    BLE connection opened. Conn ID:
    // %d\n", BLE_conn_id);
    working_state.ble_state = BLESTATE_REQ_CONN;
    return BCCMD_NOTHING;
  } else {
    return BCCMD_NOTHING;
  }
}
/*****************************************************************************/

/*****************************************************************************/
static enum serialMsgs msgState(serialParams_t *p) {
  if (!p->p1.substring(p->p1.length() - 2, p->p1.length() - 1).toInt()) {
    return BCNOT_BT_STATE;
  }
  return BCCMD_NOTHING;
}
/*****************************************************************************/



/*****************************************************************************/
/*****************************************************************************/
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
      serialAnswer_t *answ;
      parseSerialIn(inMsg, &answ);
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
void bc127BlueOn(void) { sendCmdOut(CMD_POWER, "ON"); }
/*****************************************************************************/

/*****************************************************************************/
/* bc1267BlueOff(void)
 * -------------------
 * Switch off the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOff(void) { sendCmdOut(CMD_POWER, "OFF"); }
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
void parseSerialIn(String input, serialAnswer_t *answer) {
  if (debug)
    snooze_usb.printf("<-BC127: %s\n", input.c_str());

  enum serialMsgs ret = BCCMD_NOTHING;
  serialParams_t params;
  unsigned int nb_params = sliceParams(input, &params);
  enum serialMsgs notif = readMsg(params.notif);

  switch (notif) {
    case BCNOT_ABS_VOL: {
      ret = bcnotAbsVol(&params);
      break;
    }
    case BCNOT_AVRCP_PAUSE: {
      ret = bcnotAvrcpPause();
      break;
    }
    case BCNOT_AVRCP_PLAY: {
      ret = bcnotAvrcpPlay();
      break;
    }
    case BCNOT_CLOSE_OK: {
      ret = bcnotCloseOk(&params);
      break;
    }
    case BCNOT_INQUIRY: {
      return msgInquiry(&params, nb_params);
      break;
    }
    case BCNOT_INQ_OK: {
      ret = BCNOT_INQ_DONE;
      break;
    }
    case BCNOT_LINK: {
      return msgLink(&params, nb_params);
      break;
    }
    case BCNOT_LINK_LOSS: {
      ret = msgLinkLoss(&params);
      break;
    }
    case BCNOT_NAME: {
      ret = msgName(&params, nb_params);
      break;
    }
    case BCNOT_OPEN_OK: {
      ret = msgOpenOk(&params);
      break;
    }
    case BCNOT_READY: {
      BC127_ready = true;
      break;
    }
    case BCNOT_RECV: {
      return bcnotRecv(&params, nb_params, answer);
      break;
    }
    case BCNOT_STATE: {
      return msgState(&params, nb_params);
      break;
    }

    case BCNOT_A2DP_STREAM_START:
    case BCNOT_A2DP_STREAM_SUSPEND:
    case BCNOT_ASSOCIATION:
    case BCNOT_ASSOCIATION_IN_PROGRESS:
    case BCNOT_AT:
    case BCNOT_AVRCP_BACKWARD:
    case BCNOT_AVRCP_FORWARD:
    case BCNOT_AVRCP_MEDIA:
    case BCNOT_AVRCP_STOP:
    case BCNOT_BA_BROADCASTER_START:
    case BCNOT_BA_BROADCASTER_STOP:
    case BCNOT_BA_RECEIVER_START:
    case BCNOT_BA_RECEIVER_STOP:
    case BCNOT_BC_SMART_CMD:
    case BCNOT_BC_SMART_CMD_RESP:
    case BCNOT_BLE_INDICATION:
    case BCNOT_BLE_NOTIFICATION:
    case BCNOT_BLE_PAIR_ERROR:
    case BCNOT_BLE_PAIR_OK:
    case BCNOT_BLE_READ:
    case BCNOT_BLE_WRITE:
    case BCNOT_CALL_ACTIVE:
    case BCNOT_CALL_DIAL:
    case BCNOT_CALL_END:
    case BCNOT_CALL_INCOMING:
    case BCNOT_CALL_MEMORY:
    case BCNOT_CALL_OUTGOING:
    case BCNOT_CALL_REDIAL:
    case BCNOT_CALLER_NUMBER:
    case BCNOT_CHARGING_IN_PROGRESS:
    case BCNOT_CHARGING_COMPLETE:
    case BCNOT_CHARGER_DISCONNECTED:
    case BCNOT_DTMF:
    case BCNOT_ERROR:
    case BCNOT_IAP_CLOSE_SESSION:
    case BCNOT_IAP_OPEN_SESSION:
    case BCNOT_INBAND_RING:
    case BCNOT_MAP_NEW_MSG:
    case BCNOT_PAIR_ERROR:
    case BCNOT_PAIR_OK:
    case BCNOT_PAIR_PASSKEY:
    case BCNOT_PAIR_PENDING:
    case BCNOT_REMOTE_VOLUME:
    case BCNOT_ROLE:
    case BCNOT_ROLE_OK:
    case BCNOT_ROLE_NOT_ALLOWED:
    case BCNOT_SCO_OPEN:
    case BCNOT_SCO_CLOSE:
    case BCNOT_SR:
    case NBCOT_UNKNOWN:
    default: {
      break;
    }
  }

  return ret;
}
/*****************************************************************************/

/*****************************************************************************/
/* sendCmdOut(enum bc127Cmds, String)
 * ---------------
 * Send specific commands to the BC127 UART
 * IN:	- message (int)
 * OUT:	- command confirmation (bool)
 */
bool sendCmdOut(enum bc127Cmds cmd, String param) {
  String devStr = "";
  String cmdLine = "";
  bool cmdErr = false;

  switch (cmd) {
  case CMD_ADVERTISING: {
    if (param.equalsIgnoreCase("ON")) cmdLine = "ADVERTISING ON";
    else if (param.equalsIgnoreCase("OFF")) cmdLine = "ADVERTISING OFF";
    else cmdErr = true;
    break;
  }
  case CMD_POWER: {
    if (param.equalsIgnoreCase("ON")) cmdLine = "POWER ON";
    else if (param.equalsIgnoreCase("OFF")) cmdLine = "POWER OFF";
    else cmdErr = true;
    break;
  }
  case CMD_NAME: {
    cmdLine = "NAME " + String(BT_peer_address);
    break;
  }
  case CMD_OPEN: {
    if (searchDevlist(BT_peer_name)) {
      if (debug)
        snooze_usb.printf("Info:    Opening BT connection @%s (%s)\n",
                          BT_peer_address.c_str(), BT_peer_name.c_str());

      cmdLine = "OPEN " + BT_peer_address + " A2DP";
    } else {
      cmdLine = "SEND " + String(BLE_conn_id) + " CONN ERR NO BT DEVICE!";
    }
    break;
  }
  case CMD_CLOSE: {
    if(param.equalsIgnoreCase("A2DP")) cmdLine = "CLOSE " + String(BT_id_a2dp);
    else if(param.equalsIgnoreCase("AVRCP")) cmdLine = "CLOSE " + String(BT_id_avrcp);
    break;
  }
  case CMD_INQUIRY: {
    for (int i = 0; i < DEVLIST_MAXLEN; i++) {
      dev_list[i].address = "";
      dev_list[i].capabilities = "";
      dev_list[i].strength = 0;
    }
    found_dev = 0;
    BT_peer_address = "";
    devStr = "";
    cmdLine = "SEND " + String(BLE_conn_id) + " INQ START" + "\r";
    BLUEPORT.print(cmdLine);
    Alarm.delay(80);
    cmdLine = "INQUIRY 10";
  }
  case CMD_MUSIC: {
    if(param.equalsIgnoreCase("PAUSE")) {
      working_state.mon_state = MONSTATE_REQ_OFF;
      if (working_state.bt_state == BTSTATE_CONNECTED) {
        cmdLine = "MUSIC " + String(BT_id_a2dp) + " PAUSE";
      } else
        cmdLine = "";
    }
    else if(param.equalsIgnoreCase("STOP")) {
      if ((working_state.bt_state == BTSTATE_CONNECTED) ||
          (working_state.bt_state == BTSTATE_PLAY)) {
        cmdLine = "MUSIC " + String(BT_id_a2dp) + " STOP");
      } else
        cmdLine = "";
    }
    else if(param.equalsIgnoreCase("START")) {
      // working_state.mon_state = MONSTATE_REQ_ON;
      // if(working_state.bt_state == BTSTATE_CONNECTED) {
      cmdLine = "MUSIC " + String(BT_id_a2dp) + " PLAY";
      // }
      // else cmdLine = "";
    }
  }



  case CMD_AFH_MAP:
  case CMD_ASSOCIATION:
  case CMD_AT:
  case CMD_AVRCP_META_DATA:
  case CMD_BATTERY_STATUS:
  case CMD_BC_SMART_COMMAND:
  case CMD_BC_SMART_NOTIF:
  case CMD_BLE_GET_CHAR:
  case CMD_BLE_GET_SERV:
  case CMD_BLE_INDICATION:
  case CMD_BLE_NOTIFICATION:
  case CMD_BLE_READ:
  case CMD_BLE_READ_RES:
  case CMD_BLE_SECURITY:
  case CMD_BLE_SET_DB:
  case CMD_BLE_WRITE:
  case CMD_BROADCAST:
  case CMD_BT_STATE:
  case CMD_CALL:
  case CMD_CONFIG:
  case CMD_CVC_CFG:
  case CMD_DFU:
  case CMD_ENTER_DATA_MODE:
  case CMD_GET:
  case CMD_HELP:
  case CMD_HID_DESC:
  case CMD_HID_READ:
  case CMD_IAP:
  case CMD_IAP_APP_REQ:

  case CMD_LICENSE:
  case CMD_LINK_POLICY:
  case CMD_LIST:
  case CMD_MAP_GET_MSG:
  case CMD_MM_CFG:
  case CMD_PAIR:
  case CMD_PASSKEY:
  case CMD_PB_ABORT:
  case CMD_PB_PULL:
  case CMD_PIO:
  case CMD_REMOTE_VOLUME:
  case CMD_RESET:
  case CMD_RESTORE:
  case CMD_ROLE:
  case CMD_ROUTE:
  case CMD_RSSI:
  case CMD_SCAN:
  case CMD_SEND:
  case CMD_SEND_RAW:
  case CMD_SET:
  case CMD_SPEECH_REC:
  case CMD_SSRD:
  case CMD_STATUS:
  case CMD_TOGGLE_VR:
  case CMD_TONE:
  case CMD_TX_POWER:
  case CMD_UNPAIR:
  case CMD_VERSION:
  case CMD_VOLUME:
  case CMD_WRITE:
  case CMD_UNKNOWN:
  default:
    break;
  }
  return true;
}

bool sendCmdOut(int msg) {
  String devString = "";
  String cmdLine = "";

  switch (msg) {
  /* --------
   * COMMANDS
   * -------- */
  case BCCMD_NOTHING:
    break;
  // Start BLE advertising
  // case BCCMD_ADV_ON:
  //   cmdLine = "ADVERTISING ON\r";
  //   break;
  // Stop BLE advertising
  // case BCCMD_ADV_OFF:
  //   cmdLine = "ADVERTISING OFF\r";
  //   break;
  // Switch off device (serial)
  // case BCCMD_BLUE_OFF:
  //   cmdLine = "POWER OFF\r";
  //   break;
  // // Switch on device (serial)
  // case BCCMD_BLUE_ON:
  //   cmdLine = "POWER ON\r";
  //   break;
  // Ask for friendly name of connected BT device
  // case BCCMD_BT_NAME:
  //   cmdLine = "NAME " + String(BT_peer_address) + "\r";
  //   break;
  // Open A2DP connection with 'BT_peer_address'
  // case BCCMD_DEV_CONNECT:
  //   cmdLine = cmdDevConnect();
  //   break;
  // Close A2DP connection with BT device
  // case BCCMD_DEV_A2DP_DISCONNECT:
  //   cmdLine = "CLOSE " + String(BT_id_a2dp) + "\r";
  //   break;
  // // Close AVRCP connection with BT device
  // case BCCMD_DEV_AVRCP_DISCONNECT:
  //   cmdLine = "CLOSE " + String(BT_id_avrcp) + "\r";
  //   break;
  // Start inquiry on BT for 10 s, clearing the device list first
  // case BCCMD_INQUIRY:
  //   cmdLine = cmdInquiry();
  //   break;
  // Pause monitoring -> AVRCP pause
  // case BCCMD_MON_PAUSE:
  //   cmdLine = cmdMonPause();
  //   break;
  // Start monitoring -> AVRCP play
  // case BCCMD_MON_START:
  //   cmdLine = cmdMonStart();
  //   break;
  // // Stop monitoring -> AVRCP pause
  // case BCCMD_MON_STOP:
  //   cmdLine = cmdMonStop();
  //   break;
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
