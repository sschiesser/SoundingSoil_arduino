/*
 * Audio utilities
 * 
 * Miscellaneous functions related to the
 * audio processes (recording, monitoring, etc.)
 * 
 */

#include "ssUtilities.h"

void prepareRecording(void) {
	bool ret;
	String rec_path = "";
	
	startLED(&leds[LED_RECORD], LED_MODE_WAITING);
	gpsPowerOn();
	delay(1000);
	ret = gpsGetData();
	gpsPowerOff();
	if(!ret) {
		// Serial.printf("No GPS fix received. timeStatus = %d, time = %ld\n", timeStatus(), now());
		// Time is not sychronized yet -> send a short warning
		if(timeStatus() == timeNotSet) {
			rec_path = createSDpath(false);
			startLED(&leds[LED_RECORD], LED_MODE_WARNING);
		}
		// Time synchronize from an older value
		else {
			rec_path = createSDpath(true);
			startLED(&leds[LED_RECORD], LED_MODE_ON);
		}
	}
	// GPS data valid -> adjust the internal time
	else {
		// adjustTime(TSOURCE_GPS);
		createSDpath(true);
	}
	setRecInfos(&next_record, rec_path, last_record.cnt);
}


void setRecInfos(struct recInfo* rec, String path, unsigned int rec_cnt) {
	unsigned long dur = rec_window.length.Second + 
										(rec_window.length.Minute * SECS_PER_MIN) + 
										(rec_window.length.Hour * SECS_PER_HOUR);
	rec->dur.Second = rec_window.length.Second;
	rec->dur.Minute = rec_window.length.Minute;
	rec->dur.Hour = rec_window.length.Hour;
	rec->path.remove(0);
	rec->path.concat(path.c_str());
	rec->cnt = rec_cnt;
	if(timeStatus() == timeNotSet) {
		rec->t_set = false;
		rec->ts = (time_t)0;
	}
	else {
		rec->t_set = true;
		rec->ts = now();
	}
	Serial.printf("Record information:\n-duration: %02dh%02dm%02ds\n-path: '%s'\n-time set: %d\n-rec time: %ld\n", rec->dur.Hour, rec->dur.Minute, rec->dur.Second, rec->path.c_str(), rec->t_set, rec->ts);
	Alarm.timerOnce(dur, alarmRecDone);	
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
