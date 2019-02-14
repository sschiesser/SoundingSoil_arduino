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
unsigned int									found_dev;
// Address of the connected BT device
String 												BT_peer_address;
// Name of the connected BT device
String												BT_peer_name;
// ID's (A2DP&AVRCP) of the established BT connection
int														BT_conn_id1;
int														BT_conn_id2;
// ID of the established BLE connection
int														BLE_conn_id;

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
	// Alarm.delay(2000);
	// bc127BlueOff();
	// Alarm.delay(2000);
}

/* bc127BlueOn(void)
 * -----------------
 * Switch on the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOn(void) {
	sendCmdOut(BCCMD_BLUE_ON);
}

/* bc1267BlueOff(void)
 * -------------------
 * Switch off the Bluetooth interface
 * IN:	- none
 * OUT:	- none
 */
void bc127BlueOff(void) {
	sendCmdOut(BCCMD_BLUE_OFF);
}

/* bc127Reset(void)
 * ----------------
 * Reset the BC127 device
 * IN:	- none
 * OUT:	- none
 */
void bc127Reset(void) {
	// sendCmdOut(BCCMD_RESET);
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
		sendCmdOut(BCCMD_ADV_OFF);
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
	String notif, param1, param2, param3, param4, param5, param6, param7, param8, param9, trash;
  unsigned int nb_params = 0;
  int slice1 = input.indexOf(" ");
  int slice2 = input.indexOf(" ", slice1+1);
	int slice3 = input.indexOf(" ", slice2+1);
	int slice4 = input.indexOf(" ", slice3+1);
	int slice5 = input.indexOf(" ", slice4+1);
	int slice6 = input.indexOf(" ", slice5+1);
	int slice7 = input.indexOf(" ", slice6+1);
	int slice8 = input.indexOf(" ", slice7+1);
	int slice9 = input.indexOf(" ", slice8+1);
	int slice10 = input.indexOf(" ", slice9+1);
	MONPORT.printf("From BC127: %s\n", input.c_str());
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
					// 5+ parameters
					else {
						param4 = input.substring(slice4 + 1, slice5);
						if(slice6 == -1) {
							param5 = input.substring(slice5 + 1);
							nb_params = 5;
						}
						// 6+ parameters
						else {
							param5 = input.substring(slice5 + 1, slice6);
							if(slice7 == -1) {
								param6 = input.substring(slice6 + 1);
								nb_params = 6;
							}
							// 7+ parameters
							else {
								param6 = input.substring(slice6 + 1, slice7);
								if(slice8 == -1) {
									trash = input.substring(slice7 + 1);
									nb_params = 7;
								}
								// 8+ paramters
								else {
									param7 = input.substring(slice7 + 1, slice8);
									if(slice8 == -1) {
										trash = input.substring(slice8 + 1);
										nb_params = 8;
									}
									// 9+ parameters
									else {
										param8 = input.substring(slice8 + 1, slice9);
										if(slice9 == -1) {
											trash = input.substring(slice9 + 1);
											nb_params = 9;
										}
										// 10+ parameters
										else {
											param9 = input.substring(slice9 + 1, slice10);
											if(slice10 == -1) {
												trash = input.substring(slice10 + 1);
												nb_params = 10;
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
	// MONPORT.printf("#params: %d\n", nb_params);

	// MONPORT.printf("%d parameters found:\n- notif = %s\n", nb_params, notif.c_str());
	
	switch(nb_params) {
		// PENDING
		// INQU_OK
		// PAIR_PENDING
		// READY
		case 0: {
			if(notif.equalsIgnoreCase("PENDING")) {
				// return BCNOT_INQ_START;
			}
			else if(notif.equalsIgnoreCase("INQU_OK")) {
				return BCNOT_INQ_DONE;
			}
			else if(notif.equalsIgnoreCase("PAIR_PENDING")) {
			}
			else if(notif.equalsIgnoreCase("READY")) {
			}
			else {
				// MONPORT.println(notif);
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
		// TIME (timestamp)
		case 1: {
			// MONPORT.printf("- param1 = %s\n", param1.c_str());
			if(notif.equalsIgnoreCase("A2DP_STREAM_START")) {
			}
			else if(notif.equalsIgnoreCase("A2DP_STREAM_SUSPEND")) {
			}
			else if(notif.equalsIgnoreCase("AVRCP_PLAY")) {
				// working_state.mon_state = MONSTATE_REQ_ON;
			}
			else if(notif.equalsIgnoreCase("AVRCP_STOP")) {
			}
			else if(notif.equalsIgnoreCase("AVRCP_PAUSE")) {
				// working_state.mon_state = MONSTATE_REQ_OFF;
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
				// MONPORT.printf("%s %s\n", notif, param1);
			}
			break;
		}
			
		// ABS_VOL [link_ID](value)
		// BLE_READ [link_ID] handle
		// CLOSE_ERROR [link_ID] (profile)
		// LINK_LOSS [link_ID] (status)
		// NAME [addr] [remote_name]
		// OPEN_ERROR [link_ID] (profile)
		case 2: {
			// MONPORT.printf("- param1 = %s\n", param1.c_str());
			// MONPORT.printf("- param2 = %s\n", param2.c_str());
			if(notif.equalsIgnoreCase("ABS_VOL")) {
				vol_value = (float)param2.toInt()/ABS_VOL_MAX_VAL;
				if(working_state.ble_state == BLESTATE_CONNECTED) return BCNOT_VOL_LEVEL;
			}
			else if(notif.equalsIgnoreCase("BLE_READ")) {
			}
			else if(notif.equalsIgnoreCase("CLOSE_ERROR")) {
			}
			else if(notif.equalsIgnoreCase("LINK_LOSS")) {
			}
			else if(notif.equalsIgnoreCase("NAME")) {
				BT_peer_name = param2;
				return BCNOT_BT_STATE;
			}
			else if(notif.equalsIgnoreCase("OPEN_ERROR")) {
			}
			break;
		}
		
		// AT [link_ID] (length) (data)
		// CLOSE_OK [link_ID] (profile) (Bluetooth address)
		// NAME [addr] (remote) (name)
		// OPEN_OK [link_ID] (profile) (Bluetooth address)
		// RECV [link_ID] (size) (report data)
		case 3: {
			// MONPORT.printf("- param1 = %s\n", param1.c_str());
			// MONPORT.printf("- param2 = %s\n", param2.c_str());
			// MONPORT.printf("- param3 = %s\n", param3.c_str());
			if(notif.equalsIgnoreCase("AT")) {
			}
			else if(notif.equalsIgnoreCase("CLOSE_OK")) {
				if(param1.toInt() == BT_conn_id1) {	
					if(working_state.bt_state != BTSTATE_OFF) working_state.bt_state = BTSTATE_REQ_DISC;
				}
				else if(param1.toInt() == BLE_conn_id) {
					if(working_state.ble_state != BLESTATE_OFF) working_state.ble_state = BLESTATE_REQ_DIS;
				}
			}
			else if(notif.equalsIgnoreCase("NAME")) {
				BT_peer_name = param2 + " " + param3;
				return BCNOT_BT_STATE;
			}
			else if(notif.equalsIgnoreCase("OPEN_OK")) {
				if(param2.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param3;
					MONPORT.printf("A2DP connection opened. Conn ID: %d, peer address = %s\n",
						BT_conn_id1, BT_peer_address.c_str());
					working_state.bt_state = BTSTATE_REQ_CONN;
				}
				else if(param2.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param3;
					MONPORT.printf("AVRCP connection opened. Conn ID: %d, peer address (check) = %s\n", BT_conn_id2, BT_peer_address.c_str());
				}
				else if(param2.equalsIgnoreCase("BLE")) {
					BLE_conn_id = param1.toInt();
					// MONPORT.printf("BLE connection opened. Conn ID: %d\n", BLE_conn_id);
					working_state.ble_state = BLESTATE_REQ_CONN;
				}
			}
			else if(notif.equalsIgnoreCase("RECV")) {
				// BLE commands:
				// - "inq"
				// - "disc"
				if(param1.toInt() == BLE_conn_id) {
					// MONPORT.println("Receiving 1-param BLE command");
					if(param3.equalsIgnoreCase("inq")) {
					  // MONPORT.println("Received BT inquiry command");
						return BCCMD_INQUIRY;
					}
					if(param3.equalsIgnoreCase("disc")) {
						working_state.bt_state = BTSTATE_REQ_DISC;
					}
				}
			}
			else {
				MONPORT.printf("%s %s %s\n", notif, param1, param2);
			}
			break;
		}
		
		// BLE_CHAR [link_ID] type uuid handle
		// BLE_INDICATION [link_ID] handle size data
		// BLE_NOTIFICATION [link_ID] handle size data
		// BLE_READ_RES [link_ID] handle size data
		// BLE_SERV [link_ID] type uuid handle
		// BLE_WRITE [link_ID] handle size data
		// INQUIRY(BTADDR) (NAME) (COD) (RSSI)
		// STATE (connected) (connectable) (discoverable) (ble)
		// RECV [link_ID] (size) (report data) <-- (report data) with 2 parameters
		case 4: {
			// MONPORT.printf("- param1 = %s\n", param1.c_str());
			// MONPORT.printf("- param2 = %s\n", param2.c_str());
			// MONPORT.printf("- param3 = %s\n", param3.c_str());
			// MONPORT.printf("- param4 = %s\n", param4.c_str());
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
			else if(notif.equalsIgnoreCase("STATE")) {
				if(!param1.substring(param1.length()-2, param1.length()-1).toInt()) {
					return BCNOT_BT_STATE;
				}
			}
			else if(notif.equalsIgnoreCase("INQUIRY")) {
				String addr = param1;
				String name = param2.substring(1, (param2.length()-1));
				String caps = param3;
				unsigned int stren = param4.substring(1, 3).toInt();
				populateDevlist(addr, name, caps, stren);
				return BCNOT_INQ_STATE;
			}
			else if(notif.equalsIgnoreCase("RECV")) {
				// BLE commands/requests:
				// - "conn {address}"
				// - "time {ts}"
				// - "rec {start/stop/?}"
				// - "mon {start/stop/?}"
				// - "vol {+/-/?}"
				// - "bt {?}"
				// - "rwin {?}"
				// - "filepath {?}"
				if(param1.toInt() == BLE_conn_id) {
					// MONPORT.println("Receiving 2-params BLE command");
					if(param3.equalsIgnoreCase("conn")) {
						BT_peer_address = param4;
						return BCCMD_DEV_CONNECT;
					}
					else if(param3.equalsIgnoreCase("time")) {
						unsigned long rec_time = param4.toInt();
						if(rec_time > MIN_TIME_DEC) {
							setCurTime(rec_time, TSOURCE_BLE);
							MONPORT.printf("Timestamp received: %ld\n", rec_time);
						}
						else {
							MONPORT.println("Received time not correct!");
						}
					}
					else if(param3.equalsIgnoreCase("rec")) {
						if(param4.equalsIgnoreCase("start")) return BCCMD_REC_START;
						else if(param4.equalsIgnoreCase("stop")) return BCCMD_REC_STOP;
						else if(param4.equalsIgnoreCase("?")) return BCNOT_REC_STATE;
						else {
							// MONPORT.println("BLE rec command not listed");
						}
					}
					else if(param3.equalsIgnoreCase("mon")) {
						if(param4.equalsIgnoreCase("start")) working_state.mon_state = MONSTATE_REQ_ON;
						else if(param4.equalsIgnoreCase("stop")) working_state.mon_state = MONSTATE_REQ_OFF;
						else if(param4.equalsIgnoreCase("?")) return BCNOT_MON_STATE;
						else {
							// MONPORT.println("BLE mon command not listed");
						}
					}
					else if(param3.equalsIgnoreCase("vol")) {
						if(working_state.bt_state == BTSTATE_CONNECTED) {
							if(param4.equalsIgnoreCase("+")) {
								return BCCMD_VOL_UP;
							}
							else if(param4.equalsIgnoreCase("-")) {
								return BCCMD_VOL_DOWN;
							}
							else if(param4.equalsIgnoreCase("?")) {
								return BCNOT_VOL_LEVEL;
							}
							else {
								// MONPORT.println("BLE vol command not listed");
							}
						}
						else {
							return BCERR_VOL_BT_DIS;
						}
					}
					else if(param3.equalsIgnoreCase("bt")) {
						if(param4.equalsIgnoreCase("?")) {
							// return BCNOT_BT_STATE;
							return BCCMD_STATUS;
						}
					}
					else if(param3.equalsIgnoreCase("rwin")) {
						if(param4.equalsIgnoreCase("?")) {
							return BCNOT_RWIN_VALS;
						}
						else {
							return BCERR_RWIN_BAD_REQ;
							// MONPORT.println("RWIN request not listed");
						}
					}
					else if(param3.equalsIgnoreCase("filepath")) {
						if(param4.equalsIgnoreCase("?")) {
							return BCNOT_FILEPATH;
						}
					}
				}
			}
			else {
				// MONPORT.printf("%s %s %s %s\n", notif, param1, param2, param3);
			}
			break;
		}
		
		// INQUIRY(BTADDR) ("NAME SURNAME") (COD) (RSSI)
		// LINK [link_ID] (STATE) (PROFILE) (BTADDR) (INFO)
		// RECV [link_ID] (size) (report data) <-- (report data) with 3 parameters
		case 5: {
			if(notif.equalsIgnoreCase("INQUIRY")) {
				String addr = param1;
				String name = param2 + " " + param3;
				String caps = param4;
				unsigned int stren = param5.substring(1, 3).toInt();
				populateDevlist(addr, name, caps, stren);
				return BCNOT_INQ_STATE;
			}
			else if(notif.equalsIgnoreCase("LINK")) {
				// MONPORT.println("LINK5 received");
				if(param3.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id1);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
					return BCCMD_BT_NAME;
				}
				else if(param3.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id2);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
				}
			}
			break;
		}
		
		// LINK [link_ID] (state) (profile) (btaddr) (info1) (info2)
		// RECV [link_ID] (size) (report data) <-- (report data) with 4 parameters
		case 6: {
			if(notif.equalsIgnoreCase("RECV")) {
				// BLE commands:
				// - "rwin {duration} {period} {occurences}"
				if(param1.toInt() == BLE_conn_id) {
					if(param3.equalsIgnoreCase("rwin")) {
						unsigned int d, p;
						d = param4.toInt();
						p = param5.toInt();
						if(d < p) {
							breakTime(d, rec_window.length);
							breakTime(p, rec_window.period);
							rec_window.occurences = param6.toInt();
							return BCNOT_RWIN_OK;
						}
						else {
							return BCERR_RWIN_WRONG_PARAMS;
						}
					}
				}
			}
			else if(notif.equalsIgnoreCase("LINK")) {
				MONPORT.println("LINK6 received");
				if(param3.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id1);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
					return BCCMD_BT_NAME;
				}
				else if(param3.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id2);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
				}
			}
			else {
				// MONPORT.printf("%s %s %s %s %s %s\n", notif, param1, param2, param3, param4, param5);
			}
			break;
		}
		
		// LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3)
		case 7: {
			if(notif.equalsIgnoreCase("LINK")) {
				MONPORT.println("LINK7 received");
				if(param3.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id1);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
					return BCCMD_BT_NAME;
				}
				else if(param3.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id2);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
				}
			}
			break;
		}
		
		// LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4)
		case 8: {
			if(notif.equalsIgnoreCase("LINK")) {
				MONPORT.println("LINK8 received");
				if(param3.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id1);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
					return BCCMD_BT_NAME;
				}
				else if(param3.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id2);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
				}
			}
			break;
		}
		
		// LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3) (info4) (info5)
		case 9: {
			if(notif.equalsIgnoreCase("LINK")) {
				MONPORT.println("LINK9 received");
				if(param3.equalsIgnoreCase("A2DP")) {
					BT_conn_id1 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id1);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
					return BCCMD_BT_NAME;
				}
				else if(param3.equalsIgnoreCase("AVRCP")) {
					BT_conn_id2 = param1.toInt();
					BT_peer_address = param4;
					MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_conn_id2);
					working_state.bt_state = BTSTATE_CONNECTED;
					// return BCNOT_BT_STATE;
				}
			}
			break;
		}
		
		default:
			// MONPORT.println(input.c_str());
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
	unsigned int l, p, o;
  
  switch(msg) {
		/* --------
		 * COMMANDS
		 * -------- */
    case BCCMD__NOTHING:
			break;
    // Start advertising on BLE
    case BCCMD_ADV_ON: {
			cmdLine = "ADVERTISING ON\r";
			break;
		}
		// Stop advertising
		case BCCMD_ADV_OFF: {
			cmdLine = "ADVERTISING OFF\r";
			break;
		}
		// Power-off
		case BCCMD_BLUE_OFF: {
			MONPORT.println("Switching bluetooth off");
			cmdLine = "POWER OFF\r";
			break;
		}
		// Power-on
		case BCCMD_BLUE_ON: {
			MONPORT.println("Switching bluetooth on");
			cmdLine = "POWER ON\r";
			break;
		}
		// Ask for friendly name of connected BT device
		case BCCMD_BT_NAME: {
			cmdLine = "NAME " + String(BT_peer_address) + "\r";
			break;
		}
		// Open A2DP connection with 'BT_peer_address'
    case BCCMD_DEV_CONNECT: {
			if(searchDevlist(BT_peer_address)) {
				MONPORT.printf("Opening BT connection @%s (%s)\n", BT_peer_address.c_str(), BT_peer_name.c_str());
				cmdLine = "OPEN " + BT_peer_address + " A2DP\r";
			}
			else {
				cmdLine = "SEND " + String(BLE_conn_id) + " CONN ERR NO BT DEVICE!\r";
			}
			break;
		}
		// Close connection with BT device
		case BCCMD_DEV_DISCONNECT1: {
			cmdLine = "CLOSE " + String(BT_conn_id1) + "\r";
			break;
		}
		case BCCMD_DEV_DISCONNECT2: {
			cmdLine = "CLOSE " + String(BT_conn_id2) + "\r";
			break;
		}
    // Start inquiry on BT for 10 s, first clear the device list
    case BCCMD_INQUIRY: {
			// MONPORT.println("Start inquiry");
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
		// Pause monitoring -> AVRCP pause
		case BCCMD_MON_PAUSE: {
			working_state.mon_state = MONSTATE_REQ_OFF;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				cmdLine = "MUSIC " + String(BT_conn_id1) + " PAUSE\r";
			}
			break;
		}
		// Start monitoring -> AVRCP play
    case BCCMD_MON_START: {
			// working_state.mon_state = MONSTATE_REQ_ON;
			// if(working_state.bt_state == BTSTATE_CONNECTED) {
				cmdLine = "MUSIC " + String(BT_conn_id1) + " PLAY\r";
			// }
			break;
		}
		// Stop monitoring -> AVRCP pause
    case BCCMD_MON_STOP: {
			working_state.mon_state = MONSTATE_REQ_OFF;
			if(working_state.bt_state == BTSTATE_CONNECTED) {
				cmdLine = "MUSIC " + String(BT_conn_id1) + " STOP\r";
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
		// Reset
    case BCCMD_RESET: {
			MONPORT.println("Resetting BC127...");
			cmdLine = "RESET\r";
			break;
		}
		// Status
		case BCCMD_STATUS: {
			cmdLine = "STATUS\r";
			break;
		}
		// Volume level request
		case BCCMD_VOL_A2DP: {
			cmdLine = "VOLUME " + String(BT_conn_id1) + "\r";
			break;
		}
		// Volume up -> AVRCP volume up
    case BCCMD_VOL_UP: {
			cmdLine = "VOLUME " + String(BT_conn_id1) + " UP\r";
			break;
		}
		// Volume down -> AVRCP volume down
    case BCCMD_VOL_DOWN: {
			cmdLine = "VOLUME " + String(BT_conn_id1) + " DOWN\r";
			break;
		}
		/* -------------
		 * NOTIFICATIONS
		 * ------------- */
		// BT state notification
		case BCNOT_BT_STATE: {
			cmdLine = "SEND " + String(BLE_conn_id);
			if(working_state.bt_state == BTSTATE_CONNECTED) cmdLine += (" BT " + BT_peer_name + "\r");
			else if(working_state.bt_state == BTSTATE_INQUIRY) cmdLine += " BT INQ\r";
			else cmdLine += " BT IDLE\r";
			break;
		}
		// FILEPATH notification
		case BCNOT_FILEPATH: {
			cmdLine = "SEND " + String(BLE_conn_id) + " FP " + rec_path + "\r";
			// MONPORT.printf("Sending: %s\n", cmdLine.c_str());
			break;
		}
		// Inquiry sequence done -> send notification
		case BCNOT_INQ_DONE: {
			cmdLine = "SEND " + String(BLE_conn_id) + " INQ DONE\r";
			break;
		}
		// Starting inquiry sequences -> send notification
		case BCNOT_INQ_START: {
			cmdLine = "SEND " + String(BLE_conn_id) + " INQ START\r";
			break;
		}
    // Results of the inquiry -> store devices with address & signal strength
    case BCNOT_INQ_STATE: {
			for(unsigned int i = 0; i < found_dev; i++) {
				devString = dev_list[i].address;
				devString.concat(" ");
				devString.concat(dev_list[i].strength);
				cmdLine = "SEND " + String(BLE_conn_id) + " INQ " + devString + "\r";
				BLUEPORT.print(cmdLine);
			}
			cmdLine = "";
			break;
		}
		// MON state notification
		case BCNOT_MON_STATE: {
			cmdLine = "SEND " + String(BLE_conn_id);
			if(working_state.mon_state == MONSTATE_ON) cmdLine += " MON ON\r";
			else cmdLine += " MON OFF\r";
			break;
		}
		// REC state notification
		case BCNOT_REC_STATE: {
			cmdLine = "SEND " + String(BLE_conn_id);
			if(working_state.rec_state == RECSTATE_ON) cmdLine += " REC ON\r";
			else if((working_state.rec_state == RECSTATE_WAIT) || (working_state.rec_state == RECSTATE_IDLE)) cmdLine += " REC WAIT\r";
			else cmdLine += " REC OFF\r";
			break;
		}
		// RWIN command confirmation
		case BCNOT_RWIN_OK: {
			cmdLine = "SEND " + String(BLE_conn_id) + " RWIN PARAMS OK\r";
			break;
		}
		// RWIN values notification
		case BCNOT_RWIN_VALS: {
			l = makeTime(rec_window.length);
			p = makeTime(rec_window.period);
			o = rec_window.occurences;
			cmdLine = "SEND " + String(BLE_conn_id) + " RWIN " + String(l) + " " + String(p) + " " + String(o) + "\r";
			break;
		}
		// VOL level notification
		case BCNOT_VOL_LEVEL: {
			cmdLine = "SEND " + String(BLE_conn_id) + " VOL " + String(vol_value) + "\r";
			break;
		}
		/* ------
		 * ERRORS
		 * ------ */
		// RWIN ERR BAD REQUEST
		case BCERR_RWIN_BAD_REQ: {
			cmdLine = "SEND " + String(BLE_conn_id) + " RWIN ERR BAD REQUEST!\r";
			break;
		}
		case BCERR_RWIN_WRONG_PARAMS: {
			cmdLine = "SEND " + String(BLE_conn_id) + " RWIN ERR WRONG PARAMS!\r";
			break;
		}
		// VOL ERR NO DEVICE
		case BCERR_VOL_BT_DIS: {
			cmdLine = "SEND " + String(BLE_conn_id) + " VOL ERR NO BT DEVICE!\r";
			break;
		}
		// No recognised command -> send negative confirmation
    default:
			return false;
			break;
  }
	if(cmdLine != "") {
		MONPORT.printf("To BC127: %s\n", cmdLine.c_str());
	}
	// Send the prepared command line to UART
	BLUEPORT.print(cmdLine);
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
void populateDevlist(String addr, String name, String caps, unsigned int stren) {
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
		dev_list[lastPos].name = name;
    dev_list[lastPos].capabilities = caps;
    dev_list[lastPos].strength = stren;
    found_dev += 1;
  }
  
  // MONPORT.println("Device list:");
  // for(unsigned int i = 0; i < found_dev; i++) {
    // MONPORT.print(i); MONPORT.print(": ");
    // MONPORT.print(dev_list[i].address); MONPORT.print(", ");
    // MONPORT.print(dev_list[i].capabilities); MONPORT.print(", ");
    // MONPORT.println(dev_list[i].strength);
  // }
}


/* searchDevlist(String)
 * ---------------------
 * Search for the requested address into the previously 
 * filled device list, in order to open a A2DP connection.
 * IN:	- device address (String)
 * OUT:	- device found (bool)
 */
bool searchDevlist(String addr) {
	for(int i = 0; i < DEVLIST_MAXLEN; i++) {
		if(dev_list[i].address.equalsIgnoreCase(addr)) {
			MONPORT.println("Device found in list!");
			BT_peer_name = dev_list[i].name;
			return true;
		}
	}
	MONPORT.println("Nothing found in list");
	return false;
}