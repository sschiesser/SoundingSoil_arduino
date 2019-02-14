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

// Time sources
enum tSources {
	TSOURCE_NONE,
	TSOURCE_TEENSY,
	TSOURCE_GPS,
	TSOURCE_BLE
};
extern enum tSources					time_source;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void setTimeSource(void);
void setCurTime(time_t cur_time, enum tSources source);
void alarmAdvTimeout(void);
void timerRecDone(void);
void alarmNextRec(void);

#endif /* _TIMEUTILS_H_ */