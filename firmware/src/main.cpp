#include <Arduino.h>
#include "config.h"
#include "light_controller.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

#ifndef WIFI_SSID
#define WIFI_SSID "default_ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "default_pass"
#endif

void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(STATUS_LED_PIN, HIGH); // Active low LED
  
  connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
  setupMQTT();

  publishStatus();  // Initial status
}

void loop() {
  loopMQTT();
  handleScheduledLightControl();  // Periodically check schedule
  delay(15000);  // poll again after 15 seconds
}
