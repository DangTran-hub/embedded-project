#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

typedef struct {
    uint8_t percent;
    float voltage;
} BatteryStatus;

void batteryMonitorBegin(uint8_t analogPin, float dividerRatio);
BatteryStatus batteryMonitorRead(void);
uint8_t batteryMonitorReadPercent(void);

#endif
