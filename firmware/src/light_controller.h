#ifndef LIGHT_CONTROLLER_H
#define LIGHT_CONTROLLER_H

#include <Arduino.h>

enum LightMode { MANUAL, AUTO, NONE };

extern LightMode currentMode;
extern LightMode prevMode;
extern bool lightState;

void setupMQTT();
void handleCommandMessage(String payload);
void handleScheduleMessage(String payload);
void handleScheduledLightControl();
void publishStatus();

#endif
