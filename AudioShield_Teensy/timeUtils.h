/*
 * timeUtils.h
 */
 
#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#include "main.h"

#define DEFAULT_TIME					946684800 // 01.01.2000, 00h00m00

// Time sources
enum tSources {
	TSOURCE_NONE,
	TSOURCE_GPS,
	TSOURCE_BLE
};
extern enum tSources					time_source;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */

void setDefaultTime(void);
void adjustTime(enum tSources source);
void timerRecDone(void);
void alarmNextRec(void);

#endif /* _TIMEUTILS_H_ */