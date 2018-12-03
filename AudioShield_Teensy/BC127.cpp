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
String 												peer_address;

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
  // Serial.print(input);
  String part1, part2, cmd;
  int slice1 = input.indexOf(" ");
  int slice2 = input.indexOf(" ", slice1+1);
  // 2nd space not found -> 2-word notification
  if(slice2 == -1) {
    part1 = input.substring(0, slice1);
    part2 = input.substring(slice1 + 1);
  }
	// more than 2 words...
  else {
    part1 = input.substring(0, slice1);
    part2 = input.substring(slice1 + 1, slice2);
    // "RECV BLE" case -> part2 contains everything coming after "RECV BLE"
    if(part1.equals("RECV") && part2.equals("BLE")) {
      part1.concat(" BLE");
      part2 = input.substring(slice2+1);
    }
    // other cases -> part2 contains all words after the first space
    else {
      part2 = input.substring(slice1+1);
    }
  }

  // Notifications from BC127!!
  // ==========================
  // "OPEN_OK" -> Bluetooth protocol opened succesfully
	if(part1.equalsIgnoreCase("OPEN_OK")) {
		// "OPEN_OK BLE" -> connection established with phone app
    if(part2.equalsIgnoreCase("BLE\n")) {
			working_state.ble_state = BLESTATE_REQ_CONN;
    }
		// "OPEN_OK A2DP" -> A2DP connection established with audio receiver 
    else if(part2.equalsIgnoreCase("A2DP\n")) {
      // Serial.println("A2DP protocol open!");
			working_state.bt_state = BTSTATE_REQ_CONN;
    }
  }
	// "CLOSE_OK" -> Bluetooth protocol closed succesfully
	else if(part1.equalsIgnoreCase("CLOSE_OK")) {
		if(part2.equalsIgnoreCase("BLE\n")) {
			if(working_state.ble_state != BLESTATE_OFF) {
				working_state.ble_state = BLESTATE_REQ_DIS;
			}
		}
		if(part2.equalsIgnoreCase("A2DP\n")) {
			if(working_state.bt_state != BTSTATE_OFF) {
				working_state.bt_state = BTSTATE_REQ_DIS;
			}
		}
	}
	// "AVRCP_PLAY" -> A2DP stream open, need to start monitor on Teensy
	else if(part1.equalsIgnoreCase("AVRCP_PLAY\n")) {
		working_state.mon_state = MONSTATE_REQ_ON;
	}
	// "AVRCP_PAUSE" -> A2DP stream closed, stopping monitor on Teensy
	else if(part1.equalsIgnoreCase("AVRCP_PAUSE\n")) {
		working_state.mon_state = MONSTATE_REQ_OFF;
	}
	// "AVRCP_FORWARD" -> USED AS REC START!!!
	else if(part1.equalsIgnoreCase("AVRCP_FORWARD\n")) {
		return BCCMD_REC_START;
	}
	// "AVRCP_BACKWARD" -> USED AS REC STOP!!!
	else if(part1.equalsIgnoreCase("AVRCP_BACKWARD\n")) {
		return BCCMD_REC_STOP;
	}
	// "INQUIRY xxxxxxxxxxxx yyyyyy -zzdB"
  else if(part1.equalsIgnoreCase("INQUIRY")) {
    slice1 = part2.indexOf(" ");
    String addr = part2.substring(0, slice1);
		part2 = part2.substring(slice1+1);
		slice1 = part2.indexOf(" ");
    String caps = part2.substring(0, slice1);
		part2 = part2.substring(slice1+1);
		slice1 = part2.indexOf("dB");
    unsigned int stren = part2.substring(1, slice1).toInt();
    populateDevlist(addr, caps, stren);
    return BCNOT_INQ_STATE;
  }
  // Commands/requests from Android ("RECV BLE")
  // ===========================================
  else if(part1.equalsIgnoreCase("RECV BLE")) {
		// INQ command
		if(part2.equalsIgnoreCase("inq\n")) {
      // Serial.println("Received BT inquiry command");
      return BCCMD_INQUIRY;
    }
		// REC command/request
		else if(part2.substring(0, 3).equalsIgnoreCase("rec")) {
			String cmd = part2.substring(4);
			if(cmd.equalsIgnoreCase("start\n")) {
				return BCCMD_REC_START;
			}
			else if(cmd.equalsIgnoreCase("stop\n")) {
				return BCCMD_REC_STOP;
			}
			else if(cmd.equalsIgnoreCase("?\n")) {
				return BCNOT_REC_STATE;
			}
		}
		// MON command/request
		else if(part2.substring(0, 3).equalsIgnoreCase("mon")) {
			String cmd = part2.substring(4);
			if(cmd.equalsIgnoreCase("start\n")) {
				return BCCMD_MON_START;
			}
			else if(cmd.equalsIgnoreCase("stop\n")) {
				return BCCMD_MON_STOP;
			}
			else if(cmd.equalsIgnoreCase("?\n")) {
				return BCNOT_MON_STATE;
			}
		}
		// VOL command/request
		else if(part2.substring(0, 3).equalsIgnoreCase("vol")) {
			String cmd = part2.substring(4);
			if(cmd.equalsIgnoreCase("+\n")) {
				return BCCMD_VOL_UP;
			}
			else if(cmd.equalsIgnoreCase("-\n")) {
				return BCCMD_VOL_DOWN;
			}
			else if(cmd.equalsIgnoreCase("?\n")) {
				return BCNOT_VOL_LEVEL;
			}
		}
		// RWIN command/request
		else if(part2.substring(0, 4).equalsIgnoreCase("rwin")) {
			slice1 = part2.indexOf(" ", 5);
			if(slice1 != -1) {
				slice2 = part2.indexOf(" ", (slice1+1));
				unsigned int l, p;
				l = part2.substring(5, slice1).toInt();
				p = part2.substring((slice1+1), slice2).toInt();
				breakTime(l, rec_window.length);
				breakTime(p, rec_window.period);
				rec_window.occurences = part2.substring(slice2+1).toInt();
				Serial.printf("Rwin set to: l = %02dh%02dm%02ds, p = %02dh%02dm%02ds, o = %d\n",
						rec_window.length.Hour, rec_window.length.Minute, rec_window.length.Second,
						rec_window.period.Hour, rec_window.period.Minute, rec_window.period.Second,
						rec_window.occurences);
			}
			else {
				cmd = part2.substring(5);
				if(cmd.equalsIgnoreCase("?\n")) {
					return BCNOT_RWIN_VALS;
				}
			}
		}
		// CONN command -> open A2DP connection to xxxx (12 HEX)
    else if(part2.substring(0, 4).equalsIgnoreCase("conn")) {
      int len = part2.substring(5).length()-1;
      peer_address = part2.substring(5, (5+len));
      Serial.print("peer_address: "); Serial.println(peer_address);
      return BCCMD_DEV_CONNECT;
    }
		// TIMESTAMP command -> set current time (UNINT32 DEC)
		else if(part2.substring(0, 4).equalsIgnoreCase("time")) {
			int len = part2.substring(5).length()-1;
			received_time = part2.substring(5, (5+len)).toInt();
			if(received_time > DEFAULT_TIME_DEC) {
				time_source = TSOURCE_BLE;
				adjustTime(TSOURCE_BLE);
			}
			Serial.printf("current time: 0x%x(d'%ld)\n", received_time, received_time);
		}
		// BT state request
		else if(part2.substring(0, 2).equalsIgnoreCase("bt")) {
			cmd = part2.substring(3);
			if(cmd.equalsIgnoreCase("?\n")) {
				return BCNOT_BT_STATE;
			}
		}
  }
  else {
    Serial.print(input);
  }
  
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
  
  switch(msg) {
    case BCCMD_NOTHING:
			break;

		// Reset
    case BCCMD_RESET:
			// Serial.println("Resetting BC127...");
			cmdLine = "RESET\r";
			break;
		// Power-off
		case BCCMD_PWR_OFF:
			// Serial.println("Switching BC127 off");
			cmdLine = "POWER OFF\r";
			break;
		// Power-on
		case BCCMD_PWR_ON:
			// Serial.println("Waking BC127 up");
			cmdLine = "POWER ON\r";
			break;
    // Start advertising on BLE
    case BCCMD_ADV_ON:
			// Serial.println("Starting BLE advertising");
			cmdLine = "ADVERTISING ON\r";
			break;
		// Stop advertising
		case BCCMD_ADV_OFF:
			cmdLine = "ADVERTISING OFF\r";
			break;
    // Start inquiry on BT for 10 s, first clear the device list
    case BCCMD_INQUIRY:
			// Serial.println("Start inquiry");
			for(int i = 0; i < DEVLIST_MAXLEN; i++) {
				dev_list[i].address = "";
				dev_list[i].capabilities = "";
				dev_list[i].strength = 0;
			}
			found_dev = 0;
			peer_address = "";
			devString = "";
			cmdLine = "INQUIRY 10\r";
			break;
		// Open A2DP connection with 'peer_address'
    case BCCMD_DEV_CONNECT:
			Serial.print("Opening BT connection @"); Serial.println(peer_address);
			cmdLine = "OPEN " + peer_address + " A2DP\r";
			break;
		// Start monitoring -> AVRCP play
    case BCCMD_MON_START:
			working_state.mon_state = MONSTATE_REQ_ON;
			if(working_state.bt_state == BTSTATE_CONNECTED) cmdLine = "MUSIC PLAY\r";
			break;
		// Stop monitoring -> AVRCP pause
    case BCCMD_MON_STOP:
			working_state.mon_state = MONSTATE_REQ_OFF;
			if(working_state.bt_state == BTSTATE_CONNECTED) cmdLine = "MUSIC PAUSE\r";
			break;
		// Start recording
    case BCCMD_REC_START:
			working_state.rec_state = RECSTATE_REQ_ON;
			break;
		// Stop recording
    case BCCMD_REC_STOP:
			working_state.rec_state = RECSTATE_REQ_OFF;
			break;
		// Volume up -> AVRCP volume up
    case BCCMD_VOL_UP:
			cmdLine = "VOLUME UP\r";
			break;
		// Volume down -> AVRCP volume down
    case BCCMD_VOL_DOWN:
			cmdLine = "VOLUME DOWN\r";
			break;
		case BCNOT_REC_STATE:
			break;
		case BCNOT_MON_STATE:
			break;
		case BCNOT_BT_STATE:
			break;
    // Results of the inquiry -> store devices with address & signal strength
    case BCNOT_INQ_STATE:
			for(unsigned int i = 0; i < found_dev; i++) {
				devString = dev_list[i].address;
				devString.concat(" ");
				devString.concat(dev_list[i].strength);
				cmdLine = "SEND BLE " + devString + "\r";
				Serial4.print(cmdLine);
			}
			cmdLine = "";
			break;
		case BCNOT_RWIN_VALS:
			break;
		case BCNOT_FILEPATH:
			break;
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
