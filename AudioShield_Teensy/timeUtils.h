/*
 * timeUtils.h
 */
 
#ifndef _TIMEUTILS_H_
#define _TIMEUTILS_H_

#include "main.h"

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

void adjustTime(enum tSources source);
void alarmRecDone(void);


#endif /* _TIMEUTILS_H_ */