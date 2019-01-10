#include "FS.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>


#include "config.h"

/************ WIFI and MQTT Information******************/
char mqtt_server[40] = "";
char mqtt_port[6] = "1883";
char username[34] = "";
char password[34] = "";
char mqtt_client_id[34] = MQTT_CLIENT_ID;


/**************************** FOR OTA **************************************************/
const char* eps_name = MQTT_CLIENT_ID;
const char* ota_password = OTA_PASSWORD;
int ota_port = OTA_PORT;

/************* MQTT TOPICS  **************************/
String client_id_string = MQTT_CLIENT_ID;

String availability_topic_str = client_id_string + MQTT_AVAILABILITY_TOPIC;
const char* availability_topic = availability_topic_str.c_str();

String outlet_1_state_topic_str = client_id_string + MQTT_OUTLET_1_STATE_TOPIC;
const char* outlet_1_state_topic = outlet_1_state_topic_str.c_str();
String outlet_2_state_topic_str = client_id_string + MQTT_OUTLET_2_STATE_TOPIC;
const char* outlet_2_state_topic = outlet_2_state_topic_str.c_str();
String outlets_state_topic_str = client_id_string + MQTT_OUTLETS_STATE_TOPIC;
const char* outlets_state_topic = outlets_state_topic_str.c_str();

String outlet_1_command_topic_str = client_id_string + MQTT_OUTLET_1_COMMAND_TOPIC;
const char* outlet_1_command_topic = outlet_1_command_topic_str.c_str();
String outlet_2_command_topic_str = client_id_string + MQTT_OUTLET_2_COMMAND_TOPIC;
const char* outlet_2_command_topic = outlet_2_command_topic_str.c_str();
String outlets_command_topic_str = client_id_string + MQTT_OUTLETS_COMMAND_TOPIC;
const char* outlets_command_topic = outlets_command_topic_str.c_str();

const char* on_cmd = MQTT_ON_COMMAND;
const char* off_cmd = MQTT_OFF_COMMAND;

const char* birth_message = MQTT_BIRTH_MESSAGE;
const char* lwt_message = MQTT_LWT;

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

bool outlet_1_power = false;
bool outlet_2_power = false;
bool should_toggle_1 = false;
bool should_toggle_2 = false;
bool on_published = false;
bool off_published = false;
int outlet_1_button_state;
int outlet_2_button_state;
int last_outlet_1_button_state = LOW;
int last_outlet_2_button_state = LOW;
unsigned long last_outlet_1_button_debounce_time = 0;
unsigned long last_outlet_2_button_debounce_time = 0;
unsigned long debounceDelay = 100;

bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/********************************** START SETUP*****************************************/
void setup() {
  Serial.begin(115200);

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(username, json["username"]);
          strcpy(password, json["password"]);
          strcpy(mqtt_client_id, json["client_id"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  pinMode(OUTLET_1_BUTTON_PIN, INPUT);
  pinMode(OUTLET_2_BUTTON_PIN, INPUT);
  pinMode(OUTLET_1_PIN, OUTPUT);
  pinMode(OUTLET_2_PIN, OUTPUT);

  // Turn them on when it starts up?
  digitalWrite(OUTLET_1_PIN, LOW);
  digitalWrite(OUTLET_2_PIN, LOW);
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  WiFiManagerParameter custom_username("username", "username", username, 32);
  WiFiManagerParameter custom_password("password", "password", password, 32);
  WiFiManagerParameter custom_mqtt_client_id("client_id", "client_id", mqtt_client_id, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_username);
  wifiManager.addParameter(&custom_password);
  wifiManager.addParameter(&custom_mqtt_client_id);

  //reset settings
  //wifiManager.resetSettings();

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(MQTT_CLIENT_ID)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(username, custom_username.getValue());
  strcpy(password, custom_password.getValue());
  strcpy(mqtt_client_id, custom_mqtt_client_id.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["username"] = username;
    json["password"] = password;
    json["client_id"] = mqtt_client_id;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  String client_id_string = String(mqtt_client_id);

  availability_topic_str = client_id_string + MQTT_AVAILABILITY_TOPIC;
  availability_topic = availability_topic_str.c_str();

  outlet_1_state_topic_str = client_id_string + MQTT_OUTLET_1_STATE_TOPIC;
  outlet_1_state_topic = outlet_1_state_topic_str.c_str();
  outlet_2_state_topic_str = client_id_string + MQTT_OUTLET_2_STATE_TOPIC;
  outlet_2_state_topic = outlet_2_state_topic_str.c_str();
  outlets_state_topic_str = client_id_string + MQTT_OUTLETS_STATE_TOPIC;
  outlets_state_topic = outlets_state_topic_str.c_str();

  outlet_1_command_topic_str = client_id_string + MQTT_OUTLET_1_COMMAND_TOPIC;
  outlet_1_command_topic = outlet_1_command_topic_str.c_str();
  outlet_2_command_topic_str = client_id_string + MQTT_OUTLET_2_COMMAND_TOPIC;
  outlet_2_command_topic = outlet_2_command_topic_str.c_str();
  outlets_command_topic_str = client_id_string + MQTT_OUTLETS_COMMAND_TOPIC;
  outlets_command_topic = outlets_command_topic_str.c_str();

  client.setServer(mqtt_server, atoi(mqtt_port)); // parseInt to the port
  client.setCallback(callback);

  //OTA SETUP
  ArduinoOTA.setPort(ota_port);
  ArduinoOTA.setHostname(eps_name);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)ota_password);

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
  if (strcmp(outlet_1_command_topic, requestedTopic.c_str()) == 0) {
    if (requestedAction == "ON" && !outlet_1_power) {
      Serial.println("Turning outlet 1 on");
      digitalWrite(OUTLET_1_PIN, HIGH);
    } else if (requestedAction == "OFF" && outlet_1_power) {
      Serial.println("Turning outlet 2 off");
      digitalWrite(OUTLET_1_PIN, LOW);
    } else {
      Serial.println("Invalid action requested.");
    }
  }
  if (strcmp(outlet_2_command_topic, requestedTopic.c_str()) == 0) {
    if (requestedAction == "ON" && !outlet_2_power) {
      Serial.println("Turning outlet 2 on");
      digitalWrite(OUTLET_2_PIN, HIGH);
    } else if (requestedAction == "OFF" && outlet_2_power) {
      Serial.println("Turning outlet 2 off");
      digitalWrite(OUTLET_2_PIN, LOW);
    } else {
      Serial.println("Invalid action requested.");
    }
  }
  if (strcmp(outlets_command_topic, requestedTopic.c_str()) == 0) {
    if (requestedAction == "ON") {
      Serial.println("Turn both on");
      digitalWrite(OUTLET_1_PIN, HIGH);
      digitalWrite(OUTLET_2_PIN, HIGH);
    } else if (requestedAction == "OFF") {
      Serial.println("Turn both off");
      digitalWrite(OUTLET_1_PIN, LOW);
      digitalWrite(OUTLET_2_PIN, LOW);
    } else {
      Serial.println("Invalid action requested.");
    }
  }
  publish_all_states();
}

void publish_all_states() {
  if (digitalRead(OUTLET_1_PIN) == HIGH && !outlet_1_power) {
    client.publish(outlet_1_state_topic, "ON", true);
    outlet_1_power = true;
  }
  if (digitalRead(OUTLET_1_PIN) == LOW && outlet_1_power) {
    client.publish(outlet_1_state_topic, "OFF", true);
    outlet_1_power = false;
  }
  if (digitalRead(OUTLET_2_PIN) == HIGH && !outlet_2_power) {
    client.publish(outlet_2_state_topic, "ON", true);
    outlet_2_power = true;
  }
  if (digitalRead(OUTLET_2_PIN) == LOW && outlet_2_power) {
    client.publish(outlet_2_state_topic, "OFF", true);
    outlet_2_power = false;
  }
  if (outlet_1_power || outlet_2_power) {
    if (off_published || !off_published && !on_published ) {
      Serial.println("Publishing one or both on message");
      client.publish(outlets_state_topic, "ON", true);
      on_published = true;
      off_published = false;
    }
  } else {
    if (on_published || !off_published && !on_published) {
      Serial.println("Publishing both off message");
      client.publish(outlets_state_topic, "OFF", true);
      off_published = true;
      on_published = false;
    }
  }
}

void publish_birth_message() {
  // Publish the birthMessage
  Serial.print("Publishing birth message \"");
  Serial.print(birth_message);
  Serial.print("\" to ");
  Serial.print(availability_topic);
  Serial.println("...");
  client.publish(availability_topic, birth_message, true);
}

void check_outlet_1_button() {
  int reading = digitalRead(OUTLET_1_BUTTON_PIN);

  if (reading != last_outlet_1_button_state) {
    last_outlet_1_button_debounce_time = millis();
  }

  if ((millis() - last_outlet_1_button_debounce_time) > debounceDelay) {
    if (reading != outlet_1_button_state) {
      outlet_1_button_state = reading;

      if (outlet_1_button_state == LOW) {
        Serial.println("Outlet 1 button pressed");
        should_toggle_1 = true;
      }
    }
  }

  last_outlet_1_button_state = reading;
}

void check_outlet_2_button() {
  int reading = digitalRead(OUTLET_2_BUTTON_PIN);

  if (reading != last_outlet_2_button_state) {
    last_outlet_2_button_debounce_time = millis();
  }

  if ((millis() - last_outlet_2_button_debounce_time) > debounceDelay) {
    if (reading != outlet_2_button_state) {
      outlet_2_button_state = reading;

      // only toggle the LED if the new button state is HIGH
      if (outlet_2_button_state == LOW) {
        Serial.println("Outlet 2 button pressed");
        should_toggle_2 = true;
      }
    }
  }

  last_outlet_2_button_state = reading;
}


/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_id, username, password, availability_topic, 0, true, lwt_message)) {
      Serial.println("connected");
      client.subscribe(outlet_1_command_topic);
      client.subscribe(outlet_2_command_topic);
      client.subscribe(outlets_command_topic);
      publish_birth_message();
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

  check_outlet_1_button();
  if (!outlet_1_power && should_toggle_1) {
    Serial.println("Turning on");
    digitalWrite(OUTLET_1_PIN, HIGH);
    should_toggle_1 = false;
  }
  if (outlet_1_power && should_toggle_1) {
    Serial.println("Turning off");
    digitalWrite(OUTLET_1_PIN, LOW);
    should_toggle_1 = false;
  }

  check_outlet_2_button();
  if (!outlet_2_power && should_toggle_2) {
    Serial.println("Turning on");
    digitalWrite(OUTLET_2_PIN, HIGH);
    should_toggle_2 = false;
  }
  if (outlet_2_power && should_toggle_2) {
    Serial.println("Turning off");
    digitalWrite(OUTLET_2_PIN, LOW);
    should_toggle_2 = false;
  }

  publish_all_states();

  client.loop();

  ArduinoOTA.handle();

}
