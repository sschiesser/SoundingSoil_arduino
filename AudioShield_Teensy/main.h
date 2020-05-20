/*
 * main.h
 */
#ifndef _MAIN_H_
#define _MAIN_H_

/*** IMPORTED EXTERNAL OBJECTS ***********************************************/
/*****************************************************************************/
// Arduino or Teensyduino libraries
#include <Audio.h>
#include <Bounce.h>
#include <SPI.h>
// #include <SdFat.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Snooze.h>
#include <TimeAlarms.h>
#include <TimeLib.h>
#include <TinyGPS.h>
#include <Wire.h>

// Own headers
#include "BC127.h"
#include "IOutils.h"
#include "SDutils.h"
#include "audioUtils.h"
#include "gpsRoutines.h"
#include "timeUtils.h"

/*** EXPORTED OBJECTS ********************************************************/
/*****************************************************************************/

/*** Constants ***************************************************************/
#define BLEADV_TIMEOUT_S 30

// Wake-up sources from hibernating mode
#define WAKESOURCE_BUT_REC BUTTON_RECORD_PIN
#define WAKESOURCE_BUT_MON BUTTON_MONITOR_PIN
#define WAKESOURCE_BUT_BLUE BUTTON_BLUETOOTH_PIN
#define WAKESOURCE_RTC 35

// Default recording window values
// WARNING!! If duration < 7, the FIRST recording will be 7!!
#define RWIN_DUR_DEF_SEC 0  // 0 }
#define RWIN_DUR_DEF_MIN 5  // 5 } Zero values -> continuous recording
#define RWIN_DUR_DEF_HOUR 0 // 0 }
#define RWIN_PER_DEF_SEC 0  // 0
#define RWIN_PER_DEF_MIN 0  // 0
#define RWIN_PER_DEF_HOUR 1 // 1
#define RWIN_OCC_DEF 24     // Zero value -> infinite repetitions
// DO NOT CHANGE!! D/M/Y values have to be set to
// minimum (1.1.1970) in order to obtain correct rwin times
#define RWIN_DUR_DEF_DAY 1
#define RWIN_DUR_DEF_MON 1
#define RWIN_DUR_DEF_YEAR 0
#define RWIN_PER_DEF_DAY 1
#define RWIN_PER_DEF_MON 1
#define RWIN_PER_DEF_YEAR 0
// DO NOT CHANGE!!

/*** Types *******************************************************************/
// Recording states...
enum recState {
  RECSTATE_OFF,       // 0 -> recording off
  RECSTATE_REQ_ON,    // 1 -> requesting to start recording (REC button pressed)
  RECSTATE_ON,        // 2 -> recording running
  RECSTATE_REQ_PAUSE, // 3 -> requesting to go to waiting mode (WORK state)
  RECSTATE_WAIT,      // 4 -> waiting mode (WORK state)
  RECSTATE_REQ_IDLE,  // 5 -> requesting to go to idle mode (SLEEP state)
  RECSTATE_IDLE,      // 6 -> idle mode (SLEEP state)
  RECSTATE_REQ_RESTART, // 7 -> requesting to restart recording (after wait or
                        // idle mode)
  RECSTATE_REQ_OFF // 8 -> requesting to stop recording (REC button pressed)
};
extern enum recState rec_state;
// Monitoring states...
enum monState {
  MONSTATE_OFF,    // 0 -> monitoring off
  MONSTATE_REQ_ON, // 1 -> requesting to start monitoring (MON button pressed)
  MONSTATE_ON,     // 2 -> monitoring running
  MONSTATE_REQ_OFF // 3 -> requesting to stop monitoring (MON button pressed)
};
extern enum monState mon_state;
// Bluetooth states...
// classic (BT)
enum btState {
  BTSTATE_OFF,         // 0 -> BT off
  BTSTATE_IDLE,        // 1
  BTSTATE_INQUIRY,     // 2 -> BT inquiring for audio devices
  BTSTATE_REQ_CONN,    // 3 -> requesting connection to audio device
  BTSTATE_CONNECTED,   // 4 -> connected but not playing
  BTSTATE_PLAY,        // 5 -> playing (and connected)
  BTSTATE_REQ_DISC,    // 6 -> requesting disconnection from audio device
  BTSTATE_DISCONNECTED // 7 -> BT device disconnected
};
extern enum btState bt_state;
// low energy (BLE)
enum bleState {
  BLESTATE_OFF,       // 0 -> BLE off
  BLESTATE_IDLE,      // 1
  BLESTATE_REQ_ADV,   // 2 -> requesting advertising (BLUE button pressed)
  BLESTATE_ADV,       // 3 -> advertising to phone/computer
  BLESTATE_REQ_CONN,  // 4 -> requesting connection to phone/computer
  BLESTATE_CONNECTED, // 5 -> connected to phone/computer
  BLESTATE_REQ_DISC,  // 6 -> requesting disconnection from phone/computer
  BLESTATE_REQ_OFF    // 7 -> requesting BLE off (BLUE button pressed)
};
extern enum bleState ble_state;
// Working states
struct wState {
  enum recState rec_state;
  enum monState mon_state;
  enum btState bt_state;
  enum bleState ble_state;
};
extern volatile struct wState working_state;
// Ready-to-sleep states
struct sfState {
  bool rec_ready;
  bool mon_ready;
  bool ble_ready;
  bool bt_ready;
};
extern struct sfState sleep_flags;
// GPS sources
enum gpsSource { GPS_NONE, GPS_PHONE, GPS_RECORDER };
// Record informations
struct recInfo {
  time_t tss;                // start timestamp
  time_t tsp;                // stop timestamp
  tmElements_t dur;          // duration
  tmElements_t per;          // period
  bool t_set;                // time synced?
  String rpath;              // record path on SD card
  String mpath;              // metadata path on SD card
  float gps_lat;             // GPS latitude (signed dd)
  float gps_long;            // GPS longitude (signed dd)
  enum gpsSource gps_source; // GPS source
  unsigned int cnt;          // record counter
  unsigned int rec_tot;      // total number of records
  bool man_stop;             // recording sequence manually stopped
};
extern struct recInfo last_record;
extern struct recInfo next_record;
// Recording window
struct rWindow {
  tmElements_t duration;   // length (0 -> continous recording)
  tmElements_t period;     // period (occuring every h:m:s)
  unsigned int occurences; // # of occurences (0 -> infinite repetitions)
};
extern struct rWindow rec_window;

/*** Variables ***************************************************************/
extern const bool debug;

/*** Functions ***************************************************************/

#endif /* _MAIN_H_ */
