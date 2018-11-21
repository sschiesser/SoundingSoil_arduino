/*
 * BC127
 * 
 * Miscellaneous functions to communicate with BC127.
 * 
 */

#include "BC127.h"

struct btDev {
  String address;
  String capabilities;
  unsigned int strength;
};
struct btDev dev_list[DEVLIST_MAXLEN];

unsigned int found_dev;
String peer_address;

/* bc127Init(void)
 * ---------------
 * Initialize BC127, ...by switching it off!
 * IN:	- none
 * OUT:	- none
 */
void bc127Init(void) {
	bc127Stop();
}

/* bc127Start(void)
 * ----------------
 * Start BC127, then start advertising on the BLE channel.
 * IN:	- none
 * OUT:	- none
 */
void bc127Start(void) {
	sendCmdOut(BCCMD_GEN_PWRON);
	delay(200);
	sendCmdOut(BCCMD_BLE_ADV_ON);
}

/* bc127Stop(void)
 * ---------------
 * Stop BC127 (switch off)
 * IN:	- none
 * OUT:	- none
 */
void bc127Stop(void) {
	sendCmdOut(BCCMD_GEN_PWROFF);
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
  String part1, part2;
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
  if(part1.equalsIgnoreCase("OPEN_OK")) {
		// "OPEN_OK BLE" -> connection established with phone app
    if(part2.equalsIgnoreCase("BLE\n")) {
			working_state.ble_state = BLESTATE_REQ_CONN;
    }
		// "OPEN_OK AVRCP" -> A2DP & AVRCP connections established with audio receiver 
    else if(part2.equalsIgnoreCase("AVRCP\n")) {
      Serial.println("AVRCP protocol open!");
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
	// "INQUIRY xxxx yyyyyy -zzdB" -> TODO: make addr/caps/stren parsing more robust!
  else if(part1.equalsIgnoreCase("INQUIRY")) {
    int len = part2.length()-1;
    String addr = part2.substring(0,12);
    String caps = part2.substring(13, 19);
    unsigned int stren = part2.substring(len-4, len-2).toInt();
    populateDevlist(addr, caps, stren);
    return ANNOT_INQ_STATE;
  }
  // Commands from Android ("RECV BLE")
  // ==================================
  else if(part1.equalsIgnoreCase("RECV BLE")) {
		// "inq" -> start BT inquiry
    if(part2.equalsIgnoreCase("inq\n")) {
      Serial.println("Received BT inquiry command");
      return BCCMD_BT_INQUIRY;
    }
		// "start_mon" -> start monitoring
    else if(part2.equalsIgnoreCase("start_mon\n")) {
      return BCCMD_BT_STARTMON;
    }
		// "stop_mon" -> stop monitoring
    else if(part2.equalsIgnoreCase("stop_mon\n")) {
      return BCCMD_BT_STOPMON;
    }
		// "start_rec" -> start recording
    else if(part2.equalsIgnoreCase("start_rec\n")) {
      return BCCMD_BT_STARTREC;
    }
		// "stop_rec" -> stop recording
    else if(part2.equalsIgnoreCase("stop_rec\n")) {
      return BCCMD_BT_STOPREC;
    }
		// "vol+" -> volume up
    else if(part2.equalsIgnoreCase("vol+\n")) {
      return BCCMD_BT_VOLUP;
    }
		// "vol-" -> volume down
    else if(part2.equalsIgnoreCase("vol-\n")) {
      return BCCMD_BT_VOLDOWN;
    }
		// "conn xxxx" -> open A2DP connection to xxxx
    else if(part2.substring(0, 4).equalsIgnoreCase("conn")) {
      int len = part2.substring(5).length()-1;
      peer_address = part2.substring(5, (5+len));
      Serial.print("peer_address: "); Serial.println(peer_address);
      return BCCMD_BT_OPEN;
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
    case BCCMD_GEN_RESET:
    // Serial.println("Resetting BC127...");
    cmdLine = "RESET\r";
    break;
		// Power-off
		case BCCMD_GEN_PWROFF:
		// Serial.println("Switching BC127 off");
		cmdLine = "POWER OFF\r";
		break;
		// Power-on
		case BCCMD_GEN_PWRON:
		// Serial.println("Waking BC127 up");
		cmdLine = "POWER ON\r";
		break;
    // Start advertising on BLE
    case BCCMD_BLE_ADV_ON:
    // Serial.println("Starting BLE advertising");
    cmdLine = "ADVERTISING ON\r";
    break;
		// Stop advertising
		case BCCMD_BLE_ADV_OFF:
		cmdLine = "ADVERTISING OFF\r";
		break;
    // Start inquiry on BT for 10 s, first clear the device list
    case BCCMD_BT_INQUIRY:
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
    case BCCMD_BT_OPEN:
    Serial.print("Opening BT connection @"); Serial.println(peer_address);
    cmdLine = "OPEN " + peer_address + " A2DP\r";
    break;
		// Start monitoring -> AVRCP play
    case BCCMD_BT_STARTMON:
    cmdLine = "MUSIC PLAY\r";
    break;
		// Stop monitoring -> AVRCP pause
    case BCCMD_BT_STOPMON:
    cmdLine = "MUSIC PAUSE\r";
    break;
		// Start recording
    case BCCMD_BT_STARTREC:
    break;
		// Stop recording
    case BCCMD_BT_STOPREC:
    break;
		// Volume up -> AVRCP volume up
    case BCCMD_BT_VOLUP:
    cmdLine = "VOLUME UP\r";
    break;
		// Volume down -> AVRCP volume down
    case BCCMD_BT_VOLDOWN:
    cmdLine = "VOLUME DOWN\r";
    break;
    // Results of the inquiry -> store devices with address & signal strength
    case ANNOT_INQ_STATE:
    for(unsigned int i = 0; i < found_dev; i++) {
      devString = dev_list[i].address;
      devString.concat(" ");
      devString.concat(dev_list[i].strength);
      cmdLine = "SEND BLE " + devString + "\r";
      Serial4.print(cmdLine);
    }
    cmdLine = "";
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
