/*
 * IOutils.h
 */
#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include "main.h"

// GPS switch pin#
#define GPS_SWITCH_PIN				3
// Volume controle pin#
#define AUDIO_VOLUME_PIN			A1
// Buttons pin#										//v1.0 vals		v1.3 vals		v2.0 vals
#define BUTTON_RECORD_PIN			2		//2						2						2
#define BUTTON_MONITOR_PIN		16	//4						16					16
#define BUTTON_BLUETOOTH_PIN	21	//21					21					21
// Buttons debounce time
#define BUTTON_BOUNCE_TIME_MS	20
// LEDs pin#
#define LED_RECORD_PIN				26	//26					26					26
#define LED_MONITOR_PIN				29	//27					27					29
#define LED_BLUETOOTH_PIN			25	//34					28					25
#define LED_PEAK_PIN					34	//28					34					34
// LEDs#
#define LED_MAX_NUMBER				4
// LED ON/OFF states
#define LED_OFF               HIGH
#define LED_ON                LOW
// LEDs blinking values
#define LED_BLINK_FAST_MS			(100 * 1000)
#define LED_BLINK_MED_MS			(500 * 1000)
#define LED_BLINK_SLOW_MS			(5000 * 1000)
// Buttons list
enum bList {
	BUTTON_RECORD = 0,
	BUTTON_MONITOR,
	BUTTON_BLUETOOTH
};
// Button calls
enum bCalls {
	BCALL_NONE = 0,
	BCALL_REC = BUTTON_RECORD_PIN,
	BCALL_MON = BUTTON_MONITOR_PIN,
	BCALL_BLUE = BUTTON_BLUETOOTH_PIN
};
// LED list
enum lList {
	LED_RECORD = 0,
	LED_MONITOR,
	LED_BLUETOOTH,
	LED_PEAK
};
// LEDs modes enumeration
enum lMode {
	LED_MODE_OFF = 0,						// 0
	LED_MODE_ON,								// 1
	LED_MODE_WAITING,						// 2
	LED_MODE_WARNING_SHORT,			// 3
	LED_MODE_WARNING_LONG,			// 4
	LED_MODE_ERROR,							// 5
	LED_MODE_IDLE_FAST,					// 6
	LED_MODE_IDLE_SLOW					// 7
};

// LED state & status struct
typedef void 									(*addToggle)(void);
struct leds_s {
	unsigned int pin; 
	bool status;
	enum lMode mode;
	IntervalTimer timer;
	addToggle toggle;
	unsigned int cnt;
};
extern struct leds_s 					leds[LED_MAX_NUMBER];

extern enum bCalls						button_call;
extern Bounce									but_rec;
extern Bounce									but_mon;
extern Bounce									but_blue;


/* ======================
 * FUNCTIONS DECLARATIONS
 * ====================== */
void toggleCb(struct leds_s *ld);
void toggleRecLED(void);
void toggleMonLED(void);
void toggleBtLED(void);
void togglePeakLED(void);
void initLEDButtons(void);
void startLED(struct leds_s *ld, enum lMode mode);
void stopLED(struct leds_s *ld);

#endif /* _IOUTILS_H_ */