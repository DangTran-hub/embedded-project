/**************************************************
* MQTT Topics Module - Project ESP32 Smart Lock
*
* Copyright 2026 Embedded Project Team
* All Rights Reserved
*
* The information contained herein is confidential
* property of Embedded Project Team. The use, copying,
* transfer or disclosure of such information is prohibited
* except by express written agreement with Embedded
* Project Team.
*
* 06/24/26 - Version 1.0 - ROM ID N/A
*       Initial release
**************************************************/
#undef VERSION
#define VERSION "Version 1.00"

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
