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
TinyGPS												gps;
// Latitude & longitude values
float													cur_lat, cur_long;							

#define GPS_STATIC						1
#if(GPS_STATIC==1)
	const char str1[] PROGMEM = "$GPRMC,201547.000,A,3014.5527,N,09749.5808,W,0.24,163.05,040109,,*1A";
	const char str2[] PROGMEM = "$GPGGA,201548.000,3014.5529,N,09749.5808,W,1,07,1.5,225.6,M,-22.5,M,18.8,0000*78";
	const char str3[] PROGMEM = "$GPRMC,201548.000,A,3014.5529,N,09749.5808,W,0.17,53.25,040109,,*2B";
	const char str4[] PROGMEM = "$GPGGA,201549.000,3014.5533,N,09749.5812,W,1,07,1.5,223.5,M,-22.5,M,18.8,0000*7C";
	const char *teststrs[4] = {str1, str2, str3, str4};
#endif

/* gpsPowerOn(void)
 * ----------------
 * Switch on GPS module
 */
void gpsPowerOn(void) {
	digitalWrite(GPS_SWITCH_PIN, HIGH);
}

/* gpsPowerOff(void)
 * -----------------
 * Switch off GPS module
 */
void gpsPowerOff(void) {
	digitalWrite(GPS_SWITCH_PIN, LOW);
}

/* gpsGetData(void)
 * ----------------
 * Capture NMEA strings on gps port and test validity
 * IN:	- none
 * OUT:	- valid fix received (bool)
 */
bool gpsGetData(void) {
	// float flat, flon;
	unsigned long age;
	byte retries = 0;
	bool fix_found = false;

	// MONPORT.print("Getting GPS data ");
	do {
#if(GPS_STATIC==1)
		for (int i=0; i<4; ++i) {
			gpsSendString(gps, teststrs[i]);
		}
#else
		gpsEncodeData(1000);
#endif	
		// gps.f_get_position(&flat, &flon, &age);
		gps.f_get_position(&cur_lat, &cur_long, &age);
		if((age == TinyGPS::GPS_INVALID_AGE) || (age > 10000)) {
			MONPORT.print(". ");
			retries++;
		}
		else {
			fix_found = true;
		}
	} while((!fix_found) && (retries < GPS_RETRIES_MAX));
	MONPORT.printf("Fix found? %d.", fix_found);
	MONPORT.printf(" fLat: %f, fLong: %f\n", cur_lat, cur_long);
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
