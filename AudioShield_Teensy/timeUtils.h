/*
 * timeUtils.h
 */
#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#include "main.h"

#define DEFAULT_TIME_DEC			946684800 // 01.01.2000, 00h00m00

extern time_t									received_time;
extern int										alarm_rec_id;
extern int										alarm_wait_id;
extern int										alarm_adv_id;

// Time sources
enum tSources {
	TSOURCE_NONE,
	TSOURCE_REC,
	TSOURCE_GPS,
	TSOURCE_BLE
};
extern enum tSources					time_source;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void setDefaultTime(void);
void setCurTime(time_t cur_time, enum tSources source);
void alarmAdvTimeout(void);
void timerRecDone(void);
void alarmNextRec(void);

#endif /* _TIMEUTILS_H_ */