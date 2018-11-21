/*
 * gpsRoutines.h
 */

#ifndef _GPSROUTINES_H_
#define _GPSROUTINES_H_

#include <Arduino.h>
#include <TinyGPS.h>
#include "main.h"

#define gpsPort								Serial1

extern TinyGPS								gps;
/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */

bool fetchGPS(void);
void gpsEncodeData(unsigned long timeout);
void gpsSendString(TinyGPS &gps, const char *str);

#endif /* _GPSROUTINES_H_ */

