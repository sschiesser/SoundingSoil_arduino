/*
 * time utilities
 * 
 * Miscellaneous time-related functions
 * 
 */

#include "timeUtils.h"

enum tSources									time_source;

void setDefaultTime(void) {
	setTime(DEFAULT_TIME);
	Teensy3Clock.set(DEFAULT_TIME);
	time_source = TSOURCE_NONE;
}


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
		
		case TSOURCE_REC:
			setTime(next_record.ts);
			Teensy3Clock.set(next_record.ts);
			break;
			
		default:
			break;
	}
	// Serial.printf("Time adjusted from source#%d. Current time: %ld\n", source, now());
}


/* timerRecDone(void)
 * ------------------
 * Callback of a TimeAlarms timer triggered when a record window is finished.
 * IN:	- none
 * OUT:	- none
 */
void timerRecDone(void) {
	if((rec_window.occurences == 0) || (next_record.cnt < (rec_window.occurences-1))) {
		// Serial.printf("Recording done... counting: %d\n", next_record.cnt);
		working_state.rec_state = RECSTATE_REQ_WAIT;
	}
	else {
		// Serial.println("Recording set finished!");
		working_state.rec_state = RECSTATE_REQ_OFF;
	}
}

/* alarmNextRec(void)
 * ------------------
 * Callbak of a TimeAlarms alarm triggered when the waiting time 
 * between two recordings is elapsed.
 * IN:	- none
 * OUT:	- none
 */
void alarmNextRec(void) {
	// Serial.println("Next REC called");
	working_state.rec_state = RECSTATE_RESTART;
}