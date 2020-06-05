/*
 * time utilities
 *
 * Miscellaneous time-related functions
 *
 */
/*** IMPORTED EXTERNAL OBJECTS ***********************************************/
/*****************************************************************************/
#include "timeUtils.h"

/*** MODULE OBJECTS **********************************************************/
/*****************************************************************************/
/*** Constants ***************************************************************/

/*** Types *******************************************************************/
// Time source
enum tSources time_source;

/*** Variables ***************************************************************/
// Last received time from external source
time_t received_time = 0;
// Ids of the WORK-state timers
AlarmID_t alarm_rec_id;
AlarmID_t alarm_wait_id;
AlarmID_t alarm_adv_id;
AlarmID_t alarm_rem_id;
AlarmID_t alarm_request_id;
AlarmID_t alarm_req_vol_id;

// Wakeup pins on Teensy 3.6:
// 2,4,6,7,9,10,11,13,16,21,22,26,30,33
SnoozeDigital button_wakeup;
SnoozeAlarm snooze_rec;
SnoozeAlarm snooze_led;
SnoozeUSBSerial snooze_usb;

SnoozeBlock snooze_config(snooze_usb, button_wakeup);
SnoozeBlock snooze_cpu;

/*** Function prototypes *****************************************************/
/*** Macros ******************************************************************/
/*** Constant objects ********************************************************/
/*** Functions implementation ************************************************/

/*** EXPORTED OBJECTS ********************************************************/
/*****************************************************************************/
/*** Functions ***************************************************************/
/*****************************************************************************/
/* getTeensy3Time(void)
 * --------------------
 * Function needed for 'setSyncProvider()'.
 * IN:	- none
 * OUT:	- current Teensy time (time_t)
 */
time_t getTeensy3Time() {
  Teensy3Clock.compensate(0);
  return Teensy3Clock.get();
}
/*****************************************************************************/

/*****************************************************************************/
/* setTimeSource(void)
 * -------------------
 * Test if the time value stored in Teensy3Clock
 * has been already set once or is somewhere in the 70s.
 * IN:	- none
 * OUT:	- current time (time_t)
 */
time_t setTimeSource(void) {
  tmElements_t tm;
  setSyncProvider(getTeensy3Time);
  breakTime(now(), tm);
  if (debug)
    snooze_usb.printf(
        "Time:    Date/Time at startup: %02d.%02d.%d, %02dh%02dm%02ds\n",
        tm.Day, tm.Month, (tm.Year + 1970), tm.Hour, tm.Minute, tm.Second);
  if (now() < MIN_TIME_DEC)
    time_source = TSOURCE_NONE;
  else
    time_source = TSOURCE_TEENSY;
  return now();
}
/*****************************************************************************/

/*****************************************************************************/
/* setCurTime(time_t, enum tSources)
 * ---------------------------------
 * Adjust local time according to an external source
 * (GPS or app over BLE) which has provided a new value.
 * IN:	- time value (time_t)
 *			- external source (enum tSources)
 * OUT:	- none
 */
void setCurTime(time_t cur_time, enum tSources source) {
  int year;
  byte month, day, hour, minute, second;
  unsigned long fix_age;
  tmElements_t tm;

  switch (source) {
  case TSOURCE_GPS:
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL,
                       &fix_age);
    setTime(hour, minute, second, day, month, year);
    adjustTime(GPS_TIME_OFFSET * SECS_PER_HOUR);
    Teensy3Clock.set(now());
    breakTime(cur_time, tm);
    if (debug)
      snooze_usb.printf("Time:    Current time set to: %02dh%02dm%02ds\n",
                        tm.Hour, tm.Minute, tm.Second);
    time_source = TSOURCE_TEENSY;
    break;

  case TSOURCE_PHONE:
    setTime(cur_time);
    Teensy3Clock.set(now());
    breakTime(cur_time, tm);
    if (debug)
      snooze_usb.printf("Time:    Current time set to: %02dh%02dm%02ds\n",
                        tm.Hour, tm.Minute, tm.Second);
    time_source = TSOURCE_TEENSY;
    break;

  default:
    break;
  }
  // if(debug) snooze_usb.printf("Time:    Time adjusted from source#%d. Current
  // time: %ld\n", source, now());
}
/*****************************************************************************/

/*****************************************************************************/
/* setNextAlarm(void)
 * ------------------
 */
