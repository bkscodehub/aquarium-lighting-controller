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

static const char* MQTT_SSL = R"(-----BEGIN CERTIFICATE-----
  MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
  TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
  cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
  WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
  ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
  MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
  h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
  0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
  A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
  T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
  B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
  B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
  KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
  OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
  jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
  qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
  rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
  HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
  hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
  ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
  3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
  NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
  ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
  TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
  jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
  oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
  4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
  mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
  emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
  -----END CERTIFICATE-----)";

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
  initMQTT(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD, MQTT_PORT, MQTT_SSL, callbacks, sizeof(callbacks) / sizeof(callbacks[0]));
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
