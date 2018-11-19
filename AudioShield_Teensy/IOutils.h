/*
 * IOutils.h
 */
 
#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include <Arduino.h>
#include <Bounce.h>
// #include <timer.h>

// Buttons definition
#define BUTTON_RECORD         24
#define BUTTON_MONITOR        25
#define BUTTON_BLUETOOTH      29
extern Bounce buttonRecord;
extern Bounce buttonMonitor;
extern Bounce buttonBluetooth;

// LED definition
#define LED_RECORD            26
#define LED_MONITOR           27
#define LED_BLUETOOTH         28
#define LED_OFF               HIGH
#define LED_ON                LOW

#define LED_BLINK_FAST				(100 * 1000)
#define LED_BLINK_MED					(500 * 1000)
#define LED_BLINK_SLOW				(5000 * 1000)

enum led_mode {
	LED_MODE_BLINK_SLOW,
	LED_MODE_BLINK_MED,
	LED_MODE_BLINK_FAST,
	LED_MODE_IDLE,
	LED_MODE_ON,
	LED_MODE_OFF
};

struct led_state {
	bool on;
	enum led_mode mode;
	unsigned int cnt;
};

extern IntervalTimer recLedTimer;

// auto recLedTimer = timer_create_default();
// auto monLedTimer = timer_create_default();
// auto btLedTimer = timer_create_default();


void initLEDButtons(void);
void toggleRecLED(void);

#endif