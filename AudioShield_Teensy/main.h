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
// // Monitoring states...
// enum monState {
	// MONSTATE_OFF,
	// MONSTATE_REQ,
	// MONSTATE_STREAM
// };
// extern enum monState					mon_state;
// // Recording states...
// enum recState {
	// RECSTATE_OFF,
	// RECSTATE_REQ,
	// RECSTATE_WAIT,
	// RECSTATE_RUN
// }
// extern enum recState					rec_state;
// Working states
struct wState {
  bool rec_state;
  bool mon_state;
  enum btState bt_state;
	enum bleState ble_state;
};
extern struct wState					working_state;


#endif /* _MAIN_H_ */