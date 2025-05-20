#ifndef CONFIG_H
#define CONFIG_H

#define RELAY_PIN 5          // GPIO5 (D1 on NodeMCU)
#define STATUS_LED_PIN 2     // GPIO2 (D4 on NodeMCU, active LOW LED)
#define DEBUG true

// MQTT Topics
#define MQTT_TOPIC_CMD        "home/aquarium/light_controller/cmd"
#define MQTT_TOPIC_SCHEDULE   "home/aquarium/light_controller/schedule"
#define MQTT_TOPIC_STATUS     "home/aquarium/light_controller/status"
#define MQTT_TOPIC_ACK        "home/aquarium/light_controller/ack"
#define MQTT_TOPIC_DEBUG      "home/aquarium/light_controller/debug"

#endif
