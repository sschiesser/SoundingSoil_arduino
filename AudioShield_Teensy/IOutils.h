/*
 * IOutils.h
 */
 
#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include <Arduino.h>
#include <Bounce.h>

// Buttons definition
#define BUTTON_RECORD         24
#define BUTTON_MONITOR        25
#define BUTTON_BLUETOOTH      29
extern Bounce buttonRecord;
extern Bounce buttonMonitor;
extern Bounce buttonBluetooth;

// LED definition
#define LED_RECORD_PIN        26
#define LED_MONITOR_PIN       27
#define LED_BLUETOOTH_PIN     28
#define LED_MAX_NUMBER				3

#define LED_OFF               HIGH
#define LED_ON                LOW

#define LED_BLINK_FAST_MS			(100 * 1000)
#define LED_BLINK_MED_MS			(500 * 1000)
#define LED_BLINK_SLOW_MS			(5000 * 1000)

enum led_enum {
	LED_RECORD = 0,
	LED_MONITOR,
	LED_BLUETOOTH,
	LED_MAX_SIZE
};

enum led_blink {
	LED_BLINK_OFF,
	LED_BLINK_SLOW,
	LED_BLINK_MED,
	LED_BLINK_FAST,
	LED_BLINK_FLASH
};

enum led_mode {
	LED_MODE_OFF,
	LED_MODE_SOLID,
	LED_MODE_WAITING,
	LED_MODE_WARNING,
	LED_MODE_ERROR,
	LED_MODE_IDLE
};

struct leds {
	unsigned int pin; 
	bool status;
	enum led_mode mode;
	enum led_blink blink;
	IntervalTimer timer;
	unsigned int cnt;
};

// extern struct led_state as_led_state[LED_MAX_NUMBER];
// extern IntervalTimer as_led_timer[LED_MAX_NUMBER];


void initLEDButtons(void);
// void toggleRecLED(void);

#endif