/*
 * gpsRoutines.h
 */
#ifndef _GPSROUTINES_H_
#define _GPSROUTINES_H_

#include "main.h"

// GPS serial port
#define GPSPORT                                 Serial1
// GPS wakeup pin#
#define GPS_WAKEUP_PIN                          5
// GPS switch pin#
#define GPS_RST_PIN                             8

// #define GPS_TX_PIN						     26
// #define GPS_RX_PIN						     27

#define GPS_ENCODE_TIME_MS                      1000
#define GPS_ENCODE_RETRIES_MAX                  3

extern TinyGPS                                  gps;
extern float                                    cur_lat, cur_long;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void initGps(void);
void gpsPowerOn(void);
void gpsPowerOff(void);
void gpsEnable(bool wakeup);
bool gpsGetData(void);
void gpsEncodeData(unsigned long timeout);
void gpsSendString(TinyGPS &gps, const char *str);

#endif /* _GPSROUTINES_H_ */
