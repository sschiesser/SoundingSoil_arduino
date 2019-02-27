/*
 * gpsRoutines.h
 */
#ifndef _GPSROUTINES_H_
#define _GPSROUTINES_H_

#include "main.h"

// GPS serial port
#define GPSPORT								Serial1
// #define GPS_TX_PIN						26
// #define GPS_RX_PIN						27
#define GPS_RETRIES_MAX				3

extern TinyGPS								gps;
extern float									cur_lat, cur_long;							

/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void gpsPowerOn(void);
void gpsPowerOff(void);
bool gpsGetData(void);
void gpsEncodeData(unsigned long timeout);
void gpsSendString(TinyGPS &gps, const char *str);

#endif /* _GPSROUTINES_H_ */

