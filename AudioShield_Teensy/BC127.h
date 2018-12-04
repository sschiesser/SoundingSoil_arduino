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
#define BC127PORT							Serial4

extern struct btDev 					dev_list[DEVLIST_MAXLEN];
extern unsigned int 					found_dev;
extern String									peer_address;

// Serial command messages
enum serialMsg {
  BCCMD_NOTHING = 0,
  BCCMD_RESET,
	BCCMD_PWR_OFF,
	BCCMD_PWR_ON,
  BCCMD_ADV_ON,
	BCCMD_ADV_OFF,
  BCCMD_BLE_SEND,
  BCCMD_INQUIRY,
  BCCMD_DEV_LIST,
  BCCMD_DEV_CONNECT,
  BCCMD_REC_START,
  BCCMD_REC_STOP,
  BCCMD_MON_START,
  BCCMD_MON_STOP,
  BCCMD_VOL_UP,
  BCCMD_VOL_DOWN,
	BCCMD_VOL_A2DP,
	BCCMD_RWIN_SET,
  BCNOT_INQ_STATE,
	BCNOT_BT_STATE,
	BCNOT_REC_STATE,
	BCNOT_MON_STATE,
	BCNOT_VOL_LEVEL,
	BCNOT_RWIN_VALS,
	BCNOT_FILEPATH,
  MAX_OUTPUTS
};

/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void bc127Init(void);
void bc127PowerOn(void);
void bc127PowerOff(void);
void bc127AdvStart(void);
void bc127AdvStop(void);
void bc127Inquiry(void);
int parseSerialIn(String input);
bool sendCmdOut(int msg);
void populateDevlist(String addr, String caps, unsigned int stren);

#endif /* _BC127_H_ */