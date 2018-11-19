/*
 * BC127.h
 */


#ifndef _BC127_H_
#define _BC127_H_

#include <Arduino.h>

// Bluetooth audio devices definition
#define DEVLIST_MAXLEN 6
extern struct BTdev devList[DEVLIST_MAXLEN];
extern unsigned int foundDevices;
extern String peerAddress;


// Serial command messages
enum outputMsg {
  BCCMD_NOTHING = 0,
  BCCMD_GEN_RESET,
	BCCMD_GEN_PWROFF,
	BCCMD_GEN_PWRON,
  BCCMD_BLE_ADVERTISE,
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


void initBC127(void);
int parseSerialIn(String input);
bool sendCmdOut(int msg);
void populateDevlist(String addr, String caps, unsigned int stren);

#endif /* _BC127_H_ */