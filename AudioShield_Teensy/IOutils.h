/*
 * IOutils.h
 */
 
#ifndef _IOUTILS_H_
#define _IOUTILS_H_

#include <Arduino.h>
#include <Bounce.h>
#include <timer.h>

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

auto recLedTimer = timer_create_default();
auto monLedTimer = timer_create_default();
auto btLedTimer = timer_create_default();


void initLEDButtons(void);
bool toggleRecLED(void *);

#endif