#include "light_controller.h"
#include "config.h"
#include "mqtt_manager.h"
#include <ArduinoJson.h>

LightMode currentMode = MANUAL;
bool lightState = false;
unsigned long lastStatusPublish = 0;
unsigned long lastCommandTime = 0;

// Replace with time-keeping implementation if using NTP later
String getTimestamp() {
  return String(millis() / 1000) + "s";
}

void setLight(bool state) {
  lightState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH); // Status LED
}

void handleCommandMessage(String payload) {
  DynamicJsonDocument doc(256);
  deserializeJson(doc, payload);

  String cmd = doc["cmd"];
  String mode = doc["mode"];

  if (cmd == "on") setLight(true);
  else if (cmd == "off") setLight(false);

  if (mode == "manual") currentMode = MANUAL;
  else if (mode == "auto") currentMode = AUTO;

  lastCommandTime = millis();

  // Acknowledge
  DynamicJsonDocument ack(256);
  ack["cmd_received"] = cmd;
  ack["new_mode"] = (currentMode == AUTO ? "auto" : "manual");
  ack["timestamp"] = getTimestamp();
  ack["source"] = "mqtt";

  publishMessage(MQTT_TOPIC_ACK, ack);
  publishStatus();
}

void handleScheduleMessage(String payload) {
  // Placeholder for handling schedule config via JSON
}

void handleScheduledLightControl() {
  if (currentMode == AUTO) {
    // Check if it's time to turn on/off the light (you can expand this)
  }

  if (millis() - lastStatusPublish > 600000) {  // Every 10 minutes
    publishStatus();
    lastStatusPublish = millis();
  }
}

void publishStatus() {
  DynamicJsonDocument status(256);
  status["light"] = lightState ? "on" : "off";
  status["mode"] = (currentMode == AUTO ? "auto" : "manual");
  status["last_cmd"] = lastCommandTime;
  status["wifi_strength"] = WiFi.RSSI();
  status["timestamp"] = getTimestamp();
  publishMessage(MQTT_TOPIC_STATUS, status);
}
