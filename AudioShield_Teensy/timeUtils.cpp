/*
 * time utilities
 * 
 * Miscellaneous time-related functions
 * 
 */

#include "timeUtils.h"

enum tSources									time_source;

 /* adjustTime(enum tSources)
 * -------------------------
 * Adjust local time after an external source
 * (GPS or app over BLE) has provided a new value.
 * IN:	- external source (enum tSources)
 * OUT:	- none
 */
void adjustTime(enum tSources source) {
	int year;
	byte month, day, hour, minute, second;
	unsigned long fix_age;
	
	switch(source) {
		case TSOURCE_GPS:
			gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &fix_age);
			setTime(hour, minute, second, day, month, year);
			adjustTime(TIME_OFFSET * SECS_PER_HOUR);
			Teensy3Clock.set(now());
			break;
			
		case TSOURCE_BLE:
			setTime(ble_time);
			Teensy3Clock.set(ble_time);
			break;
			
		default:
			break;
	}
	Serial.printf("Time adjusted from source#%d. Current time: %ld\n", source, now());
}


/* alarmRecDone(void)
 * ------------------
 * Callback of a TimeAlarms timer triggered when a record window is finished.
 * IN:	- none
 * OUT:	- none
 */
void alarmRecDone(void) {
	if((rec_window.occurences == 0) || (last_record.cnt < rec_window.occurences)) {
		working_state.rec_state = RECSTATE_REQ_WAIT;
	}
	else {
		working_state.rec_state = RECSTATE_REQ_OFF;
	}
}
