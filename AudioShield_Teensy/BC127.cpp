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
unsigned int                                found_dev;
// Address of the connected BT device
String                                      BT_peer_address;
// Name of the connected BT device
String                                      BT_peer_name;
// ID's (A2DP&AVRCP) of the established BT connection
int                                         BT_id_a2dp;
int                                         BT_id_avrcp;
// ID of the established BLE connection
int                                         BLE_conn_id;
// Flag indicating that the BC127 device is ready
bool                                        BC127_ready = false;
String notif, param1, param2, param3, param4, param5, param6, param7, param8, param9, trash;

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
    while(!BC127_ready) {
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
    MONPORT.printf("From BC127: %s\n", input.c_str());
    unsigned int nb_params = countParams(input);
    double tofloat;

    switch(nb_params) {
        // INQU_OK
        // READY
        case 0: {
            if(notif.equalsIgnoreCase("INQU_OK")) {
                return BCNOT_INQ_DONE;
            }
            else if(notif.equalsIgnoreCase("READY")) {
                BC127_ready = true;
            }
            break;
        }

        // AVRCP_PLAY [link_ID]
        // AVRCP_PAUSE [link_ID]
        // AVRCP_FORWARD [link_ID]
        // AVRCP_BACKWARD [link_ID]
        case 1: {
            if(notif.equalsIgnoreCase("AVRCP_PLAY")) {
                working_state.mon_state = MONSTATE_REQ_ON;
            }
            else if(notif.equalsIgnoreCase("AVRCP_PAUSE")) {
                working_state.mon_state = MONSTATE_REQ_OFF;
            }
            else if(notif.equalsIgnoreCase("AVRCP_FORWARD")) {
                // return BCCMD_REC_START;
            }
            else if(notif.equalsIgnoreCase("AVRCP_BACKWARD")) {
                // return BCCMD_REC_STOP;
            }
            break;
        }

        // ABS_VOL [link_ID](value)
        // LINK_LOSS [link_ID] (status)
        // NAME [addr] [remote_name]
        case 2: {
            if(notif.equalsIgnoreCase("ABS_VOL")) {
                vol_value = (float)param2.toInt()/ABS_VOL_MAX_VAL;
                if(working_state.ble_state == BLESTATE_CONNECTED) return BCNOT_VOL_LEVEL;
            }
            else if(notif.equalsIgnoreCase("LINK_LOSS")) {
                MONPORT.printf("link_ID: %s, status: %s\n", param1.c_str(), param2.c_str());
                if(param1.toInt() == BT_id_a2dp) {
                    if(param2.toInt() == 1) {
                        working_state.bt_state = BTSTATE_DISCONNECTED;
                    }
                    else if(param2.toInt() == 0) {
                        working_state.bt_state = BTSTATE_CONNECTED;
                    }
                    return BCNOT_BT_STATE;
                }
            }
            else if(notif.equalsIgnoreCase("NAME")) {
                if(param2.substring(0,1).equalsIgnoreCase("\"")) {
                    int strlen = param2.length();
                    BT_peer_name = param2.substring(1, (strlen-1));
                }
                else {
                    BT_peer_name = param2;
                }
                if(working_state.ble_state == BLESTATE_CONNECTED) {
                    return BCNOT_BT_STATE;
                }
            }
            break;
        }

        // CLOSE_OK [link_ID] (profile) (Bluetooth address)
        // NAME [addr] (remote) (name)
        // OPEN_OK [link_ID] (profile) (Bluetooth address)
        // RECV [link_ID] (size) (report data)
        case 3: {
            if(notif.equalsIgnoreCase("CLOSE_OK")) {
                if(param1.toInt() == BT_id_a2dp) {
                    if(working_state.bt_state != BTSTATE_OFF) working_state.bt_state = BTSTATE_REQ_DISC;
                }
                else if(param1.toInt() == BLE_conn_id) {
                    if(working_state.ble_state != BLESTATE_OFF) working_state.ble_state = BLESTATE_REQ_DISC;
                }
            }
            else if(notif.equalsIgnoreCase("NAME")) {
                BT_peer_name = param2 + " " + param3;
                if(working_state.ble_state == BLESTATE_CONNECTED) {
                    return BCNOT_BT_STATE;
                }
            }
            else if(notif.equalsIgnoreCase("OPEN_OK")) {
                if(param2.equalsIgnoreCase("A2DP")) {
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param3;
                    MONPORT.printf("A2DP connection opened. Conn ID: %d, peer address = %s\n",
                    BT_id_a2dp, BT_peer_address.c_str());
                    working_state.bt_state = BTSTATE_REQ_CONN;
                    return BCCMD_BT_NAME;
                }
                else if(param2.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param3;
                    MONPORT.printf("AVRCP connection opened. Conn ID: %d, peer address (check) = %s\n", BT_id_avrcp, BT_peer_address.c_str());
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
                // - "latlong"
                if(param1.toInt() == BLE_conn_id) {
                    // MONPORT.println("Receiving 1-param BLE command");
                    if(param3.equalsIgnoreCase("inq")) {
                        // MONPORT.println("Received BT inquiry command");
                        return BCCMD_INQUIRY;
                    }
                    else if(param3.equalsIgnoreCase("disc")) {
                        working_state.bt_state = BTSTATE_REQ_DISC;
                    }
                    else if(param3.equalsIgnoreCase("latlong")) {
                        MONPORT.println("Receiving latlong without values");
                    }
                }
            }
            break;
        }

        // INQUIRY(BTADDR) (NAME) (COD) (RSSI)
        // STATE (connected) (connectable) (discoverable) (ble)
        // RECV [link_ID] (size) (report data) <-- (report data) with 2 parameters
        case 4: {
            if(notif.equalsIgnoreCase("STATE")) {
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
                if(param1.toInt() == BLE_conn_id) {
                    // MONPORT.println("Receiving 2-params BLE command");
                    if(param3.equalsIgnoreCase("conn")) {
                        BT_peer_name = param4;
                        return BCCMD_DEV_CONNECT;
                    }
                    else if(param3.equalsIgnoreCase("time")) {
                        unsigned long rec_time = param4.toInt();
                        if(rec_time > MIN_TIME_DEC) {
                            setCurTime(rec_time, TSOURCE_PHONE);
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
                    else if(param3.equalsIgnoreCase("rec_next")) {
                        if(param4.equalsIgnoreCase("?")) return BCNOT_REC_NEXT;
                        else {
                            MONPORT.println("BLE rec_next command not listed");
                        }
                    }
                    else if(param3.equalsIgnoreCase("rec_nb")) {
                        if(param4.equalsIgnoreCase("?")) return BCNOT_REC_NB;
                        else {
                            MONPORT.println("BLE rec_nb command not listed");
                        }
                    }
                    else if(param3.equalsIgnoreCase("rec_ts")) {
                        if(param4.equalsIgnoreCase("?")) return BCNOT_REC_TS;
                        else {
                            MONPORT.println("BLE rec_ts command not listed");
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
                        if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
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
                    else if(param3.equalsIgnoreCase("latlong")) {
                        if(param4.equalsIgnoreCase("?")) {
                            return BCNOT_LATLONG;
                        }
                    }
                }
            }
            break;
        }

        // INQUIRY (BTADDR) ("NAME SURNAME") (COD) (RSSI)
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
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_a2dp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                    return BCCMD_BT_NAME;
                }
                else if(param3.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_avrcp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                }
            }
            else if(notif.equalsIgnoreCase("RECV")) {
                // BLE command/request:
                // - "latlong {lat long}"
                if(param1.toInt() == BLE_conn_id) {
                    if(param3.equalsIgnoreCase("latlong")) {
                        if((param4.c_str() == NULL) || (param5.c_str() == NULL)) {
                            MONPORT.println("Received latlong values not available!");
                            next_record.gps_source = GPS_NONE;
                            next_record.gps_lat = NULL;
                            next_record.gps_long = NULL;
                        }
                        else {
                            next_record.gps_lat = atof(param4.c_str());
                            next_record.gps_long = atof(param5.c_str());
                            next_record.gps_source = GPS_PHONE;
                        }
                    }
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
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_a2dp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                    return BCCMD_BT_NAME;
                }
                else if(param3.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_avrcp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                }
            }
            break;
        }

        // LINK [link_ID] (state) (profile) (btaddr) (info1) (info2) (info3)
        case 7: {
            if(notif.equalsIgnoreCase("LINK")) {
                MONPORT.println("LINK7 received");
                if(param3.equalsIgnoreCase("A2DP")) {
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_a2dp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                    return BCCMD_BT_NAME;
                }
                else if(param3.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_avrcp);
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
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_a2dp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                    return BCCMD_BT_NAME;
                }
                else if(param3.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_avrcp);
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
                    BT_id_a2dp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("A2DP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_a2dp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                    return BCCMD_BT_NAME;
                }
                else if(param3.equalsIgnoreCase("AVRCP")) {
                    BT_id_avrcp = param1.toInt();
                    BT_peer_address = param4;
                    MONPORT.printf("AVRCP address: %s, ID: %d\n", BT_peer_address.c_str(), BT_id_avrcp);
                    working_state.bt_state = BTSTATE_CONNECTED;
                    // return BCNOT_BT_STATE;
                }
            }
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

    switch(msg) {
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
        case BCCMD_INQUIRY: // Start inquiry on BT for 10 s, first clear the device list
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
            for(unsigned int i = 0; i < found_dev; i++) {
                devString = dev_list[i].name;
                devString.concat(" ");
                devString.concat(dev_list[i].strength);
                cmdLine = "SEND " + String(BLE_conn_id) + " INQ " + devString + "\r";
                BLUEPORT.print(cmdLine);
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
    if(cmdLine != "") {
        MONPORT.printf("To BC127: %s\n", cmdLine.c_str());
    }
    // Send the prepared command line to UART
    BLUEPORT.print(cmdLine);
    // Wait some time to let the sent line finish
    Alarm.delay(50);
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
* IN:	- device name (String)
* OUT:	- device found (bool)
*/
bool searchDevlist(String name) {
    for(int i = 0; i < DEVLIST_MAXLEN; i++) {
        if(dev_list[i].name.equalsIgnoreCase(name)) {
            MONPORT.println("Device found in list!");
            BT_peer_address = dev_list[i].address;
            return true;
        }
    }
    MONPORT.println("Nothing found in list");
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
    int slice2 = input.indexOf(" ", slice1+1);
    int slice3 = input.indexOf(" ", slice2+1);
    int slice4 = input.indexOf(" ", slice3+1);
    int slice5 = input.indexOf(" ", slice4+1);
    int slice6 = input.indexOf(" ", slice5+1);
    int slice7 = input.indexOf(" ", slice6+1);
    int slice8 = input.indexOf(" ", slice7+1);
    int slice9 = input.indexOf(" ", slice8+1);
    int slice10 = input.indexOf(" ", slice9+1);

    // no space found -> notification without parameter
    if(slice1 == -1) {
        notif = input;
        return 0;
    }
    // 1+ parameter
    else {
        notif = input.substring(0, slice1);
        if(slice2 == -1) {
            param1 = input.substring(slice1 + 1);
            return 1;
        }
        // 2+ parameters
        else {
            param1 = input.substring(slice1 + 1, slice2);
            if(slice3 == -1) {
                param2 = input.substring(slice2 + 1);
                return 2;
            }
            // 3+ parameters
            else {
                param2 = input.substring(slice2 + 1, slice3);
                if(slice4 == -1) {
                    param3 = input.substring(slice3 + 1);
                    return 3;
                }
                // 4+ parameters
                else {
                    param3 = input.substring(slice3 + 1, slice4);
                    if(slice5 == -1) {
                        param4 = input.substring(slice4 + 1);
                        return 4;
                    }
                    // 5+ parameters
                    else {
                        param4 = input.substring(slice4 + 1, slice5);
                        if(slice6 == -1) {
                            param5 = input.substring(slice5 + 1);
                            return 5;
                        }
                        // 6+ parameters
                        else {
                            param5 = input.substring(slice5 + 1, slice6);
                            if(slice7 == -1) {
                                param6 = input.substring(slice6 + 1);
                                return 6;
                            }
                            // 7+ parameters
                            else {
                                param6 = input.substring(slice6 + 1, slice7);
                                if(slice8 == -1) {
                                    trash = input.substring(slice7 + 1);
                                    return 7;
                                }
                                // 8+ paramters
                                else {
                                    param7 = input.substring(slice7 + 1, slice8);
                                    if(slice8 == -1) {
                                        trash = input.substring(slice8 + 1);
                                        return 8;
                                    }
                                    // 9+ parameters
                                    else {
                                        param8 = input.substring(slice8 + 1, slice9);
                                        if(slice9 == -1) {
                                            trash = input.substring(slice9 + 1);
                                            return 9;
                                        }
                                        // 10+ parameters
                                        else {
                                            param9 = input.substring(slice9 + 1, slice10);
                                            if(slice10 == -1) {
                                                trash = input.substring(slice10 + 1);
                                                return 10;
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
}

String cmdDevConnect(void) {
    if(searchDevlist(BT_peer_name)) {
        MONPORT.printf("Opening BT connection @%s (%s)\n", BT_peer_address.c_str(), BT_peer_name.c_str());
        return ("OPEN " + BT_peer_address + " A2DP\r");
    }
    else {
        return ("SEND " + String(BLE_conn_id) + " CONN ERR NO BT DEVICE!\r");
    }
}
String cmdInquiry(void) {
    String cmd, devString;
    for(int i = 0; i < DEVLIST_MAXLEN; i++) {
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
    if(working_state.bt_state == BTSTATE_CONNECTED) {
        return ("MUSIC " + String(BT_id_a2dp) + " PAUSE\r");
    }
    else return "";
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
    if((working_state.bt_state == BTSTATE_CONNECTED) || (working_state.bt_state == BTSTATE_PLAY)) {
        return ("MUSIC " + String(BT_id_a2dp) + " STOP\r");
    }
    else return "";
}

String notBtState(void) {
    String ret;
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        ret = "SEND " + String(BLE_conn_id);
        if(working_state.bt_state == BTSTATE_CONNECTED) {
            ret += " BT " + BT_peer_name + "\r";
        }
        else if(working_state.bt_state == BTSTATE_INQUIRY) {
            ret += " BT INQ\r";
        }
        else {
            ret += " BT disconnected\r";
        }
    }
    else ret = "";
    return ret;
}
String notFilepath(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " FP " + rec_path + "\r");
    }
    else return "";
}
String notInqDone(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " INQ DONE\r");
    }
    else return "";
}
String notInqStart(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " INQ START\r");
    }
    else return "";
}
String notLatlong(void) {
    String ret;
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        ret = "SEND " + String(BLE_conn_id) + " LATLONG ";
        if((next_record.gps_lat != NULL) && (next_record.gps_long != NULL)) {
            ret += String(next_record.gps_lat) + " " + String(next_record.gps_long);
        }
        ret += "\r";
    }
    else ret = "";
    return ret;
}
String notMonState(void) {
    String ret;
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        ret = "SEND " + String(BLE_conn_id);
        if(working_state.mon_state == MONSTATE_ON) ret += " MON ON\r";
        else ret += " MON OFF\r";
    }
    else ret = "";
    return ret;
}
String notRecNb(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " REC_NB " + (next_record.cnt+1) + "\r");
    }
    else return "";
}
String notRecNext(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " REC_NEXT " + next_record.ts + "\r");
    }
    else return "";
}
String notRecState(void) {
    String ret;
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        ret = "SEND " + String(BLE_conn_id);
        if(working_state.rec_state == RECSTATE_ON) {
            ret += " REC ON\r";
        }
        else if((working_state.rec_state == RECSTATE_WAIT) || (working_state.rec_state == RECSTATE_IDLE)) {
            ret += " REC WAIT\r";
        }
        else {
            ret += " REC OFF\r";
        }
    }
    else ret = "";
    return ret;
}
String notRecTs(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " REC_TS " + next_record.ts + "\r");
    }
    else return "";
}
String notRwinOk(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " RWIN PARAMS OK\r");
    }
    else return "";
}
String notRwinVals(void) {
    unsigned int l, p, o;
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        l = makeTime(rec_window.length);
        p = makeTime(rec_window.period);
        o = rec_window.occurences;
        return ("SEND " + String(BLE_conn_id) + " RWIN " + String(l) + " " + String(p) + " " + String(o) + "\r");
    }
    else return "";
}
String notVolLevel(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " VOL " + String(vol_value) + "\r");
    }
    else return "";
}

String errRwinBadReq(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " RWIN ERR BAD REQUEST!\r");
    }
    else return "";
}
String errRwinWrongParams(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " RWIN ERR WRONG PARAMS!\r");
    }
    else return "";
}
String errVolBtDis(void) {
    if(working_state.ble_state == BLESTATE_CONNECTED) {
        return ("SEND " + String(BLE_conn_id) + " VOL ERR NO BT DEVICE!\r");
    }
    else return "";
}
