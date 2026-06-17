/**
 * @file mqtt_topics.cpp
 * @brief MQTT topic builder for the ESP32 smart lock firmware.
 *
 * @details
 * This module centralizes the MQTT topic naming scheme used by the device and
 * application server. All topics are generated from the lock identifier under
 * the "smartlock/<lock-id>" namespace. Status messages are published to the
 * "/status" topic, while remote control and administration commands are
 * received from the "/cmd" topic.
 */
#include "mqtt_topics.h"

static String mqttBaseTopic(const char *lockId)
{
    return "smartlock/" + String(lockId);
}

String mqttStatusTopic(const char *lockId)
{
    return mqttBaseTopic(lockId) + "/status";
}

String mqttCommandTopic(const char *lockId)
{
    return mqttBaseTopic(lockId) + "/cmd";
}
