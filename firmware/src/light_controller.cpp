#include <ArduinoJson.h>
#include "light_controller.h"
#include "config.h"
#include <time_utils.h>
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

static const char *MQTT_SSL = R"(-----BEGIN CERTIFICATE-----
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

LightMode currentMode = NONE;
LightMode prevMode = NONE;
bool lightState = false;

unsigned long lastStatusPublish = 0;
String lastCommandTime = "";
int scheduledOnHour = -1, scheduledOnMinute = -1;
int scheduledOffHour = -1, scheduledOffMinute = -1;

String cmd = "";    // "ON" | "OFF" | "AUTO"
String source = ""; // "dashboard" | "mqtt_api" | "manual"

PersistentLightScheduleState state;
const int EEPROM_SIZE = sizeof(PersistentLightScheduleState);

void initLightController()
{
  EEPROM.begin(EEPROM_SIZE);

  // Read stored state
  EEPROM.get(0, state);

  // Optional: Validate the retrieved data
  if ((state.scheduledOnHour < -1 || state.scheduledOnHour > 23) &&
      (state.scheduledOnMinute < -1 || state.scheduledOnMinute > 59) &&
      (state.scheduledOffHour < -1 || state.scheduledOffHour > 23) &&
      (state.scheduledOffMinute < -1 || state.scheduledOffMinute > 59))
  {
    // Uninitialized EEPROM or corrupted data; Reset persisted schedule
    Serial.println("Schedule state in EEPROM not initialized or is corrupt. Resetting schedule state.");
    state.scheduledOnHour = -1;
    state.scheduledOnMinute = -1;
    state.scheduledOffHour = -1;
    state.scheduledOffMinute = -1;
    String timestamp = getTimestamp(); // Ensure getTimestamp() returns a string in ISO 8601 format
    timestamp.toCharArray(state.scheduleDefinedOn, sizeof(state.scheduleDefinedOn));
  }
  else
  {
    Serial.print("Using persisted schedule. ON at ");
    Serial.print(state.scheduledOnHour);
    Serial.print(":");
    Serial.print(state.scheduledOnMinute);
    Serial.print(", OFF at ");
    Serial.print(state.scheduledOffHour);
    Serial.print(":");
    Serial.print(state.scheduledOffMinute);
    Serial.print(", Defined on ");
    Serial.println(state.scheduleDefinedOn);

    // If valid schedule exists in EEPROM then set to AUTO mode
    if (state.scheduledOnHour != -1 && state.scheduledOnMinute != -1 && state.scheduledOffHour != -1 && state.scheduledOffMinute != -1)
    {
      Serial.println("Schedule exists during bootstrap. Setting mode to AUTO");
      currentMode = AUTO;
    }
  }
}

void setLight(bool state)
{
  Serial.print("Turning Aquarium light ");
  Serial.println(state ? "ON" : "OFF");
  lightState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);      // HIGH → Relay ON (closes  switch) | LOW → Relay OFF(NO stays open)
  digitalWrite(STATUS_LED_PIN, state ? LOW : HIGH); // Status LED
}

void handleCommandMessage(const String &topic, const String &payload)
{
  Serial.println("Handle command message");
  if (topic == MQTT_TOPIC_CMD)
  {
    ArduinoJson::DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    cmd = doc["command"].as<String>();   // "ON" | "OFF" | "AUTO"
    source = doc["source"].as<String>(); // "dashboard" | "mqtt_api" | "manual"

    bool changed = false;
    bool success = true;
    String resp_msg = "";

    if (cmd == "ON")
    {
      setLight(true);
      currentMode = MANUAL;
    }
    else if (cmd == "OFF")
    {
      setLight(false);
      currentMode = MANUAL;
    }
    else if (cmd == "AUTO")
    {
      if (state.scheduledOnHour != -1 && state.scheduledOnMinute != -1 && state.scheduledOffHour != -1 && state.scheduledOffMinute != -1)
      {
        currentMode = AUTO;
      }
      else
      {
        resp_msg = "Schedule not available. Update schedule first. Command Rejected!";
        success = false;
      }
    }
    else
    {
      currentMode = NONE;
      success = false;
    }

    changed = (currentMode != prevMode);
    lastCommandTime = getTimestamp();

    // Build acknowledgment message
    ArduinoJson::DynamicJsonDocument ack(256);
    ack["ack_for"] = MQTT_TOPIC_CMD;
    ack["cmd_received"] = cmd;
    ack["prev_mode"] = (prevMode == AUTO ? "AUTO" : "MANUAL");
    ack["new_mode"] = (currentMode == AUTO ? "AUTO" : "MANUAL");
    ack["changed"] = changed;
    ack["timestamp"] = lastCommandTime;
    ack["success"] = success;
    ack["msg"] = resp_msg;
    ack["source"] = source;

    Serial.println("Publish acknowledgement");
    publishMessage(MQTT_TOPIC_ACK, ack);
    publishStatus();
    prevMode = currentMode;
  }
  else
  {
    Serial.print("ERROR: Topic mismatch! Expected ");
    Serial.print(MQTT_TOPIC_CMD);
    Serial.print(", but received ");
    Serial.println(topic);
  }
}

