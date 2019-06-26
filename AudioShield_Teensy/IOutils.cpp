/*
 * IO utils
 *
 * Miscellaneous IO utilities...
 *
 */
#include "IOutils.h"

Bounce												but_rec = Bounce(BUTTON_RECORD_PIN, BUTTON_BOUNCE_TIME_MS);
Bounce 												but_mon = Bounce(BUTTON_MONITOR_PIN, BUTTON_BOUNCE_TIME_MS);
Bounce 												but_blue = Bounce(BUTTON_BLUETOOTH_PIN, BUTTON_BOUNCE_TIME_MS);

enum bCalls											button_call;

IntervalTimer										led_timers[LED_MAX_NUMBER];
byte 												led_pins[LED_MAX_NUMBER] = { LED_RECORD_PIN, LED_MONITOR_PIN, LED_BLUETOOTH_PIN, LED_PEAK_PIN };
struct leds_s 										leds[LED_MAX_NUMBER];

/* initLEDButtons(void)
 * --------------------
 * Assign and initialize all the LED-related elements
 * (pin#, status, mode, cnt, timer, callback_fn)
 * ...and initialize the button inputs
 * IN:	- none
 * OUT:	- none
 */
void initLEDButtons(void) {
	// Configure the pushbutton pins
	pinMode(BUTTON_RECORD_PIN, INPUT_PULLUP);
	pinMode(BUTTON_MONITOR_PIN, INPUT_PULLUP);
	pinMode(BUTTON_BLUETOOTH_PIN, INPUT_PULLUP);

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
			case LED_PEAK:
				leds[i].toggle = togglePeakLED;
				break;
			default:
				break;
		}
		pinMode(leds[i].pin, OUTPUT);
		digitalWrite(leds[i].pin, leds[i].status);
	}
	// unsigned long a;
	// a = (unsigned long)&toggleRecLED;
	// MONPORT.print("Info:    toggleRecLED address: 0x"); MONPORT.println(a, HEX);
	// a = (unsigned long)&toggleMonLED;
	// MONPORT.print("Info:    toggleMonLED address: 0x"); MONPORT.println(a, HEX);
	// a = (unsigned long)&toggleBtLED;
	// MONPORT.print("Info:    toggleBtLED address: 0x"); MONPORT.println(a, HEX);
}

/* toggleCb(struct leds_s *)
 * -------------------------
 * LED timer callback function! Parse the
 * callbacks along LED-mode and process next action.
 * IN:	- pointer to LED (struct leds_s *)
 * OUT:	- none
 */
void toggleCb(struct leds_s *ld) {
	// MONPORT.print("Info:    toggleCb called with led#"); MONPORT.println(ld->pin);
	// MONPORT.print("Info:    address: 0x"); MONPORT.println((unsigned long)toggleCb, HEX);
	switch(ld->mode) {
		case LED_MODE_OFF:
		// MONPORT.println("Info:    LED off: stopping timer");
		ld->status = LED_OFF;
		stopLED(ld);
		break;

		case LED_MODE_ON:
		// MONPORT.println("Info:    LED on: no toggle!");
		ld->status = LED_ON;
		break;

		case LED_MODE_WAITING:
		// MONPORT.println("Info:    Waiting mode: regular medium toggle");
		ld->status = !ld->status;
		break;

		case LED_MODE_WARNING_SHORT:
		if(ld->cnt++ > 2) {
			stopLED(ld);
		}
		else {
			ld->status = !ld->status;
		}
		break;

		case LED_MODE_WARNING_LONG:
		// MONPORT.print("Info:    Warning mode: "); MONPORT.print(ld->cnt); MONPORT.println(" toggles");
		if(ld->cnt++ > 12) {
			// stopLED(ld);
			startLED(ld, LED_MODE_ON);
		}
		else {
			ld->status = !ld->status;
		}
		break;

		case LED_MODE_ERROR:
		// MONPORT.println("Info:    Error mode: regular fast toggle");
		ld->status = !ld->status;
		break;

		case LED_MODE_IDLE_FAST:
		// MONPORT.println("Info:    Fast flashing mode: irregular flash toggle");
		if(ld->status == LED_ON) {
			ld->timer.update(LED_BLINK_FAST_MS);
		}
		else {
			ld->timer.update(LED_BLINK_MED_MS);
		}
		ld->status = !ld->status;
		break;

		case LED_MODE_IDLE_SLOW:
		// MONPORT.println("Info:    Slow flashing mode: irregular flash toggle");
		if(ld->status == LED_ON) {
			ld->timer.update(LED_BLINK_FAST_MS);
		}
		else {
			ld->timer.update(LED_BLINK_SLOW_MS);
		}
		ld->status = !ld->status;
		break;

		default:
		break;
	}
	digitalWrite(ld->pin, ld->status);
}

/* toggleXxxLED(void)
 * ------------------
 * Redirect the callback function of each LED to
 * a generic one with assignment by reference
 * IN:	- none
 * OUT:	- none
 */
void toggleRecLED(void) {
	toggleCb(&leds[LED_RECORD]);
}
void toggleMonLED(void) {
	toggleCb(&leds[LED_MONITOR]);
}
void toggleBtLED(void) {
	toggleCb(&leds[LED_BLUETOOTH]);
}
void togglePeakLED(void) {
	toggleCb(&leds[LED_PEAK]);
}

/* startLED(struct leds_s *, enum lMode)
 * ----------------------------------------
 * Start a LED switch action (bloinking or solid):
 * set the wanted LED-mode and start the corresponding timer.
 * IN:	- pointer to LED (struct leds_s *)
 *			- LED-mode (enum lMode)
 * OUT:	- none
 */
void startLED(struct leds_s *ld, enum lMode mode) {
	// MONPORT.print("Info:    startLED: LED#"); MONPORT.print(ld->pin);
	// MONPORT.print(", mode = "); MONPORT.println(mode);
	// First stop any remaining LED state
	stopLED(ld);
	ld->mode = mode;
	switch(mode) {
		case LED_MODE_OFF:
		stopLED(ld);
		break;

		case LED_MODE_ON:
		ld->status = LED_ON;
		break;

		case LED_MODE_WAITING:
		ld->timer.begin(ld->toggle, LED_BLINK_MED_MS);
		// MONPORT.print("Info:    Timer started, returning at 0x"); MONPORT.println((unsigned long)ld->toggle, HEX);
		break;

		case LED_MODE_WARNING_SHORT:
		ld->timer.begin(ld->toggle, LED_BLINK_FAST_MS);
		break;

		case LED_MODE_WARNING_LONG:
		ld->timer.begin(ld->toggle, LED_BLINK_FAST_MS);
		break;

		case LED_MODE_ERROR:
		break;

		case LED_MODE_IDLE_FAST:
		ld->timer.begin(ld->toggle, LED_BLINK_FAST_MS);
		break;

		case LED_MODE_IDLE_SLOW:
		ld->timer.begin(ld->toggle, LED_BLINK_FAST_MS);
		break;

		default:
		break;
	}
	digitalWrite(ld->pin, ld->status);
}

/* stopLED(struct leds_s *)
 * ------------------------
 * Stop a specific LED and reset all corresponding parameters.
 * IN:	- pointer to LED (struct leds_s *)
 * OUT:	- none
 */
void stopLED(struct leds_s *ld) {
	// MONPORT.print("Info:    StopLED: LED#"); MONPORT.println(ld->pin);
	ld->timer.end();
	ld->mode = LED_MODE_OFF;
	ld->cnt = 0;
	ld->status = LED_OFF;
	digitalWrite(ld->pin, ld->status);
}
