/*
 * SDutils.h
 */
 
#ifndef _SDUTILS_H_
#define _SDUTILS_H_

#include <Arduino.h>
#include <SD.h>
#include "gpsRoutines.h"

String createSDpath(struct gps_rmc_tag tag);



#endif /* _SDUTILS_H_ */