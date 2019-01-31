/*
 * time utilities
 * 
 * Miscellaneous time-related functions
 * 
 */
#include "timeUtils.h"

// Time source
enum tSources									time_source;
// Last received time from external source
time_t												received_time = 0;
// Ids of the WORK-state timers
int														alarm_rec_id;
int														alarm_wait_id;
int														alarm_adv_id;

/* setDefaultTime(void)
 * --------------------
 * Set the Teensy clock to DEFAULT_TIME and
 * the time source to none
 * IN:	- none
 * OUT:	- none
 */
void setDefaultTime(void) {
	setTime(DEFAULT_TIME_DEC);
	Teensy3Clock.set(DEFAULT_TIME_DEC);
	time_source = TSOURCE_NONE;
}


/* setCurTime(time_t, enum tSources)
 * ---------------------------------
 * Adjust local time according to an external source
 * (GPS or app over BLE) which has provided a new value.
 * IN:	- time value (time_t)
 *			- external source (enum tSources)
 * OUT:	- none
 */
void setCurTime(time_t cur_time, enum tSources source) {
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
			setTime(cur_time);
			Teensy3Clock.set(cur_time);
			break;
		
		case TSOURCE_REC:
			setTime(next_record.ts);
			Teensy3Clock.set(next_record.ts);
			break;
			
		default:
			break;
	}
	// MONPORT.printf("Time adjusted from source#%d. Current time: %ld\n", source, now());
}


/* alarmAdvTimeout(void)
 * ---------------------
 * Callback of a TimeAlarms timer triggered when BLE advertising times out.
 * IN:	- none
 * OUT:	- none
 */
void alarmAdvTimeout(void) {
	Alarm.free(alarm_adv_id);
	working_state.ble_state = BLESTATE_REQ_OFF;
}


/* timerRecDone(void)
 * ------------------
 * Callback of a TimeAlarms timer triggered when a recording is finished.
 * IN:	- none
 * OUT:	- none
 */
void timerRecDone(void) {
	Alarm.free(alarm_rec_id);
	
	if((rec_window.occurences == 0) || (next_record.cnt < (rec_window.occurences-1))) {
		MONPORT.printf("Recording done... counting: %d\n", next_record.cnt);
		working_state.rec_state = RECSTATE_REQ_WAIT;
	}
	else {
		MONPORT.println("Recording set finished!");
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
	Alarm.free(alarm_wait_id);
	// MONPORT.println("Next REC called");
	working_state.rec_state = RECSTATE_RESTART;
}