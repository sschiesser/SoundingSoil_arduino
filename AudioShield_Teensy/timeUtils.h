/*
 * timeUtils.h
 */
#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#include "main.h"

#define MIN_TIME_DEC					1549014885  // 01.02.2019, 09h54m00

extern time_t									received_time;
extern int										alarm_rec_id;
extern int										alarm_wait_id;
extern int										alarm_adv_id;

extern SnoozeDigital 					button_wakeup; 	// Wakeup pins on Teensy 3.6:
														// 2,4,6,7,9,10,11,13,16,21,22,26,30,33
extern SnoozeAlarm						snooze_rec;
extern SnoozeAlarm						snooze_led;
extern SnoozeBlock 						snooze_config;
extern SnoozeBlock						snooze_cpu;

// Time sources
enum tSources {
	TSOURCE_NONE,
	TSOURCE_TEENSY,
	TSOURCE_GPS,
	TSOURCE_PHONE
};
extern enum tSources					time_source;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
time_t setTimeSource(void);
void setCurTime(time_t cur_time, enum tSources source);
void setWaitAlarm(void);
void setIdleSnooze(void);
void removeWaitAlarm(void);
void removeIdleSnooze(void);
void alarmAdvTimeout(void);
void timerRecDone(void);
void alarmNextRec(void);

#endif /* _TIMEUTILS_H_ */
