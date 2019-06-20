/*
 * GPS routines
 *
 * Miscellaneous functions to read GPS NMEA tags,
 * find a specific format (RMC), slice the read string
 * and store the information in a specifict struct
 *
 */
#include "gpsRoutines.h"

// GPS instance
TinyGPS											gps;
// Latitude & longitude values
float											cur_lat, cur_long;

#define GPS_STATIC						        0
#if(GPS_STATIC==1)
	const char str1[] PROGMEM = "$GPRMC,201547.000,A,3014.5527,N,09749.5808,W,0.24,163.05,040109,,*1A";
	const char str2[] PROGMEM = "$GPGGA,201548.000,3014.5529,N,09749.5808,W,1,07,1.5,225.6,M,-22.5,M,18.8,0000*78";
	const char str3[] PROGMEM = "$GPRMC,201548.000,A,3014.5529,N,09749.5808,W,0.17,53.25,040109,,*2B";
	const char str4[] PROGMEM = "$GPGGA,201549.000,3014.5533,N,09749.5812,W,1,07,1.5,223.5,M,-22.5,M,18.8,0000*7C";
	const char *teststrs[4] = {str1, str2, str3, str4};
#endif

void initGps(void) {
	pinMode(GPS_WAKEUP_PIN, OUTPUT);
	digitalWrite(GPS_WAKEUP_PIN, LOW);
}

/* gpsPowerOn(void)
 * ----------------
 * Switch on GPS module
 */
void gpsPowerOn(void) {
	digitalWrite(GPS_RST_PIN, HIGH);
}

/* gpsPowerOff(void)
 * -----------------
 * Switch off GPS module
 */
void gpsPowerOff(void) {
	digitalWrite(GPS_RST_PIN, LOW);
}

void gpsEnable(bool wakeup) {
	if(wakeup) {
		digitalWrite(GPS_WAKEUP_PIN, HIGH);
		Alarm.delay(20);
	}
	else {
		digitalWrite(GPS_WAKEUP_PIN, LOW);
		Alarm.delay(20);
	}
}

/* gpsGetData(void)
 * ----------------
 * Capture NMEA strings on gps port and test validity
 * IN:	- none
 * OUT:	- valid fix received (bool)
 */
bool gpsGetData(void) {
	unsigned long age;
	byte retries = 0;
	bool fix_found = false;

	do {
#if(GPS_STATIC==1)
		for (int i=0; i<4; ++i) {
			gpsSendString(gps, teststrs[i]);
		}
#else
		gpsEnable(true);
		gpsEncodeData(GPS_ENCODE_TIME_MS);
		gpsEnable(false);
#endif
		gps.f_get_position(&cur_lat, &cur_long, &age);
		if((age == TinyGPS::GPS_INVALID_AGE)) {
			MONPORT.printf("age: %d\n");
		}
		else {
			fix_found = true;
		}
		retries++;
	} while((!fix_found) && (retries < GPS_ENCODE_RETRIES_MAX));
	MONPORT.printf("Fix found? %d", fix_found);
	if(fix_found) {
        next_record.gps_lat = cur_lat;
        next_record.gps_long = cur_long;
		MONPORT.printf(" fLat: %f, fLong: %f\n", next_record.gps_lat, next_record.gps_long);
		Alarm.delay((GPS_ENCODE_RETRIES_MAX - retries) * 1000);
	}
	else {
        next_record.gps_lat = NULL;
        next_record.gps_long = NULL;
        MONPORT.println("");
    }
	return fix_found;
}

/* gpsEncodeData(unsigned long)
 * ----------------------------
 * Capture incoming GPS fixes and encode them
 * IN:	- timeout limit in ms (unsigned long)
 * OUT:	- none
 */
void gpsEncodeData(unsigned long timeout) {
	unsigned long start = millis();
	do {
		while(GPSPORT.available())
			gps.encode(GPSPORT.read());
	} while (millis() - start < timeout);
}


/* gpsSendString(TinyGPS &, const char)
 * ------------------------------------
 * Send "fake" NMEA strings to the gps object for test purposes
 * IN:	- address of GPS object (TinyGPS &)
 * 			- static NMEA string (const char *)
 * OUT:	- none
 */
#if(GPS_STATIC==1)
void gpsSendString(TinyGPS &gps, const char *str) {
	while (true) {
		char c = pgm_read_byte_near(str++);
		if (!c) break;
		gps.encode(c);
	}
	gps.encode('\r');
	gps.encode('\n');
}
#endif
