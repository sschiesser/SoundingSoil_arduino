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
  String capabilities;
  unsigned int strength;
};
struct btDev dev_list[DEVLIST_MAXLEN];

// Amount of found BT devices on inquiry
unsigned int									found_dev;
// Address of the connected BT device
String 												BT_peer_address;
// ID of the established BT connection
int														BT_conn_id;
// ID of the established BLE connection
int														BLE_conn_id;

/* bc127Init(void)
 * ---------------
 * Initialize BC127, ...by switching it off!
 * IN:	- none
 * OUT:	- none
 */
void bc127Init(void) {
	bc127PowerOff();
}

void bc127PowerOn(void) {
	sendCmdOut(BCCMD_PWR_ON);
}

void bc127PowerOff(void) {
	sendCmdOut(BCCMD_PWR_OFF);
}

/* bc127AdvStart(void)
 * -------------------
 * Start advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStart(void) {
	sendCmdOut(BCCMD_ADV_ON);
}

/* bc127AdvStop(void)
 * ------------------
 * Stop advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127AdvStop(void) {
	if(working_state.ble_state == BLESTATE_ADV) {
		sendCmdOut(BCCMD_ADV_OFF);
	}
}

/* parseSerialIn(String)
 * ---------------------
 * Parse received string and read information along
 * minimal keywords.
 * IN:	- received input (String)
 * OUT:	- message to send back (int)
 */
