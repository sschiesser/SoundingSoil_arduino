/*
 * BC127.h
 */
#ifndef _BC127_H_
#define _BC127_H_

#include "main.h"

// Max amount of BT devices to list on inquiry
#define DEVLIST_MAXLEN				6
// Time offset to adjust from GPS UTC
#define TIME_OFFSET						1 // Central European Time
// BC127 serial port
#define BLUEPORT							Serial4
// BC127 reset pin
#define BC127_RST_PIN					30

extern struct btDev 					dev_list[DEVLIST_MAXLEN];
extern unsigned int 					found_dev;
extern String									BT_peer_address;
extern String									BT_peer_name;
extern int										BT_conn_id1;
extern int										BT_conn_id2;
extern int										BLE_conn_id;

// Serial command messages
enum serialMsg {
  BCCMD__NOTHING = 0,
  BCCMD_ADV_ON,
	BCCMD_ADV_OFF,
	BCCMD_BLUE_OFF,
	BCCMD_BLUE_ON,
	BCCMD_BT_NAME,
  BCCMD_DEV_CONNECT,
	BCCMD_DEV_DISCONNECT1,
	BCCMD_DEV_DISCONNECT2,
  BCCMD_INQUIRY,
	BCCMD_MON_PAUSE,
  BCCMD_MON_START,
  BCCMD_MON_STOP,
  BCCMD_REC_START,
  BCCMD_REC_STOP,
	BCCMD_RESET,
	BCCMD_STATUS,
	BCCMD_VOL_A2DP,
  BCCMD_VOL_DOWN,
  BCCMD_VOL_UP,
	// ----------
	BCNOT_BT_STATE,
	BCNOT_FILEPATH,
	BCNOT_INQ_DONE,
	BCNOT_INQ_START,
  BCNOT_INQ_STATE,
	BCNOT_MON_STATE,
	BCNOT_REC_STATE,
	BCNOT_RWIN_OK,
	BCNOT_RWIN_VALS,
	BCNOT_VOL_LEVEL,
	// ----------
	BCERR_RWIN_BAD_REQ,
	BCERR_RWIN_WRONG_PARAMS,
	BCERR_VOL_BT_DIS,
  MAX_OUTPUTS
};

/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void initBc127(void);
void bc127Reset(void);
void bc127BlueOn(void);
void bc127BlueOff(void);
void bc127AdvStart(void);
void bc127AdvStop(void);
void bc127Inquiry(void);
int parseSerialIn(String input);
bool sendCmdOut(int msg);
void populateDevlist(String addr, String name, String caps, unsigned int stren);
bool searchDevlist(String addr);

#endif /* _BC127_H_ */