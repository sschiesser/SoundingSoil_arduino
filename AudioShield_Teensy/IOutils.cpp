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

struct leds_s leds[LED_MAX_NUMBER];
IntervalTimer led_timers[LED_MAX_NUMBER];
byte led_pins[LED_MAX_NUMBER] = { LED_RECORD_PIN, LED_MONITOR_PIN, LED_BLUETOOTH_PIN };
// unsigned long led_callbacks[LED_MAX_NUMBER] = 
	
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

	for(int i = 0; i < LED_MAX_NUMBER; i++) {
		leds[i].pin = led_pins[i];
		leds[i].status = LED_OFF;
		leds[i].mode = LED_MODE_OFF;
		leds[i].timer = led_timers[i];
		leds[i].cnt = 0;
		switch(i) {
			case LED_RECORD:
		leds[i].toggle = toggleRecLED;
			break;
			
			case LED_MONITOR:
			leds[i].toggle = toggleMonLED;
			break;
			
			case LED_BLUETOOTH:
			leds[i].toggle = toggleBtLED;
			break;
			
			default:
			break;
		}
		// Serial.print("Toggle address: 0x"); Serial.println((unsigned long)leds[i].toggle, HEX);
		pinMode(leds[i].pin, OUTPUT);
		digitalWrite(leds[i].pin, leds[i].status);
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
	switch(leds[LED_RECORD].mode) {
		case LED_MODE_OFF:
		Serial.println("LED off: stopping timer");
		leds[LED_RECORD].status = LED_OFF;
		stopLED(&leds[LED_RECORD]);
		break;
		
		case LED_MODE_ON:
		Serial.println("LED on: no toggle!");
		leds[LED_RECORD].status = LED_ON;
		break;
		
		case LED_MODE_WAITING:
		Serial.println("Waiting mode: regular medium toggle");
		leds[LED_RECORD].status = !leds[LED_RECORD].status;
		break;
		
		case LED_MODE_WARNING:
		Serial.print("Warning mode: "); Serial.print(leds[LED_RECORD].cnt); Serial.println(" toggles");
		if(leds[LED_RECORD].cnt++ > 12) {
			stopLED(&leds[LED_RECORD]);
		}
		else {
			leds[LED_RECORD].status = !leds[LED_RECORD].status;
		}
		break;
		
		case LED_MODE_ERROR:
		Serial.println("Error mode: regular fast toggle");
		leds[LED_RECORD].status = !leds[LED_RECORD].status;
		break;
		
		case LED_MODE_IDLE_FAST:
		Serial.println("Fast flashing mode: irregular flash toggle");
		break;
		
		case LED_MODE_IDLE_SLOW:
		Serial.println("Slow flashing mode: irregular flash toggle");
		if(leds[LED_RECORD].status == LED_ON) {
			leds[LED_RECORD].timer.update(LED_BLINK_FAST_MS);
		}
		else {
			leds[LED_RECORD].timer.update(LED_BLINK_SLOW_MS);
		}
		leds[LED_RECORD].status = !leds[LED_RECORD].status;
		break;
		
		default:
		break;
	}
	digitalWrite(leds[LED_RECORD].pin, leds[LED_RECORD].status);
}

void toggleMonLED(void) {
	Serial.print("toggleMON@ 0x"); Serial.println((unsigned long)toggleMonLED, HEX);
	switch(leds[LED_MONITOR].mode) {
		case LED_MODE_OFF:
		Serial.println("LED off: stopping timer");
		leds[LED_MONITOR].status = LED_OFF;
		break;

		case LED_MODE_ON:
		Serial.println("LED on: no toggle!");
		leds[LED_MONITOR].status = LED_ON;
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
	digitalWrite(leds[LED_MONITOR].pin, leds[LED_MONITOR].status);
}

void toggleBtLED(void) {
	Serial.print("toggleBT@ 0x"); Serial.println((unsigned long)toggleBtLED, HEX);
	switch(leds[LED_BLUETOOTH].mode) {

	}
}

void startLED(struct leds_s *ld, enum led_mode mode) {
	// Serial.print("startLED: LED#"); Serial.print(ld->pin);
	// Serial.print(", mode = "); Serial.println(mode);
	// First stop any remaining LED state
	stopLED(ld);
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
		// Serial.print("Timer started, returning at 0x"); Serial.println((unsigned long)ld->toggle, HEX);
		break;
		
		case LED_MODE_WARNING: // 3
		ld->timer.begin(ld->toggle, LED_BLINK_FAST_MS);
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

void stopLED(struct leds_s *ld) {
	// Serial.print("StopLED: LED#"); Serial.println(ld->pin);
	ld->timer.end();
	ld->mode = LED_MODE_OFF;
	ld->cnt = 0;
	ld->status = LED_OFF;
	digitalWrite(ld->pin, ld->status);
}