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
		// as_leds[i].blink = LED_BLINK_OFF;
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
		// Serial.print("Toggle address: 0x"); Serial.println((unsigned long)as_leds[i].toggle, HEX);
		pinMode(as_leds[i].pin, OUTPUT);
		digitalWrite(as_leds[i].pin, as_leds[i].status);
	}
	unsigned long a;
	a = (unsigned long)&toggleRecLED;
	Serial.print("toggleRecLED address: 0x"); Serial.println(a, HEX);
	a = (unsigned long)&toggleMonLED;
	Serial.print("toggleMonLED address: 0x"); Serial.println(a, HEX);
	a = (unsigned long)&toggleBtLED;
	Serial.print("toggleBtLED address: 0x"); Serial.println(a, HEX);
}


void toggleRecLED(void) {
	Serial.print("toggleREC @ 0x"); Serial.println((unsigned long)toggleRecLED, HEX);
	switch(as_leds[LED_RECORD].mode) {
		case LED_MODE_OFF:
		Serial.println("LED off: stopping timer");
		as_leds[LED_RECORD].status = LED_OFF;
		stopLED(&as_leds[LED_RECORD]);
		break;
		
		case LED_MODE_ON:
		Serial.println("LED on: no toggle!");
		as_leds[LED_RECORD].status = LED_ON;
		break;
		
		case LED_MODE_WAITING:
		Serial.println("Waiting mode: regular medium toggle");
		as_leds[LED_RECORD].status = !as_leds[LED_RECORD].status;
		break;
		
		case LED_MODE_WARNING:
		Serial.println("Warning mode: ... toggle");
		as_leds[LED_RECORD].status = !as_leds[LED_RECORD].status;
		break;
		
		case LED_MODE_ERROR:
		Serial.println("Error mode: regular fast toggle");
		as_leds[LED_RECORD].status = !as_leds[LED_RECORD].status;
		break;
		
		case LED_MODE_IDLE_FAST:
		Serial.println("Fast flashing mode: irregular flash toggle");
		break;
		
		case LED_MODE_IDLE_SLOW:
		Serial.println("Slow flashing mode: irregular flash toggle");
		if(as_leds[LED_RECORD].status == LED_ON) {
			as_leds[LED_RECORD].timer.update(LED_BLINK_FAST_MS);
		}
		else {
			as_leds[LED_RECORD].timer.update(LED_BLINK_SLOW_MS);
		}
		as_leds[LED_RECORD].status = !as_leds[LED_RECORD].status;
		break;
		
		default:
		break;
	}
	digitalWrite(as_leds[LED_RECORD].pin, as_leds[LED_RECORD].status);
}

void toggleMonLED(void) {
	Serial.print("toggleMON@ 0x"); Serial.println((unsigned long)toggleMonLED, HEX);
	switch(as_leds[LED_MONITOR].mode) {
		case LED_MODE_OFF:
		Serial.println("LED off: stopping timer");
		as_leds[LED_MONITOR].status = LED_OFF;
		break;

		case LED_MODE_ON:
		Serial.println("LED on: no toggle!");
		as_leds[LED_MONITOR].status = LED_ON;
		break;
		
		case LED_MODE_WARNING:
		break;
		
		case LED_MODE_WAITING:
		break;
		
		case LED_MODE_ERROR:
		break;
		
		case LED_MODE_IDLE_FAST:
		break;
		
		case LED_MODE_IDLE_SLOW:
		break;
		
		default:
		break;
	}
	digitalWrite(as_leds[LED_MONITOR].pin, as_leds[LED_MONITOR].status);
}

void toggleBtLED(void) {
	Serial.print("toggleBT@ 0x"); Serial.println((unsigned long)toggleBtLED, HEX);
	switch(as_leds[LED_BLUETOOTH].mode) {

	}
}

void startLED(struct leds *ld, enum led_mode mode) {
	Serial.print("startLED: LED#"); Serial.print(ld->pin);
	Serial.print(", mode = "); Serial.println(mode);
	ld->mode = mode;
	switch(mode) {
		case LED_MODE_OFF: // 0
		stopLED(ld);
		break;
		
		case LED_MODE_ON: // 1
		ld->status = LED_ON;
		break;
		
		case LED_MODE_WAITING: // 2
		ld->timer.begin(ld->toggle, LED_BLINK_MED_MS);
		Serial.print("Timer started, returning at 0x"); Serial.println((unsigned long)ld->toggle, HEX);
		break;
		
		case LED_MODE_WARNING: // 3
		break;
		
		case LED_MODE_ERROR: // 4
		break;
		
		case LED_MODE_IDLE_FAST: // 5
		break;
		
		case LED_MODE_IDLE_SLOW: // 6
		break;
		
		default:
		break;
	}
	digitalWrite(ld->pin, ld->status);
}

void stopLED(struct leds *ld) {
	Serial.print("StopLED: LED#"); Serial.println(ld->pin);
	ld->timer.end();
	ld->mode = LED_MODE_OFF;
	ld->status = LED_OFF;
	digitalWrite(ld->pin, ld->status);
}