#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>



/************ WIFI and MQTT Information******************/
const char* ssid = "SSID";
const char* password = "PASSWORD";
const char* mqtt_server = "IP_ADDRESS";
const char* mqtt_username = "USERNAME";
const char* mqtt_password = "PASSWORD";
const int mqtt_port = 1883;



/**************************** FOR OTA **************************************************/
#define EPSNAME "power_strip_1"
#define OTApassword "OTA_PASSWORD"
int OTAport = 8266;



/************* MQTT TOPICS  **************************/
const char* outlet_1_state_topic = "power_strip_1/outlet_1/state";
const char* outlet_2_state_topic = "power_strip_1/outlet_2/state";
const char* outlet_1_set_topic = "power_strip_1/outlet_1/set";
const char* outlet_2_set_topic = "power_strip_1/outlet_2/set";

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/*********************************** PIN Defintions ********************************/
#define OUTLET_1_BUTTON_PIN    4
#define OUTLET_2_BUTTON_PIN    4
#define OUTLET_1_PIN 13
#define OUTLET_2_PIN 14

WiFiClient espClient;
PubSubClient client(espClient);

bool outlet_1_power = false;
bool outlet_2_power = false;


/********************************** START SETUP*****************************************/
void setup() {
  Serial.begin(115200);

  pinMode(OUTLET_1_BUTTON_PIN, INPUT);
  pinMode(OUTLET_2_BUTTON_PIN, INPUT);
  pinMode(OUTLET_1_PIN, OUTPUT);
  pinMode(OUTLET_2_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //OTA SETUP
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(EPSNAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

}




/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();

  String topicToProcess = topic;
  payload[length] = '\0';
  String payloadToProcess = (char*)payload;
  triggerAction(topicToProcess, payloadToProcess);
}

void triggerAction(String requestedTopic, String requestedAction) {
  Serial.println("MQTT request received.");
  Serial.print("Topic: ");
  Serial.println(requestedTopic);
  Serial.print("Action: ");
  Serial.println(requestedAction);
  if (strcmp(outlet_1_set_topic, requestedTopic.c_str()) == 0) {
    if (requestedAction == "ON" && !outlet_1_power) {
      Serial.println("Turning on");
      digitalWrite(OUTLET_1_PIN, HIGH);
    } else if (requestedAction == "OFF" && outlet_1_power) {
      Serial.println("Turning off");
      digitalWrite(OUTLET_1_PIN, LOW);
    } else {
      Serial.println("Invalid action requested.");
    }
  }
  publish_all_states();
}

void publish_all_states() {
  if (digitalRead(OUTLET_1_PIN) == HIGH) {
    client.publish(outlet_1_state_topic, "ON", true);
    outlet_1_power = true;
  }
}


/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(EPSNAME, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(outlet_1_set_topic);
      publish_all_states();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/********************************** START MAIN LOOP*****************************************/
void loop() {

  if (!client.connected()) {
    reconnect();
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(1);
    Serial.print("WIFI Disconnected. Attempting reconnection.");
    setup_wifi();
    return;
  }

  publish_all_states();

  client.loop();

  ArduinoOTA.handle();

}
