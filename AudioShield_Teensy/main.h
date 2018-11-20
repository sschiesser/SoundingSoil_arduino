/*
 * main.h
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include "BC127.h"

// Audio mixer channels
#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1

// Bluetooth states...
// classic (BT)
enum btState {
  BTSTATE_IDLE,
  BTSTATE_INQUIRY,
  BTSTATE_CONNECTING,
	BTSTATE_CONNECTED,
  BTSTATE_PLAY,
	BTSTATE_STOP
};
extern enum btState 					bt_state;
// low energy (BLE)
enum bleState {
  BLESTATE_IDLE,
  BLESTATE_ADV,
  BLESTATE_CONNECTING,
	BLESTATE_CONNECTED
};
extern enum bleState 					ble_state;
// Working states
struct wState {
  bool rec_state;
  bool mon_state;
  enum btState bt_state;
	enum bleState ble_state;
};
extern struct wState					working_state;


#endif /* _MAIN_H_ */