void setWaitAlarm(void) {
  tmElements_t tm, tm_disp;
  time_t next_time =
      next_record.tss - (GPS_ENCODE_TIME_MS / 1000 * GPS_ENCODE_RETRIES_MAX);
  breakTime(
      (next_record.tss - (GPS_ENCODE_TIME_MS / 1000 * GPS_ENCODE_RETRIES_MAX)),
      tm);
  breakTime(next_record.tss, tm_disp);
  if (next_time > now()) {
    alarm_wait_id =
        Alarm.alarmOnce(tm.Hour, tm.Minute, tm.Second, alarmNextRec);
    if (debug)
      snooze_usb.printf("Time:    Next recording at %02dh%02dm%02ds\n",
                        tm_disp.Hour, tm_disp.Minute, tm_disp.Second);
  } else {
    if (debug)
      snooze_usb.printf("Time:    Mismatching time. Stopping recording\n");
    Alarm.disable(alarm_wait_id);
    working_state.rec_state = RECSTATE_REQ_OFF;
  }
}
/*****************************************************************************/

/*****************************************************************************/
void setIdleSnooze(void) {
  time_t delta = next_record.tss - now() -
                 (GPS_ENCODE_TIME_MS / 1000 * GPS_ENCODE_RETRIES_MAX);
  tmElements_t now_tm, delta_tm, next_tm;
  breakTime(now(), now_tm);
  breakTime(delta, delta_tm);
  breakTime(next_record.tss, next_tm);
  snooze_config += snooze_rec;
  snooze_rec.setRtcTimer(delta_tm.Hour, delta_tm.Minute, delta_tm.Second);
  if (debug) {
    snooze_usb.printf("Time:    Current time %02dh%02dm%02ds\n", now_tm.Hour,
                      now_tm.Minute, now_tm.Second);
    snooze_usb.printf("Time:    Next recording at %02dh%02dm%02ds\n",
                      next_tm.Hour, next_tm.Minute, next_tm.Second);
    snooze_usb.printf("Time:    Waking up in %02dh%02dm%02ds\n", delta_tm.Hour,
                      delta_tm.Minute, delta_tm.Second);
  }
  Alarm.delay(100);
}
/*****************************************************************************/

/*****************************************************************************/
void removeWaitAlarm(void) { Alarm.free(alarm_wait_id); }
/*****************************************************************************/

/*****************************************************************************/
void removeIdleSnooze(void) { snooze_config -= snooze_rec; }
/*****************************************************************************/

/*****************************************************************************/
/* alarmAdvTimeout(void)
 * ---------------------
 * Callback of a TimeAlarms timer triggered when BLE advertising times out.
 * IN:	- none
 * OUT:	- none
 */
void alarmAdvTimeout(void) {
  if (debug)
    snooze_usb.printf("Time:    Advertising timeout.\n");
  Alarm.free(alarm_adv_id);
  working_state.ble_state = BLESTATE_REQ_OFF;
}
/*****************************************************************************/

/*****************************************************************************/
/* timerRecDone(void)
 * ------------------
 * Callback of a TimeAlarms timer triggered when a recording is finished.
 * IN:	- none
 * OUT:	- none
 */
void timerRecDone(void) {
  Alarm.free(alarm_rec_id);
  if (debug)
    snooze_usb.printf("Time:    Recording#%d done.", (next_record.cnt + 1));

  if ((rec_window.occurences == 0) ||
      (next_record.cnt < (rec_window.occurences - 1))) {
    if (debug)
      snooze_usb.println(" Going on...");
    working_state.rec_state = RECSTATE_REQ_PAUSE;
  } else {
    if (debug)
      snooze_usb.println(" Finished!");
    working_state.rec_state = RECSTATE_REQ_OFF;
  }
}
/*****************************************************************************/

/*****************************************************************************/
void timerRemDone(void) {
  int dur_sec = (next_record.dur.Hour * SECS_PER_HOUR) + (next_record.dur.Minute * SECS_PER_MIN) + next_record.dur.Second;
  rec_rem = dur_sec - (now() - next_record.tss);

  if(working_state.ble_state == BLESTATE_CONNECTED) {
    sendCmdOut(BCNOT_REC_REM);
  }
}
/*****************************************************************************/

/*****************************************************************************/
void timerReqVolDone(void) {
  if((working_state.ble_state == BLESTATE_CONNECTED) && (working_state.bt_state == BTSTATE_PLAY)) {
    snooze_usb.printf("Requesting volume information\n");
    sendCmdOut(BCCMD_VOL_REQ);
  }
}
/*****************************************************************************/

/*****************************************************************************/
void alarmRequestDone(void) {
  Alarm.free(alarm_request_id);
  sendCmdOut(BCREQ_TIME);
}
/*****************************************************************************/

/*****************************************************************************/
/* alarmNextRec(void)
 * ------------------
 * Callbak of a TimeAlarms alarm triggered when the waiting time
 * between two recordings is elapsed.
 * IN:	- none
 * OUT:	- none
 */
void alarmNextRec(void) {
  removeWaitAlarm();
  if (debug)
    snooze_usb.printf("Time:    Starting recording#%d\n",
                      (next_record.cnt + 1));
  working_state.rec_state = RECSTATE_REQ_RESTART;
}
/*****************************************************************************/
