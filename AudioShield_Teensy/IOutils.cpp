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
enum led_enum led_cnt;
	
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
			break;
			
			case LED_MONITOR:
			as_leds[i].pin = LED_MONITOR_PIN;
			break;
			
			case LED_BLUETOOTH:
			as_leds[i].pin = LED_BLUETOOTH_PIN;
			break;
			
			default:
			break;
		}
		pinMode(as_leds[i].pin, OUTPUT);
		digitalWrite(as_leds[i].pin, as_leds[i].status);
	}
}


// void toggleRecLED(void) {
	// switch(rec_led_state.blink) {
		// case LED_BLINK_FAST:
		// case LED_BLINK_MED:
		// case LED_BLINK_SLOW:
		// Serial.print("LED state: "); Serial.println(rec_led_state.status);
		// break;
				
		// case LED_BLINK_FLASH:
		// if(rec_led_state.status == LED_ON) {
			// rec_led_timer.update(LED_BLINK_FAST_MS);
		// }
		// else {
			// rec_led_timer.update(LED_BLINK_SLOW_MS);
		// }
		// break;
		
		// default:
		// break;
	// }
	// rec_led_state.status = !rec_led_state.status;
	// digitalWrite(LED_RECORD_PIN, rec_led_state.status);
// }

// void startLED(enum leds ld, enum led_mode) {
	// switch(led_mode) {
		// case LED_MODE_OFF:
		// break;
		
		// case LED_MODE_SOLID:
		// break;
		
		// case LED_MODE_WAITING:
		// break;
		
		// case LED_MODE_WARNING:
		// break;
		
		// case LED_MODE_ERROR:
		// break;
		
		// case LED_MODE_IDLE:
		// break;
		
		// default:
		// break;
	// }
// }