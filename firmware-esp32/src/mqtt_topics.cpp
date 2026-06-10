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
