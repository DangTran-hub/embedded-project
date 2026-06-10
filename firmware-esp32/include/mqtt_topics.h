#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

#include <Arduino.h>

String mqttStatusTopic(const char *lockId);
String mqttCommandTopic(const char *lockId);

#endif
