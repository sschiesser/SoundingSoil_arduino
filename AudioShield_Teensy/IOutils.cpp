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

struct leds as_leds[LED_MAX_SIZE];
IntervalTimer as_led_timers[LED_MAX_SIZE];
	
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

	for(int i = 0; i < LED_MAX_SIZE; i++) {
		as_leds[i].status = LED_OFF;
		as_leds[i].mode = LED_MODE_OFF;
		as_leds[i].blink = LED_BLINK_OFF;
		as_leds[i].timer = as_led_timers[i];
		as_leds[i].cnt = 0;
		switch(i) {
			case LED_RECORD:
			as_leds[i].pin = LED_RECORD_PIN;
			as_leds[i].toggle = toggleRecLED;
			break;
			
			case LED_MONITOR:
			as_leds[i].pin = LED_MONITOR_PIN;
			as_leds[i].toggle = toggleMonLED;
			break;
			
			case LED_BLUETOOTH:
			as_leds[i].pin = LED_BLUETOOTH_PIN;
			as_leds[i].toggle = toggleBtLED;
			break;
			
			default:
			break;
		}
		pinMode(as_leds[i].pin, OUTPUT);
		digitalWrite(as_leds[i].pin, as_leds[i].status);
	}
}


void toggleRecLED(void) {
	switch(as_leds[LED_RECORD].blink) {
		case LED_BLINK_FAST:
		case LED_BLINK_MED:
		case LED_BLINK_SLOW:
		Serial.print("LED status: "); Serial.println(as_leds[LED_RECORD].status);
		break;
				
		case LED_BLINK_FLASH:
		if(as_leds[LED_RECORD].status == LED_ON) {
			as_leds[LED_RECORD].timer.update(LED_BLINK_FAST_MS);
		}
		else {
			as_leds[LED_RECORD].timer.update(LED_BLINK_SLOW_MS);
		}
		break;
		
		default:
		break;
	}
	as_leds[LED_RECORD].status = !as_leds[LED_RECORD].status;
	digitalWrite(as_leds[LED_RECORD].pin, as_leds[LED_RECORD].status);
}

void toggleMonLED(void) {
}

void toggleBtLED(void) {
}


void startLED(struct leds ld, enum led_mode mode) {
	ld.mode = mode;
	switch(mode) {
		case LED_MODE_OFF:
		ld.blink = LED_BLINK_OFF;
		ld.status = LED_OFF;
		ld.timer.end();
		break;
		
		case LED_MODE_SOLID:
		ld.blink = LED_BLINK_OFF;
		ld.status = LED_ON;
		ld.timer.end();
		break;
		
		case LED_MODE_WAITING:
		ld.blink = LED_BLINK_MED;
		ld.status = LED_ON;
		break;
		
		case LED_MODE_WARNING:
		break;
		
		case LED_MODE_ERROR:
		break;
		
		case LED_MODE_IDLE:
		break;
		
		default:
		break;
	}
}