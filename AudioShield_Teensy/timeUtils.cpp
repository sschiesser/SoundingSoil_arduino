/*
 * time utilities
 *
 * Miscellaneous time-related functions
 *
 */
#include "timeUtils.h"

// Time source
enum tSources												time_source;
// Last received time from external source
time_t														received_time = 0;
// Ids of the WORK-state timers
AlarmID_t													alarm_rec_id;
AlarmID_t													alarm_wait_id;
AlarmID_t													alarm_adv_id;

// Wakeup pins on Teensy 3.6:
// 2,4,6,7,9,10,11,13,16,21,22,26,30,33
SnoozeDigital 												button_wakeup;
SnoozeAlarm													snooze_rec;
SnoozeAlarm													snooze_led;

SnoozeBlock 												snooze_config(button_wakeup);
SnoozeBlock													snooze_cpu;


/* getTeensy3Time(void)
 * --------------------
 * Function needed for 'setSyncProvider()'.
 * IN:	- none
 * OUT:	- current Teensy time (time_t)
 */
time_t getTeensy3Time() {
    Teensy3Clock.compensate(-13383);
	return Teensy3Clock.get();
}


/* setTimeSource(void)
 * -------------------
 * Test if the time value stored in Teensy3Clock
 * has been already set once or is somewhere in the 70s.
 * IN:	- none
 * OUT:	- current time (time_t)
 */
time_t setTimeSource(void) {
	tmElements_t tm;
	setSyncProvider(getTeensy3Time);
	breakTime(now(), tm);
	if(debug) MONPORT.printf("Time:    Date/Time at startup: %02d.%02d.%d, %02dh%02dm%02ds\n", tm.Day, tm.Month, (tm.Year+1970) ,tm.Hour, tm.Minute, tm.Second);
	if(now() < MIN_TIME_DEC) time_source = TSOURCE_NONE;
	else time_source = TSOURCE_TEENSY;
    return now();
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
    tmElements_t tm;

	switch(source) {
		case TSOURCE_GPS:
			gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, NULL, &fix_age);
			setTime(hour, minute, second, day, month, year);
			adjustTime(GPS_TIME_OFFSET * SECS_PER_HOUR);
			Teensy3Clock.set(now());
            breakTime(cur_time, tm);
            if(debug) MONPORT.printf("Time:    Current time set to: %02dh%02dm%02ds\n", tm.Hour, tm.Minute, tm.Second);
			time_source = TSOURCE_TEENSY;
			break;

		case TSOURCE_PHONE:
			setTime(cur_time);
			Teensy3Clock.set(now());
            breakTime(cur_time, tm);
            if(debug) MONPORT.printf("Time:    Current time set to: %02dh%02dm%02ds\n", tm.Hour, tm.Minute, tm.Second);
			time_source = TSOURCE_TEENSY;
			break;

		default:
			break;
	}
	// if(debug) MONPORT.printf("Time:    Time adjusted from source#%d. Current time: %ld\n", source, now());
}

/* setNextAlarm(void)
 * ------------------
 */
void setWaitAlarm(void) {
    tmElements_t tm, tm_disp;
    time_t next_time = next_record.ts - (GPS_ENCODE_TIME_MS/1000 * GPS_ENCODE_RETRIES_MAX);
	breakTime((next_record.ts - (GPS_ENCODE_TIME_MS/1000 * GPS_ENCODE_RETRIES_MAX)), tm);
    breakTime(next_record.ts, tm_disp);
    if(next_time > now()) {
        alarm_wait_id = Alarm.alarmOnce(tm.Hour, tm.Minute, tm.Second, alarmNextRec);
        if(debug) MONPORT.printf("Time:    Next recording at %02dh%02dm%02ds\n", tm_disp.Hour, tm_disp.Minute, tm_disp.Second);
    }
    else {
        if(debug) MONPORT.printf("Time:    Mismatching time. Stopping recording\n");
        Alarm.disable(alarm_wait_id);
        working_state.rec_state = RECSTATE_REQ_OFF;
    }
}

void setIdleSnooze(void) {
	time_t delta = next_record.ts - now() - (GPS_ENCODE_TIME_MS/1000 * GPS_ENCODE_RETRIES_MAX);
	tmElements_t tm1, tm2;
	breakTime(delta, tm1);
	breakTime(next_record.ts, tm2);
	snooze_config += snooze_rec;
	snooze_rec.setRtcTimer(tm1.Hour, tm1.Minute, tm1.Second);
	if(debug) MONPORT.printf("Time:    Next recording at %02dh%02dm%02ds\n", tm2.Hour, tm2.Minute, tm2.Second);
	if(debug) MONPORT.printf("Time:    Waking up in %02dh%02dm%02ds\n", tm1.Hour, tm1.Minute, tm1.Second);
	Alarm.delay(100);
}

void removeWaitAlarm(void) {
	Alarm.free(alarm_wait_id);
}

void removeIdleSnooze(void) {
	snooze_config -= snooze_rec;
}

/* alarmAdvTimeout(void)
 * ---------------------
 * Callback of a TimeAlarms timer triggered when BLE advertising times out.
 * IN:	- none
 * OUT:	- none
 */
void alarmAdvTimeout(void) {
    if(debug) MONPORT.printf("Time:    Advertising timeout.\n");
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
	if(debug) MONPORT.printf("Time:    Recording#%d done.", (next_record.cnt+1));

	if((rec_window.occurences == 0) || (next_record.cnt < (rec_window.occurences-1))) {
		if(debug) MONPORT.println(" Going on...");
		working_state.rec_state = RECSTATE_REQ_PAUSE;
	}
	else {
		if(debug) MONPORT.println(" Finished!");
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
	removeWaitAlarm();
	if(debug) MONPORT.printf("Time:    Starting recording#%d\n", (next_record.cnt+1));
	working_state.rec_state = RECSTATE_REQ_RESTART;
}
