#ifndef SMART_LOCK_H
#define SMART_LOCK_H

#include <Arduino.h>

#define MQTT_SERVER "d60daf22.ala.asia-southeast1.emqxsl.com"
#define MQTT_PORT 8883
#define LOCK_ID "e16147df-f1a2-40bb-b813-750bd79e6b2e"

#define SS_PIN 5
#define RST_PIN 32
#define BUZZER_PIN 4
#define BUTTON_PIN 34
#define SOLENOID_PIN 17
#define BATTERY_PIN 35

#define ROW_NUM 4
#define COL_NUM 3

#define LCD_TIMEOUT_MS 20000UL
#define INPUT_TIMEOUT_MS 10000UL
#define LOCK_TIMEOUT_MS 30000UL
#define NETWORK_CHECK_MS 10000UL

typedef enum {
    SYS_IDLE,
    SYS_WAIT_PASSWORD,
    SYS_LOCKED,
    SYS_CHANGE_OLD,
    SYS_CHANGE_NEW,
    SYS_CHANGE_CONFIRM
} SystemState;

void smartLockSetup(void);
void smartLockLoop(void);

#endif