int parseSerialIn(String input) {
  // ================================================================
  // Slicing input string.
  // Currently 3 usefull schemes:
  // - 2 words   --> e.g. "OPEN_OK AVRCP"
  // - RECV BLE  --> commands received from phone over BLE
  // - other     --> 3+ words inputs like "INQUIRY xxxx 240404 -54dB"
  // ================================================================
	String notif, param1, param2, param3, param4, trash;
  unsigned int nb_params;
  int slice1 = input.indexOf(" ");
  int slice2 = input.indexOf(" ", slice1+1);
	int slice3 = input.indexOf(" ", slice2+1);
	int slice4 = input.indexOf(" ", slice3+1);
	int slice5 = input.indexOf(" ", slice4+1);
	// no space found -> notification without parameter
	if(slice1 == -1) {
		notif = input;
		nb_params = 0;
	}
  // 1+ parameter
  else {
    notif = input.substring(0, slice1);
		if(slice2 == -1) {
			param1 = input.substring(slice1 + 1);
			nb_params = 1;
		}
		// 2+ parameters
		else {
			param1 = input.substring(slice1 + 1, slice2);
			if(slice3 == -1) {
				param2 = input.substring(slice2 + 1);
				nb_params = 2;
			}
			// 3+ parameters
			else {
				param2 = input.substring(slice2 + 1, slice3);
				if(slice4 == -1) {
					param3 = input.substring(slice3 + 1);
					nb_params = 3;
				}
				// 4+ parameters
				else {
					param3 = input.substring(slice3 + 1, slice4);
					if(slice5 == -1) {
						param4 = input.substring(slice4 + 1);
						nb_params = 4;
					}
					else {
						param4 = input.substring(slice4 + 1, slice5);
						trash = input.substring(slice5 + 1);
						nb_params = 5;
					}
				}
			}
		}
	}

	Serial.printf("%d parameters found:\n- notif = %s\n", nb_params, notif.c_str());
	
	switch(nb_params) {
		// INQU_OK
		// PAIR_PENDING
		// READY
		case 0: {
			if(notif.equalsIgnoreCase("INQU_OK")) {
			}
			else if(notif.equalsIgnoreCase("PAIR_PENDING")) {
			}
			else if(notif.equalsIgnoreCase("READY")) {
			}
			else {
				Serial.println(notif);
			}
			break;
		}
		
		// A2DP_STEAM_START [link_ID]
		// A2DP_STEAM_SUSPEND [link_ID]
		// AVRCP_PLAY [link_ID]
		// AVRCP_STOP [link_ID]
		// AVRCP_PAUSE [link_ID]
		// AVRCP_FORWARD [link_ID]
		// AVRCP_BACKWARD [link_ID]
		// ERROR 0xXXXX
		// PAIR_ERROR (Bluetooth address)
		// PAIR_OK (Bluetooth address)
		case 1: {
			Serial.printf("- param1 = %s\n", param1.c_str());
			if(notif.equalsIgnoreCase("A2DP_STREAM_START")) {
			}
			else if(notif.equalsIgnoreCase("A2DP_STREAM_SUSPEND")) {
			}
			else if(notif.equalsIgnoreCase("AVRCP_PLAY")) {
				working_state.mon_state = MONSTATE_REQ_ON;
			}
			else if(notif.equalsIgnoreCase("AVRCP_STOP")) {
			}
			else if(notif.equalsIgnoreCase("AVRCP_PAUSE")) {
				working_state.mon_state = MONSTATE_REQ_OFF;
			}
			else if(notif.equalsIgnoreCase("AVRCP_FORWARD")) {
				return BCCMD_REC_START;
			}
			else if(notif.equalsIgnoreCase("AVRCP_BACKWARD")) {
				return BCCMD_REC_STOP;
			}
			else if(notif.equalsIgnoreCase("ERROR")) {
			}
			else if(notif.equalsIgnoreCase("PAIR_ERROR")) {
			}
			else if(notif.equalsIgnoreCase("PAIR_OK")) {
			}
			else {
				Serial.printf("%s %s\n", notif, param1);
			}
			break;
		}
			
		// ABS_VOL [link_ID](value)
		// BLE_READ [link_ID] handle
		// CLOSE_OK [link_ID] (profile)
		// CLOSE_ERROR [link_ID] (profile)
		// LINK_LOSS [link_ID] (status)
		// NAME [addr] [remote_name]
		// OPEN_ERROR [link_ID] (profile)
		case 2: {
			Serial.printf("- param1 = %s\n", param1.c_str());
			Serial.printf("- param2 = %s\n", param2.c_str());
			if(notif.equalsIgnoreCase("ABS_VOL")) {
				vol_value = (float)param2.toInt()/120.0;
				Serial.printf("Volume: %d (%f)\n", param2.toInt(), vol_value);
				return BCNOT_VOL_LEVEL;
			}
			if(notif.equalsIgnoreCase("BLE_READ")) {
			}
			if(notif.equalsIgnoreCase("CLOSE_OK")) {
				if(param1.toInt() == BT_conn_id) {
					if(working_state.bt_state != BTSTATE_OFF) working_state.bt_state = BTSTATE_REQ_DIS;
					BT_conn_id = 0;
					BT_peer_address = "";
				}
				else if(param1.toInt() == BLE_conn_id) {
					if(working_state.ble_state != BLESTATE_OFF) working_state.ble_state = BLESTATE_REQ_DIS;
					BLE_conn_id = 0;
				}
			}
			if(notif.equalsIgnoreCase("CLOSE_ERROR")) {
			}
			if(notif.equalsIgnoreCase("LINK_LOSS")) {
			}
			if(notif.equalsIgnoreCase("NAME")) {
			}
			if(notif.equalsIgnoreCase("OPEN_ERROR")) {
			}
			break;
		}
		
		// AT [link_ID] (length) (data)
		// OPEN_OK [link_ID] (profile) (Bluetooth address)
		// RECV [link_ID] (size) (report data)
		case 3: {
			Serial.printf("- param1 = %s\n", param1.c_str());
			Serial.printf("- param2 = %s\n", param2.c_str());
			Serial.printf("- param3 = %s\n", param3.c_str());
			if(notif.equalsIgnoreCase("AT")) {
			}
			else if(notif.equalsIgnoreCase("OPEN_OK")) {
				if(param2.equalsIgnoreCase("A2DP")) {
					BT_conn_id = param1.toInt();
					BT_peer_address = param3;
					Serial.printf("A2DP connection opened. Conn ID: %d, peer address = %s\n",
						BT_conn_id, BT_peer_address.c_str());
					working_state.bt_state = BTSTATE_REQ_CONN;
				}
				else if(param2.equalsIgnoreCase("BLE")) {
					BLE_conn_id = param1.toInt();
					Serial.printf("BLE connection opened. Conn ID: %d\n", BLE_conn_id);
					working_state.ble_state = BLESTATE_REQ_CONN;
				}
			}
			else if(notif.equalsIgnoreCase("RECV")) 
			{
				// BLE commands:
				// - "inq"
				if(param1.toInt() == BLE_conn_id) {
					// Serial.println("Receiving 1-param BLE command");
					if(param3.equalsIgnoreCase("inq")) {
					  Serial.println("Received BT inquiry command");
						return BCCMD_INQUIRY;
					}

				}
			}
			else {
				Serial.printf("%s %s %s\n", notif, param1, param2);
			}
			break;
		}
		
		// BLE_CHAR [link_ID] type uuid handle
		// BLE_INDICATION [link_ID] handle size data
		// BLE_NOTIFICATION [link_ID] handle size data
		// BLE_READ_RES [link_ID] handle size data
		// BLE_SERV [link_ID] type uuid handle
		// BLE_WRITE [link_ID] handle size data
		// INQUIRY(BDADDR) (NAME) (COD) (RSSI)
		// RECV [link_ID] (size) (report data) <-- (report data) with 2 parameters
		case 4: {
			Serial.printf("- param1 = %s\n", param1.c_str());
			Serial.printf("- param2 = %s\n", param2.c_str());
			Serial.printf("- param3 = %s\n", param3.c_str());
			Serial.printf("- param4 = %s\n", param4.c_str());
			if(notif.equalsIgnoreCase("BLE_CHAR")) {
			}
			else if(notif.equalsIgnoreCase("BLE_INDICATION")) {
			}
			else if(notif.equalsIgnoreCase("BLE_NOTIFICATION")) {
			}
			else if(notif.equalsIgnoreCase("BLE_READ_RES")) {
			}
			else if(notif.equalsIgnoreCase("BLE_SERV")) {
			}
			else if(notif.equalsIgnoreCase("BLE_WRITE")) {
			}
			else if(notif.equalsIgnoreCase("INQUIRY")) {
				String addr = param1;
				String name = param2;
				String caps = param3;
				unsigned int stren = param4.substring(1, 3).toInt();
				populateDevlist(addr, caps, stren);
				return BCNOT_INQ_STATE;
			}
			else if(notif.equalsIgnoreCase("RECV")) {
				// BLE commands:
				// - "time {ts}"
				// - "rec {start/stop/?/state}"
				// - "mon {start/stop/?/state}"
				// - "vol {+/-/?/value}"
				// - "bt {?/address}"
				// - "rwin {?}"
				// - "filepath {?/fp}"
				if(param1.toInt() == BLE_conn_id) {
					// Serial.println("Receiving 2-params BLE command");
					if(param3.equalsIgnoreCase("time")) {
					}
					else if(param3.equalsIgnoreCase("rec")) {
						if(param4.equalsIgnoreCase("start")) return BCCMD_REC_START;
						else if(param4.equalsIgnoreCase("stop")) return BCCMD_REC_STOP;
						else if(param4.equalsIgnoreCase("?")) return BCNOT_REC_STATE;
						else {
							Serial.println("BLE rec command not listed");
						}
					}
					else if(param3.equalsIgnoreCase("mon")) {
						if(param4.equalsIgnoreCase("start")) {
							Serial.println("Start MON");
							return BCCMD_MON_START;
						}
						else if(param4.equalsIgnoreCase("stop")) {
							Serial.println("Stop MON");
							return BCCMD_MON_STOP;
						}
						else if(param4.equalsIgnoreCase("?")) return BCNOT_MON_STATE;
						else {
							Serial.println("BLE mon command not listed");
						}
					}
					else if(param3.equalsIgnoreCase("vol")) {
					}
					else if(param3.equalsIgnoreCase("bt")) {
					}
					else if(param3.equalsIgnoreCase("rwin")) {
					}
					else if(param3.equalsIgnoreCase("filepath")) {
					}
				}
			}
			else {
				Serial.printf("%s %s %s %s\n", notif, param1, param2, param3);
			}
			break;
		}
		
		case 5:
		default:
			Serial.println(input.c_str());
			break;
	}
	
	
  // Commands/requests from Android ("RECV BLE")
  // ===========================================
	// if(part1.equalsIgnoreCase("LLL")) {
	// }
  // else if(part1.equalsIgnoreCase("RECV BLE")) {
		// // INQ command
		// if(part2.equalsIgnoreCase("inq\n")) {
      // // Serial.println("Received BT inquiry command");
      // return BCCMD_INQUIRY;
    // }
		// REC command/request
		// else if(part2.substring(0, 3).equalsIgnoreCase("rec")) {
			// cmd = part2.substring(4);
			// if(cmd.equalsIgnoreCase("start\n")) {
				// return BCCMD_REC_START;
			// }
			// else if(cmd.equalsIgnoreCase("stop\n")) {
				// return BCCMD_REC_STOP;
			// }
			// else if(cmd.equalsIgnoreCase("?\n")) {
				// return BCNOT_REC_STATE;
			// }
		// }
		// MON command/request
		// else if(part2.substring(0, 3).equalsIgnoreCase("mon")) {
			// cmd = part2.substring(4);
			// if(cmd.equalsIgnoreCase("start\n")) {
				// return BCCMD_MON_START;
			// }
			// else if(cmd.equalsIgnoreCase("stop\n")) {
				// return BCCMD_MON_STOP;
			// }
			// else if(cmd.equalsIgnoreCase("?\n")) {
				// return BCNOT_MON_STATE;
			// }
		// }
		// // VOL command/request
		// else if(part2.substring(0, 3).equalsIgnoreCase("vol")) {
			// cmd = part2.substring(4);
			// if(cmd.equalsIgnoreCase("+\n")) {
				// return BCCMD_VOL_UP;
			// }
			// else if(cmd.equalsIgnoreCase("-\n")) {
				// return BCCMD_VOL_DOWN;
			// }
			// else if(cmd.equalsIgnoreCase("?\n")) {
				// return BCNOT_VOL_LEVEL;
			// }
		// }
		// // RWIN command/request
		// else if(part2.substring(0, 4).equalsIgnoreCase("rwin")) {
			// slice1 = part2.indexOf(" ", 5);
			// if(slice1 != -1) {
				// slice2 = part2.indexOf(" ", (slice1+1));
				// unsigned int l, p;
				// l = part2.substring(5, slice1).toInt();
				// p = part2.substring((slice1+1), slice2).toInt();
				// breakTime(l, rec_window.length);
				// breakTime(p, rec_window.period);
				// rec_window.occurences = part2.substring(slice2+1).toInt();
				// Serial.printf("Rwin set to: l = %02dh%02dm%02ds, p = %02dh%02dm%02ds, o = %d\n",
						// rec_window.length.Hour, rec_window.length.Minute, rec_window.length.Second,
						// rec_window.period.Hour, rec_window.period.Minute, rec_window.period.Second,
						// rec_window.occurences);
			// }
			// else {
				// cmd = part2.substring(5);
				// if(cmd.equalsIgnoreCase("?\n")) {
					// return BCNOT_RWIN_VALS;
				// }
			// }
		// }
		// // CONN command -> open A2DP connection to xxxx (12 HEX)
    // else if(part2.substring(0, 4).equalsIgnoreCase("conn")) {
      // int len = part2.substring(5).length()-1;
      // BT_peer_address = part2.substring(5, (5+len));
      // Serial.print("BT_peer_address: "); Serial.println(BT_peer_address);
      // return BCCMD_DEV_CONNECT;
    // }
		// // TIMESTAMP command -> set current time (UNINT32 DEC)
		// else if(part2.substring(0, 4).equalsIgnoreCase("time")) {
			// int len = part2.substring(5).length()-1;
			// received_time = part2.substring(5, (5+len)).toInt();
			// if(received_time > DEFAULT_TIME_DEC) {
				// time_source = TSOURCE_BLE;
				// adjustTime(TSOURCE_BLE);
			// }
			// Serial.printf("current time: 0x%x(d'%ld)\n", received_time, received_time);
		// }
		// // BT state request
		// else if(part2.substring(0, 2).equalsIgnoreCase("bt")) {
			// cmd = part2.substring(3);
			// if(cmd.equalsIgnoreCase("?\n")) {
				// return BCNOT_BT_STATE;
			// }
		// }
		// // FILEPATH request
		// else if(part2.substring(0, 8).equalsIgnoreCase("filepath")) {
			// cmd = part2.substring(9);
			// if(cmd.equalsIgnoreCase("?\n")) {
				// return BCNOT_FILEPATH;
			// }
		// }
  // }
  // else {
    // // Serial.print(input);
  // }
  return BCCMD_NOTHING;
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
	unsigned int l, p, o;
  
  switch(msg) {
    case BCCMD_NOTHING:
			break;

		// Reset
    case BCCMD_RESET: {
			// Serial.println("Resetting BC127...");
			cmdLine = "RESET\r";
			break;
		}
		// Power-off
		case BCCMD_PWR_OFF: {
			// Serial.println("Switching BC127 off");
			cmdLine = "POWER OFF\r";
			break;
		}
		// Power-on
		case BCCMD_PWR_ON: {
			// Serial.println("Waking BC127 up");
			cmdLine = "POWER ON\r";
			break;
		}
    // Start advertising on BLE
    case BCCMD_ADV_ON: {
			// Serial.println("Starting BLE advertising");
			cmdLine = "ADVERTISING ON\r";
			break;
		}
		// Stop advertising
		case BCCMD_ADV_OFF: {
			cmdLine = "ADVERTISING OFF\r";
			break;
		}
    // Start inquiry on BT for 10 s, first clear the device list
    case BCCMD_INQUIRY: {
			// Serial.println("Start inquiry");
			for(int i = 0; i < DEVLIST_MAXLEN; i++) {
				dev_list[i].address = "";
				dev_list[i].capabilities = "";
				dev_list[i].strength = 0;
			}
			found_dev = 0;
			BT_peer_address = "";
			devString = "";
			cmdLine = "INQUIRY 10\r";
			break;
		}
		// Open A2DP connection with 'BT_peer_address'
    case BCCMD_DEV_CONNECT: {
			Serial.print("Opening BT connection @"); Serial.println(BT_peer_address);
			cmdLine = "OPEN " + BT_peer_address + " A2DP\r";
			break;
		}
		// Start monitoring -> AVRCP play
    case BCCMD_MON_START: {
			Serial.println("Sending MON start");
			working_state.mon_state = MONSTATE_REQ_ON;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				cmdLine = "MUSIC " + String(BT_conn_id) + " PLAY\r";
			}
			break;
		}
		// Stop monitoring -> AVRCP pause
    case BCCMD_MON_STOP: {
			Serial.println("Sending MON stop");
			working_state.mon_state = MONSTATE_REQ_OFF;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				cmdLine = "MUSIC " + String(BT_conn_id) + " PAUSE\r";
			}
			break;
		}
		// Start recording
    case BCCMD_REC_START: {
			working_state.rec_state = RECSTATE_REQ_ON;
			break;
		}
		// Stop recording
    case BCCMD_REC_STOP: {
			working_state.rec_state = RECSTATE_REQ_OFF;
			break;
		}
		// Volume up -> AVRCP volume up
    case BCCMD_VOL_UP: {
			cmdLine = "VOLUME UP\r";
			break;
		}
		// Volume down -> AVRCP volume down
    case BCCMD_VOL_DOWN: {
			cmdLine = "VOLUME DOWN\r";
			break;
		}
		// Volume level request
		case BCCMD_VOL_A2DP: {
			cmdLine = "VOLUME A2DP\r";
			break;
		}
		// REC state notification
		case BCNOT_REC_STATE: {
			cmdLine = "SEND BLE ";
			if(working_state.rec_state == RECSTATE_ON) cmdLine += "REC ON\r";
			else if((working_state.rec_state == RECSTATE_WAIT) || (working_state.rec_state == RECSTATE_IDLE)) cmdLine += "REC WAIT\r";
			else cmdLine += "REC OFF\r";
			break;
		}
		// MON state notification
		case BCNOT_MON_STATE: {
			cmdLine = "SEND BLE ";
			if(working_state.mon_state == MONSTATE_ON) cmdLine += "MON ON\r";
			else cmdLine += "MON OFF\r";
			break;
		}
		// BT state notification
		case BCNOT_BT_STATE: {
			cmdLine = "SEND BLE ";
			if(working_state.bt_state == BTSTATE_CONNECTED) cmdLine += ("BT " + BT_peer_address + "\r");
			else if(working_state.bt_state == BTSTATE_INQUIRY) cmdLine += "BT INQ\r";
			else cmdLine += "BT IDLE\r";
			break;
		}
		// VOL level notification
		case BCNOT_VOL_LEVEL: {
			cmdLine = "SEND BLE VOL " + String(vol_value) + "\r";
			break;
		}
    // Results of the inquiry -> store devices with address & signal strength
    case BCNOT_INQ_STATE: {
			for(unsigned int i = 0; i < found_dev; i++) {
				devString = dev_list[i].address;
				devString.concat(" ");
				devString.concat(dev_list[i].strength);
				cmdLine = "SEND BLE " + devString + "\r";
				Serial4.print(cmdLine);
			}
			cmdLine = "";
			break;
		}
		// RWIN values notification
		case BCNOT_RWIN_VALS: {
			l = makeTime(rec_window.length);
			p = makeTime(rec_window.period);
			o = rec_window.occurences;
			cmdLine = "SEND BLE RWIN " + String(l) + " " + String(p) + " " + String(o) + "\r";
			break;
		}
		// FILEPATH notification !!CURRENTLY MAX NOTIFICATION SIZE = 20 chars -> change to at least 26
		case BCNOT_FILEPATH: {
			cmdLine = "SEND BLE P " + rec_path + "\r";
			Serial.printf("Sending: %s\n", cmdLine.c_str());
			break;
		}
		// No recognised command -> send negative confirmation
    default:
			return false;
			break;
  }
	// Send the prepared command line to UART
  Serial4.print(cmdLine);
  // Send positive confirmation
  return true;
}

