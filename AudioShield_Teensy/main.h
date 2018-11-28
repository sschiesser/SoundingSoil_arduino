/*
 * main.h
 */
#ifndef _MAIN_H_
#define _MAIN_H_

// Arduino or Teensyduino libraries
#include <Audio.h>
#include <Bounce.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Snooze.h>
#include <SPI.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <TinyGPS.h>
#include <Wire.h>

// Own headers
#include "gpsRoutines.h"
#include "SDutils.h"
#include "BC127.h"
#include "IOutils.h"
#include "audioUtils.h"
#include "timeUtils.h"

// Monitoring serial port
#define MONITORPORT						Serial

// Wake-up sources from hibernating mode
#define WAKESOURCE_BUT_REC		BUTTON_RECORD_PIN
#define WAKESOURCE_BUT_MON		BUTTON_MONITOR_PIN
#define WAKESOURCE_BUT_BLUE		BUTTON_BLUETOOTH_PIN
#define WAKESOURCE_RTC				35

// Default recording window values
#define RWIN_LEN_DEF_S				5 // }
#define RWIN_LEN_DEF_M				0 // } Zero values -> continuous recording
#define RWIN_LEN_DEF_H				0 // }
#define RWIN_PER_DEF_S				20
#define RWIN_PER_DEF_M				0
#define RWIN_PER_DEF_H				0
#define RWIN_OCC_DEF					3 // Zero value -> infinite repetitions


// Recording states...
enum recState {
	RECSTATE_OFF,								// 0
	RECSTATE_REQ_ON,						// 1
	RECSTATE_ON,								// 2
	RECSTATE_REQ_WAIT,					// 3
	RECSTATE_WAIT,							// 4
	RECSTATE_REQ_IDLE,					// 5
	RECSTATE_IDLE,							// 6
	RECSTATE_RESTART,						// 7
	RECSTATE_REQ_OFF						// 8
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
extern volatile struct wState	working_state;
// Record informations
struct recInfo {
	time_t ts;
	tmElements_t dur;
	bool t_set;
	String path;
	unsigned int cnt;
};
extern struct recInfo					last_record;
extern struct recInfo					next_record;
// Recording window
struct rWindow {
	tmElements_t length;
	tmElements_t period;
	unsigned int occurences;
};
extern struct rWindow					rec_window;

#endif /* _MAIN_H_ */