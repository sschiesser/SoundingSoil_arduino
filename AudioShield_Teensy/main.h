/*
 * main.h
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <Audio.h>
#include <Bounce.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Snooze.h>
#include <SPI.h>
#include <Wire.h>

#include "gpsRoutines.h"
#include "SDutils.h"
#include "BC127.h"
#include "IOutils.h"

// Audio mixer channels
#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1

// Recording states...
enum recState {
	RECSTATE_OFF,								// 0
	RECSTATE_REQ_ON,						// 1
	RECSTATE_WAIT,							// 2
	RECSTATE_REQ_OFF,						// 3
	RECSTATE_ON									// 4
};
extern enum recState					rec_state;
// Monitoring states...
enum monState {
	MONSTATE_OFF,								// 0
	MONSTATE_REQ_ON,						// 1
	MONSTATE_REQ_OFF,						// 2
	MONSTATE_ON									// 3
};
extern enum monState					mon_state;
// Bluetooth states...
// classic (BT)
enum btState {
	BTSTATE_OFF,								// 0
  BTSTATE_IDLE,								// 1
  BTSTATE_INQUIRY,						// 2
  BTSTATE_REQ_CONN,						// 3
	BTSTATE_CONNECTED,					// 4
	BTSTATE_REQ_DIS,						// 5
  BTSTATE_MUS_PLAY,						// 6
	BTSTATE_MUS_STOP						// 7
};
extern enum btState 					bt_state;
// low energy (BLE)
enum bleState {
	BLESTATE_OFF,								// 0
  BLESTATE_IDLE,							// 1
	BLESTATE_REQ_ADV,						// 2
  BLESTATE_ADV,								// 3
  BLESTATE_REQ_CONN,					// 4
	BLESTATE_CONNECTED,					// 5
	BLESTATE_REQ_DIS,						// 6
	BLESTATE_REQ_OFF						// 7
};
extern enum bleState 					ble_state;
// Working states
struct wState {
  enum recState rec_state;
  enum monState mon_state;
  enum btState bt_state;
	enum bleState ble_state;
};
extern struct wState					working_state;
// Button calls
enum bCalls {
	BCALL_NONE = 0,
	BCALL_REC = BUTTON_RECORD_PIN,
	BCALL_MON = BUTTON_MONITOR_PIN,
	BCALL_BLUE = BUTTON_BLUETOOTH_PIN
};
extern enum bCalls						button_call;

#endif /* _MAIN_H_ */