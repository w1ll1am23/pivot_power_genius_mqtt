// MQTT Parameters
#define MQTT_BROKER "MQTT_BROKER_IP"
#define MQTT_CLIENT_ID "pivotPowerGenius"

#define MQTT_AVAILABILITY_TOPIC "/availability"

#define MQTT_OUTLETS_COMMAND_TOPIC "/outlets/power/set"
#define MQTT_OUTLETS_STATE_TOPIC "/outlets/power/state"

#define MQTT_OUTLET_1_COMMAND_TOPIC "/outlet/1/power/set"
#define MQTT_OUTLET_1_STATE_TOPIC "/outlet/1/power/state"
#define MQTT_OUTLET_2_COMMAND_TOPIC "/outlet/2/power/set"
#define MQTT_OUTLET_2_STATE_TOPIC "/outlet/2/power/state"

#define MQTT_ON_COMMAND "ON"
#define MQTT_OFF_COMMAND "OFF"

#define MQTT_BIRTH_MESSAGE "online"
#define MQTT_LWT "offline"

// OTA config
#define OTA_PASSWORD "password"
#define OTA_PORT 8266

// PIN setup
#define OUTLET_1_BUTTON_PIN 13
#define OUTLET_2_BUTTON_PIN 12
#define OUTLET_1_PIN  4
#define OUTLET_2_PIN 5

// WiFi manager stuff
#define SETUP_AP_PASSWORD "password12345"
