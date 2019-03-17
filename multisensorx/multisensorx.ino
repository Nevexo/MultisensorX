// MultisensorX (MSX) - A NodeMCU Multisensor Harness
// By Nevexo (github.com/nevexo/multisensorx)

#include "CONFIG.h"
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configure globals used in time management
unsigned long previousMillis = 0;

// Setup onewire
OneWire oneWire(BUS_ONEWIRE_PIN);
// Setup DallsTemp
DallasTemperature sensors(&oneWire);
DeviceAddress DS18 = SENSOR_DS18B20_ADDR;

// Setup MQTT
WiFiClient espClient;
void callback(char* topic, byte* payload, unsigned int length) {
  log("[MSX] [MQTT] Got Data on: ");
  if (FEATURE_SERIAL_LOGGING) {
    Serial.print(topic);
  }
  // MQTT Data Handler
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  if (topic == OUTPUT_LED_TOPIC && FEATURE_LEDS_ENABLE) {
    // LED Command get!
    digitalWrite(OUTPUT_LED_R, HIGH);
    log(message);
  }
}

PubSubClient client(MQTT_HOST, MQTT_PORT, callback, espClient);

long lastReconnectAttempt = 0;

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("[MSX] [MQTT] Attempting to connect...");
    // Create a random client ID
    // Attempt to connect
    if (client.connect(MQTT_IDENT)) {
      log("[MSX] [MQTT] Connected!");
    } else {
      log("failed, rc=");
      log(client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  
  // Configure RGB LED(s)
  
  if (FEATURE_LEDS_ENABLE) {
    log("[MSX] [LED] Configuring LEDs...");
    pinMode(OUTPUT_LED_R, OUTPUT);
    pinMode(OUTPUT_LED_G, OUTPUT);
    pinMode(OUTPUT_LED_B, OUTPUT);
    client.subscribe(OUTPUT_LED_TOPIC);
  }
}


// Helpers
void log(char *Message) {
  if (FEATURE_SERIAL_LOGGING == true) {
    Serial.println(Message);
  }
}

void espLed(boolean state) {
  if (FEATURE_ESP_LED) {
    if (state) {
      digitalWrite(2, LOW);      
    }else {
      digitalWrite(2, HIGH);
    }
  }
}

void listOnewireDevs(void) {
  // By Henry's Bench
  byte i;
  byte addr[8];
  
  Serial.println("Getting the address...\n\r");
  /* initiate a search for the OneWire object we created and read its value into
  addr array we declared above*/
  
  while(oneWire.search(addr)) {
    Serial.print("The address is:\t");
    //read each byte in the address array
    for( i = 0; i < 8; i++) {
      Serial.print("0x");
      if (addr[i] < 16) {
        Serial.print('0');
      }
      // print each byte in the address array in hex format
      Serial.print(addr[i], HEX);
      if (i < 7) {
        Serial.print(", ");
      }
    }
    // a check to make sure that what we read is correct.
    if ( OneWire::crc8( addr, 7) != addr[7]) {
        Serial.print("CRC is not valid!\n");
        return;
    }
  }
  oneWire.reset_search();
  return;
}

void setup() {
  // Configure the MSX Board

  if (FEATURE_SERIAL_LOGGING) {
    // Setup serial logging if it's enabled
    Serial.begin(SERIAL_BUARD);
    Serial.println("[MSX] Welcome to MultisensorX!");
  }
  
  if (FEATURE_ESP_LED) {
    // Setup onboard LED
    pinMode(2, OUTPUT);
  }

  // Connect to WiFi
  log("[MSX] [LINK] Waiting for Wi-Fi to init...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int counter = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if ((counter % 2) == 0) {
      espLed(false);
    }else {
      espLed(true);
    }
    counter++;
  }
  // Wi-Fi Has connected!
  log("[MSX] [LINK] Connected to Wi-Fi!");
  espLed(false);

  // List Bus Devices (feature)
  if (FEATURE_ONEWIRE_LIST_DEV && BUS_ONEWIRE_PIN != false) {
    // List all connected OneWire Devices to serial
    log("[MSX] [BUS] Listing OneWire bus devices:");
    listOnewireDevs();
  }

  // Configure DallasTemp Device (DS18B20)

  if (FEATURE_DS18B20_ENABLE && BUS_ONEWIRE_PIN != false) {
    sensors.begin();
    sensors.setResolution(12);
    log("[MSX] [TEMP] [DSB] Enabled DS18B20 on OneWire Bus!");
  }
}

void loop() {
  // Main System Loop

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= MQTT_PUBLISH_CLOCK * 1000 && client.connected()) {
    // Everytime this runs, the device will publish to all enabled topics.
    previousMillis = currentMillis;

    // Publish DS1820 values:
    if (FEATURE_DS18B20_ENABLE) {
      sensors.requestTemperatures();
      float temp = sensors.getTempC(DS18);
      log ("[MSX] [SENSE] [DS18] Current Reading: "); 
      if (FEATURE_SERIAL_LOGGING) {
        Serial.print(temp);
      }
      client.publish(SENSOR_DS18B20_TEMP_TOPIC, String(temp).c_str(), true);
    }

  }
}