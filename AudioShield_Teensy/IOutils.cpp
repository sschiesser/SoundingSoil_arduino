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


volatile struct led_state rec_led_state;
volatile struct led_state mon_led_state;
volatile struct led_state bt_led_state;
	
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
	rec_led_state.on = LED_OFF;
	rec_led_state.mode = LED_MODE_IDLE;
	rec_led_state.cnt = 0;
	pinMode(LED_MONITOR, OUTPUT);
	digitalWrite(LED_MONITOR, LED_OFF);
	pinMode(LED_BLUETOOTH, OUTPUT);
	digitalWrite(LED_BLUETOOTH, LED_OFF);
}

// bool toggleRecLED(void *next) {
	// int cur_int = (int)next;
	// int next_int = 0;
	// bool ret_val = true;
	// Serial.print("Toggle. Current = "); Serial.println(cur_int);
	// switch(cur_int) {
		// case 0:
		// ret_val = false;
		// break;
		
		// case 100:
		// next_int = 500;
		// break;
		
		// case 500:
		// next_int = 1000;
		// break;
		
		// case 1000:
		// next_int = 100;
		// break;
		
		// default:
		// break;
	// }
	// recLedTimer.in(next_int, toggleRecLED, (void*)next_int);
	// digitalWrite(LED_RECORD, !digitalRead(LED_RECORD));
	// return true;
// }

void toggleRecLED(void) {
	switch(rec_led_state.mode) {
		case LED_MODE_BLINK_FAST:
		case LED_MODE_BLINK_MED:
		case LED_MODE_BLINK_SLOW:
		Serial.print("LED state: "); Serial.println(rec_led_state.on);
		break;
				
		case LED_MODE_IDLE:
		if(rec_led_state.on == LED_ON) {
			recLedTimer.update(LED_BLINK_FAST);
		}
		else {
			recLedTimer.update(LED_BLINK_SLOW);
		}
		break;
		
		default:
		break;
	}
	rec_led_state.on = !rec_led_state.on;
	digitalWrite(LED_RECORD, rec_led_state.on);
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