/**
 * @file main.cpp
 * @brief Arduino entry point for the ESP32 smart lock application.
 *
 * @details
 * This file keeps the Arduino runtime interface intentionally small. During
 * startup, setup() delegates all hardware, network, storage, and user-interface
 * initialization to smartLockSetup(). During normal operation, loop() repeatedly
 * calls smartLockLoop(), which runs the lock state machine, keypad and RFID
 * scanning, MQTT communication, button handling, battery reporting, and LCD
 * timeout management.
 */
#include "smart_lock.h"

void setup(void)
{
    smartLockSetup();
}

void loop(void)
{
    smartLockLoop();
}
