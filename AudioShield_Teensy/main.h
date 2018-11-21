/*
 * main.h
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <Bounce.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "gpsRoutines.h"
#include "SDutils.h"
#include "BC127.h"
#include "IOutils.h"

// Audio mixer channels
#define MIXER_CH_REC          0
#define MIXER_CH_SDC          1

// Recording states...
enum recState {
	RECSTATE_OFF,
	RECSTATE_REQ_ON,
	RECSTATE_WAIT,
	RECSTATE_REQ_OFF,
	RECSTATE_ON
};
extern enum recState					rec_state;
// Monitoring states...
enum monState {
	MONSTATE_OFF,
	MONSTATE_REQ_ON,
	MONSTATE_REQ_OFF,
	MONSTATE_ON
};
extern enum monState					mon_state;
// Bluetooth states...
// classic (BT)
enum btState {
  BTSTATE_IDLE,
  BTSTATE_INQUIRY,
  BTSTATE_REQ_CONN,
	BTSTATE_CONNECTED,
  BTSTATE_PLAY,
	BTSTATE_STOP
};
extern enum btState 					bt_state;
// low energy (BLE)
enum bleState {
  BLESTATE_IDLE,
  BLESTATE_ADV,
  BLESTATE_REQ_CONN,
	BLESTATE_CONNECTED
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


#endif /* _MAIN_H_ */