/* populateDevlist(String, String, unsigned int)
 * ---------------------------------------------
 * Search if received device address is already existing.
 * If not, fill the list at the next empty place, otherwise
 * just update the signal strength value.
 * IN:	- device address (String)
 *			- device capabilities (String)
 *			- absolute signal stregth value (unsigned int)
 * OUT:	- none
 */
void populateDevlist(String addr, String caps, unsigned int stren) {
  bool found = false;
  int lastPos = 0;
  for(int i = 0; i < DEVLIST_MAXLEN; i++) {
		// Received address matches to an already existing one.
    if(addr.equals(dev_list[i].address)) {
      dev_list[i].strength = stren;
      found = true;
      break;
    }
		// No address matched and the current position is empty.
    else if(dev_list[i].address.equals("")) {
      lastPos = i;
      break;
    }
  }

	// If not matching device has been found, fill the next emplacement.
  if(!found) {
    dev_list[lastPos].address = addr;
    dev_list[lastPos].capabilities = caps;
    dev_list[lastPos].strength = stren;
    found_dev += 1;
  }
  
  // Serial.println("Device list:");
  // for(unsigned int i = 0; i < found_dev; i++) {
    // Serial.print(i); Serial.print(": ");
    // Serial.print(dev_list[i].address); Serial.print(", ");
    // Serial.print(dev_list[i].capabilities); Serial.print(", ");
    // Serial.println(dev_list[i].strength);
  // }
}
