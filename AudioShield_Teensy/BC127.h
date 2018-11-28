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
enum outputMsg {
  BCCMD_NOTHING = 0,
  BCCMD_GEN_RESET,
	BCCMD_GEN_PWROFF,
	BCCMD_GEN_PWRON,
  BCCMD_BLE_ADV_ON,
	BCCMD_BLE_ADV_OFF,
  BCCMD_BLE_SEND,
  BCCMD_BT_INQUIRY,
  BCCMD_BT_LIST,
  BCCMD_BT_OPEN,
  BCCMD_BT_STATUS,
  BCCMD_BT_VOLUP,
  BCCMD_BT_VOLDOWN,
  BCCMD_BT_STARTMON,
  BCCMD_BT_STOPMON,
  BCCMD_BT_STARTREC,
  BCCMD_BT_STOPREC,
  ANNOT_INQ_STATE,
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