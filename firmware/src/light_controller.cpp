#include <ArduinoJson.h>
#include "light_controller.h"
#include "config.h"
#include "mqtt_manager.h"
#include <ESP8266WiFi.h>

#ifndef MQTT_BROKER
#define MQTT_BROKER "default_broker_url"
#endif

#ifndef MQTT_USERNAME
#define MQTT_USERNAME "default_mqtt_username"
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD "default_password"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 8883
#endif

LightMode currentMode = MANUAL;
bool lightState = false;
unsigned long lastStatusPublish = 0;
unsigned long lastCommandTime = 0;

int scheduledOnHour = -1, scheduledOnMinute = -1;
int scheduledOffHour = -1, scheduledOffMinute = -1;

// Replace with time-keeping implementation if using NTP later
String getTimestamp() {
  return String(millis() / 1000) + "s";
}

void setLight(bool state) {
  lightState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
  digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH); // Status LED
}

void handleCommandMessage(const String& topic, const String& payload) {
  if (topic == MQTT_TOPIC_CMD) {
    ArduinoJson::DynamicJsonDocument doc(256);
    deserializeJson(doc, payload);
  
    String cmd = doc["cmd"];
    String mode = doc["mode"];
  
    if (cmd == "on") setLight(true);
    else if (cmd == "off") setLight(false);
  
    if (mode == "manual") currentMode = MANUAL;
    else if (mode == "auto") currentMode = AUTO;
  
    lastCommandTime = millis();
  
    // Acknowledge
    ArduinoJson::DynamicJsonDocument ack(256);
    ack["cmd_received"] = cmd;
    ack["new_mode"] = (currentMode == AUTO ? "auto" : "manual");
    ack["timestamp"] = getTimestamp();
    ack["source"] = "mqtt";
  
    publishMessage(MQTT_TOPIC_ACK, ack);
    publishStatus();
  } else {
    Serial.print("Topic mismatch. Expected ");
    Serial.print(MQTT_TOPIC_CMD);
    Serial.print(" but received ");
    Serial.println(topic);
  }
}

void handleScheduleMessage(const String& topic, const String& payload) {
  if (topic == MQTT_TOPIC_SCHEDULE) {
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("Failed to parse schedule message: ");
      Serial.println(error.c_str());
      return;
    }

    String onTime = doc["on"];
    String offTime = doc["off"];

    if (sscanf(onTime.c_str(), "%d:%d", &scheduledOnHour, &scheduledOnMinute) != 2 ||
        sscanf(offTime.c_str(), "%d:%d", &scheduledOffHour, &scheduledOffMinute) != 2) {
      Serial.println("Invalid time format in schedule message. Expected HH:MM.");
      return;
    }

    Serial.print("Schedule set - ON at ");
    Serial.print(scheduledOnHour);
    Serial.print(":");
    Serial.print(scheduledOnMinute);
    Serial.print(", OFF at ");
    Serial.print(scheduledOffHour);
    Serial.print(":");
    Serial.println(scheduledOffMinute);
  } else {
    Serial.print("Topic mismatch. Expected ");
    Serial.print(MQTT_TOPIC_SCHEDULE);
    Serial.print(" but received ");
    Serial.println(topic);
  }
}

void handleScheduledLightControl() {
  if (currentMode == AUTO) {
    // Simulated current time from millis()
    unsigned long msSinceBoot = millis();
    unsigned long seconds = msSinceBoot / 1000;
    int currentHour = (seconds / 3600) % 24;
    int currentMinute = (seconds / 60) % 60;

    bool shouldTurnOn = (currentHour > scheduledOnHour || 
                         (currentHour == scheduledOnHour && currentMinute >= scheduledOnMinute)) &&
                        (currentHour < scheduledOffHour || 
                         (currentHour == scheduledOffHour && currentMinute < scheduledOffMinute));

    setLight(shouldTurnOn);
  }

  if (millis() - lastStatusPublish > 600000) {  // Every 10 minutes
//    publishStatus();
    lastStatusPublish = millis();
  }
}

void setupMQTT() {
  Serial.begin(115200);

  // Define topic-callback mappings
  static MqttCallbackEntry callbacks[] = {
    { "/aquarium/cmd", handleCommandMessage },
    { "/aquarium/schedule", handleScheduleMessage }
  };

  // Initialize MQTT
  initMQTT(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD, MQTT_PORT, callbacks, sizeof(callbacks) / sizeof(callbacks[0]));
}

void publishStatus() {
  ArduinoJson::DynamicJsonDocument status(256);
  status["light"] = lightState ? "on" : "off";
  status["mode"] = (currentMode == AUTO ? "auto" : "manual");
  status["last_cmd"] = lastCommandTime;
  status["wifi_strength"] = WiFi.RSSI();
  status["timestamp"] = getTimestamp();
  publishMessage(MQTT_TOPIC_STATUS, status);
}
