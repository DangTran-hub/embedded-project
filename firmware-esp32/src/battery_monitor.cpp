#include "battery_monitor.h"

static uint8_t batteryPin = 35;
static float voltageDividerRatio = 4.7f;

void batteryMonitorBegin(uint8_t analogPin, float dividerRatio)
{
    batteryPin = analogPin;
    voltageDividerRatio = dividerRatio;
}

BatteryStatus batteryMonitorRead(void)
{
    long sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += analogRead(batteryPin);
    }

    float voltage = ((sum / 10.0f) / 4095.0f) * 3.3f * voltageDividerRatio;
    float percent = (voltage - 9.0f) / (12.6f - 9.0f) * 100.0f;

    BatteryStatus status;
    status.voltage = voltage;
    status.percent = (uint8_t)constrain(percent, 0, 100);
    return status;
}

uint8_t batteryMonitorReadPercent(void)
{
    return batteryMonitorRead().percent;
}