void handleScheduleMessage(const String &topic, const String &payload)
{
  Serial.println("Handle schedule message");
  if (topic != MQTT_TOPIC_SCHEDULE)
  {
    Serial.print("ERROR: Topic mismatch. Expected ");
    Serial.print(MQTT_TOPIC_SCHEDULE);
    Serial.print(", but received ");
    Serial.println(topic);
    return;
  }

  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, payload);

  bool success = true;
  String resp_msg = "";
  String onTime = "";
  String offTime = "";
  String source = "unknown";
  bool enabled = true;
  if (error)
  {
    Serial.print("ERROR: Failed to parse schedule message: ");
    Serial.println(error.c_str());
    // return;
    success = false;
    resp_msg = error.c_str();
  }
  else
  {

    // Extract fields with default values
    onTime = doc["on"] | "";
    offTime = doc["off"] | "";
    source = doc["source"] | "unknown";
    enabled = doc["enabled"] | true;

    // Validate time formats
    if (sscanf(onTime.c_str(), "%d:%d", &state.scheduledOnHour, &state.scheduledOnMinute) != 2 ||
        sscanf(offTime.c_str(), "%d:%d", &state.scheduledOffHour, &state.scheduledOffMinute) != 2)
    {
      Serial.println("ERROR: Invalid time format in schedule message. Expected HH:MM.");
      // return;
      success = false;
      resp_msg = "ERROR: Invalid time format in schedule message. Expected HH:MM.";
    }

    // Optional: Validate hour and minute ranges
    if (state.scheduledOnHour < 0 || state.scheduledOnHour > 23 || state.scheduledOnMinute < 0 || state.scheduledOnMinute > 59 ||
        state.scheduledOffHour < 0 || state.scheduledOffHour > 23 || state.scheduledOffMinute < 0 || state.scheduledOffMinute > 59)
    {
      Serial.println("ERROR: Invalid time values in schedule message.");
      // return;
      success = false;
      resp_msg = "ERROR: Invalid time values in schedule message.";
    }

    if (success)
    {                                    // No validation errors
      String timestamp = getTimestamp(); // Ensure getTimestamp() returns a string in ISO 8601 format
      timestamp.toCharArray(state.scheduleDefinedOn, sizeof(state.scheduleDefinedOn));

      // Write updated state to EEPROM
      EEPROM.put(0, state);
      EEPROM.commit();

      Serial.print("Schedule set - ON at ");
      Serial.print(state.scheduledOnHour);
      Serial.print(":");
      Serial.print(state.scheduledOnMinute);
      Serial.print(", OFF at ");
      Serial.print(state.scheduledOffHour);
      Serial.print(":");
      Serial.println(state.scheduledOffMinute);
    }
  }
  // Construct acknowledgment message
  DynamicJsonDocument ack(256);
  ack["ack_for"] = MQTT_TOPIC_SCHEDULE;
  ack["schedule_received"] = "On-" + onTime + ", Off-" + offTime;
  ack["timestamp"] = getTimestamp(); // Assumes getTimestamp() returns ISO 8601 format in IST
  ack["success"] = success;
  ack["msg"] = resp_msg;
  ack["source"] = source;

  serializeJson(ack, Serial); // Serialize and print JSON
  Serial.println();           // Ensure a new line for better readability

  Serial.println("Publish acknowledgement");
  publishMessage(MQTT_TOPIC_ACK, ack);
  publishStatus();
}

void handleScheduledLightControl()
{
  if (currentMode == AUTO)
  {
    int currentHour = getHourNow();
    int currentMinute = getMinuteNow();

    bool shouldTurnOn = isTimeInRange(state.scheduledOnHour, state.scheduledOnMinute, state.scheduledOffHour, state.scheduledOffMinute);

    if (lightState != shouldTurnOn)
    {
      setLight(shouldTurnOn); // Toggle aquarium light
      publishStatus();
    }
  }

  if (millis() - lastStatusPublish > 1000 * 60 * 30)
  { // Every 30 minutes
    publishStatus();
    lastStatusPublish = millis();
  }
}

void setupMQTT()
{
  // Define topic-callback mappings
  static MqttCallbackEntry callbacks[] = {
      {MQTT_TOPIC_CMD, handleCommandMessage},
      {MQTT_TOPIC_SCHEDULE, handleScheduleMessage}};

  // Initialize MQTT
  initMQTT(MQTT_BROKER, MQTT_USERNAME, MQTT_PASSWORD, MQTT_PORT, MQTT_SSL, callbacks, sizeof(callbacks) / sizeof(callbacks[0]));
}

void publishStatus()
{
  Serial.print("Publishing status message:");
  ArduinoJson::DynamicJsonDocument status(256);
  status["light_status"] = lightState ? "on" : "off";
  status["mode"] = (currentMode == AUTO ? "auto" : "manual");
  status["last_cmd"] = cmd;
  status["last_cmd_source"] = source;
  status["wifi_strength"] = WiFi.RSSI();
  status["last_cmd_timestamp"] = lastCommandTime;
  status["uptime_sec"] = millis() / 1000;

  // Serialize and print JSON
  serializeJson(status, Serial);
  Serial.println(); // Ensure a new line for better readability

  publishMessage(MQTT_TOPIC_STATUS, status);
}
