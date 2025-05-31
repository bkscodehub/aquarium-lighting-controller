#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>
#include <EEPROM.h>

enum LightMode
{
    MANUAL,
    AUTO,
    NONE
};

struct PersistentLightScheduleState
{
    int scheduledOnHour;
    int scheduledOnMinute;
    int scheduledOffHour;
    int scheduledOffMinute;
    char scheduleDefinedOn[32];
};

extern LightMode currentMode;
extern LightMode prevMode;
extern bool lightState;

void initLightController();
void setupMQTT();
void handleCommandMessage(String payload);
void handleScheduleMessage(String payload);
void handleScheduledLightControl();
void publishStatus();

#endif
