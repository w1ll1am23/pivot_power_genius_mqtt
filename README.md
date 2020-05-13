# pivot_power_genius_mqtt
esp8266 replacement for the Electric Imp in the Quirky Pivot Power Genius

Don't use this code, it's terrible.

Use either Tasmota or ESPHome

ESPHome example:

```yaml
esphome:
  name: power_strip
  platform: ESP8266
  board: d1_mini

wifi:
  ssid: "WIFI"
  password: "WIFI_PASSWORD"

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: "Power Strip"
    password: "pivotpower"

captive_portal:

# Enable logging
logger:

# Enable Home Assistant API
api:

ota:

switch:
  - platform: gpio
    pin: GPIO5
    name: "outlet 2"
    id: "outlet2"
  - platform: gpio
    pin: GPIO4
    name: "outlet 1"
    id: "outlet1"

binary_sensor:
  - platform: gpio
    pin:
      number: GPIO12
      mode: INPUT_PULLUP
      inverted: True
    name: "outlet 2 button"
    internal: true
    on_click:
      min_length: 100ms
      then:
        - switch.toggle: "outlet2"
  - platform: gpio
    pin:
      number: GPIO14
      mode: INPUT_PULLUP
      inverted: True
    name: "outlet 1 button"
    internal: true
    on_click:
      min_length: 100ms
      then:
        - switch.toggle: "outlet1"
```

This code works on the Adafruit Huzzah, but doesn't work on Wemos D1 mini. I am not sure why. However is you are using a Wemos or any other esp8266 and it doesn't I had success use the Tasmota firmware and setting the module as generic, and assigning relays and buttons to the correct PINs. 

This repo contains all of the data on how to convert your Quirky Pivot power Genius to a 100% local MQTT driven device. 

All functionality of the Pivot Power Genius works!
- Both outlets work independently 2 command and 2 state topics
- Both outlets work as 1 using a seperate command and state topic
- Both buttons work as well. Clicking a button with toggle the associated outlets state.
- MQTT availbility
- WiFi and MQTT setup via captive portal

** When setting up the WiFI via the captive portal, if you have more than 1 Pivot Power Genius be sure to adjust the client id which defaults to PivotPowerGenius to something like PivotPowerGenius1

Follow the Wiki https://github.com/w1ll1am23/pivot_power_genius_mqtt/wiki for physical steps required to retrofit your Pivot Power Genius with an esp8266.


Full Home Assistant switch config for the Pivot Power Genius

```yaml
  - platform: mqtt
    name: "outlet_1"
    state_topic: "pivotPowerGenius/outlet/1/power/state"
    command_topic: "pivotPowerGenius/outlet/1/power/set"
    availability_topic: "pivotPowerGenius/availability"
    qos: 1
    payload_on: "ON"
    payload_off: "OFF"
    payload_available: "online"
    payload_not_available: "offline"
    retain: true    

  - platform: mqtt
    name: "outlet_2"
    state_topic: "pivotPowerGenius/outlet/2/power/state"
    command_topic: "pivotPowerGenius/outlet/2/power/set"
    availability_topic: "pivotPowerGenius/availability"
    qos: 1
    payload_on: "ON"
    payload_off: "OFF"
    payload_available: "online"
    payload_not_available: "offline"
    retain: true

  - platform: mqtt
    name: "both_outlets"
    state_topic: "pivotPowerGenius/outlets/power/state"
    command_topic: "pivotPowerGenius/outlets/power/set"
    availability_topic: "pivotPowerGenius/availability"
    qos: 1
    payload_on: "ON"
    payload_off: "OFF"
    payload_available: "online"
    payload_not_available: "offline"
    retain: true
```
