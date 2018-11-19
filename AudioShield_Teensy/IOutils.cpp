/*
 * IO utils
 * 
 * Miscellaneous IO utilities...
 * 
 */

#include "IOutils.h"

Bounce buttonRecord = Bounce(BUTTON_RECORD, 8);
Bounce buttonMonitor = Bounce(BUTTON_MONITOR, 8);
Bounce buttonBluetooth = Bounce(BUTTON_BLUETOOTH, 8);

// auto recLedTimer = timer_create_default();
// auto monLedTimer = timer_create_default();
// auto btLedTimer = timer_create_default();

/* initLEDButtons(void)
 * --------------------
 * Initialize pin direction & mode for LED and buttons
 * IN:	- none
 * OUT:	- none
 */
void initLEDButtons(void) {
	// Configure the pushbutton pins
	pinMode(BUTTON_RECORD, INPUT_PULLUP);
	pinMode(BUTTON_MONITOR, INPUT_PULLUP);
	pinMode(BUTTON_BLUETOOTH, INPUT_PULLUP);

	// Configure the output pins
	pinMode(LED_RECORD, OUTPUT);
	digitalWrite(LED_RECORD, LED_OFF);
	pinMode(LED_MONITOR, OUTPUT);
	digitalWrite(LED_MONITOR, LED_OFF);
	pinMode(LED_BLUETOOTH, OUTPUT);
	digitalWrite(LED_BLUETOOTH, LED_OFF);
}

bool toggleRecLED(void *next) {
	int cur_int = (int)next;
	int next_int = 0;
	bool ret_val = true;
	Serial.print("Toggle. Current = "); Serial.println(cur_int);
	switch(cur_int) {
		case 0:
		ret_val = false;
		break;
		
		case 100:
		next_int = 500;
		break;
		
		case 500:
		next_int = 1000;
		break;
		
		case 1000:
		next_int = 100;
		break;
		
		default:
		break;
	}
	recLedTimer.in(next_int, toggleRecLED, (void*)next_int);
	digitalWrite(LED_RECORD, !digitalRead(LED_RECORD));
	return true;
}

void blinkLED(int LEDnr, int duration) {
	switch(LEDnr) {
		case LED_RECORD:
		break;
		
		case LED_MONITOR:
		break;
		
		case LED_BLUETOOTH:
		break;
		
		default:
		break;
	}
}