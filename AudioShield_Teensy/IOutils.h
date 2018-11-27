/*
 * IOutils.h
 */
 
#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include <Arduino.h>
#include <Bounce.h>

// Switches definitions
#define GPS_SWITCH_PIN				3

// Volume controle
#define AUDIO_VOLUME_PIN			A1
extern int										vol_ctrl;

// Buttons definitions
#define BUTTON_RECORD_PIN			2 //24
#define BUTTON_MONITOR_PIN		4 //25
#define BUTTON_BLUETOOTH_PIN	21 //29
#define BUTTON_BOUNCE_TIME_MS	20
extern Bounce									but_rec;
extern Bounce									but_mon;
extern Bounce									but_blue;

enum button_list {
	BUTTON_RECORD,
	BUTTON_MONITOR,
	BUTTON_BLUETOOTH
};

// LED definitions
#define LED_RECORD_PIN				26
#define LED_MONITOR_PIN				27
#define LED_BLUETOOTH_PIN			28
#define LED_MAX_NUMBER				3

#define LED_OFF               HIGH
#define LED_ON                LOW

#define LED_BLINK_FAST_MS			(100 * 1000)
#define LED_BLINK_MED_MS			(500 * 1000)
#define LED_BLINK_SLOW_MS			(5000 * 1000)

enum led_list {
	LED_RECORD = 0,
	LED_MONITOR,
	LED_BLUETOOTH,
};

enum led_mode {
	LED_MODE_OFF = 0,
	LED_MODE_ON,
	LED_MODE_WAITING,
	LED_MODE_WARNING,
	LED_MODE_ERROR,
	LED_MODE_IDLE_FAST,
	LED_MODE_IDLE_SLOW
};

// LED struct containing all the related elements
typedef void 									(*addToggle)(void);
struct leds_s {
	unsigned int pin; 
	bool status;
	enum led_mode mode;
	IntervalTimer timer;
	addToggle toggle;
	unsigned int cnt;
};
extern struct leds_s 					leds[LED_MAX_NUMBER];


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */

void toggleCb(struct leds_s *ld);
void toggleRecLED(void);
void toggleMonLED(void);
void toggleBtLED(void);
void initLEDButtons(void);
void startLED(struct leds_s *ld, enum led_mode mode);
void stopLED(struct leds_s *ld);

#endif /* _IOUTILS_H_ */