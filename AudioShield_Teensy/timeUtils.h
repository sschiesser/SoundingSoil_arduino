/*
 * timeUtils.h
 */
#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

/*** IMPORTED EXTERNAL OBJECTS ***********************************************/
/*****************************************************************************/
#include "main.h"

/*** EXPORTED OBJECTS ********************************************************/
/*****************************************************************************/

/*** Constants ***************************************************************/
#define MIN_TIME_DEC 1549014885 // 01.02.2019, 09h54m00
#define REQ_RECREM_INTERVAL_SEC 10
#define REQ_TIME_INTERVAL_SEC 5
#define REQ_VOL_INTERVAL_SEC 1

/*** Types *******************************************************************/
// Time sources
enum tSources { TSOURCE_NONE, TSOURCE_TEENSY, TSOURCE_GPS, TSOURCE_PHONE };
extern enum tSources time_source;

/*** Variables ***************************************************************/
extern time_t received_time;
extern AlarmID_t alarm_rec_id;
extern AlarmID_t alarm_wait_id;
extern AlarmID_t alarm_adv_id;
extern AlarmID_t alarm_rem_id;
extern AlarmID_t alarm_request_id;
extern AlarmID_t alarm_req_vol_id;

extern SnoozeDigital button_wakeup; // Wakeup pins on Teensy 3.6:
                                    // 2,4,6,7,9,10,11,13,16,21,22,26,30,33
extern SnoozeAlarm snooze_rec;
extern SnoozeAlarm snooze_led;
extern SnoozeBlock snooze_config;
extern SnoozeUSBSerial snooze_usb;
// extern SnoozeBlock						snooze_cpu;

/*** Functions ***************************************************************/
time_t setTimeSource(void);
time_t getTeensy3Time(void);
void setCurTime(time_t cur_time, enum tSources source);
void setWaitAlarm(void);
void setIdleSnooze(void);
void removeWaitAlarm(void);
void removeIdleSnooze(void);
void alarmAdvTimeout(void);
void timerRecDone(void);
void timerRemDone(void);
void alarmRequestDone(void);
void timerReqVolDone(void);
void alarmNextRec(void);

#endif /* _TIMEUTILS_H_ */
