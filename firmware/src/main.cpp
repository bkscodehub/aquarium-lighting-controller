#include <Arduino.h>
#include "env.h"
#include "config.h"
#include "light_controller.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, HIGH); // Active low LED
  
  connectToWiFi(WIFI_SSID, WIFI_PASS);
  setupMQTT();

  publishStatus();  // Initial status
}

void loop() {
  loopMQTT();
  handleScheduledLightControl();  // Periodically check schedule
  delay(15000);  // poll again after 15 seconds
}
