#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>
#include <Ethernet.h>

#include "Mudbus.h"
#include "StringSplitter.h"

Mudbus Mb;

#define DHTPIN 5
#define DHTTYPE    DHT22
#define SPTR_SIZE   20
char   *sPtr [SPTR_SIZE];

float acsMultiplier = 0.185;

DHT_Unified dht(DHTPIN, DHTTYPE);
EthernetServer server(8080);

uint32_t delayMS = 5000;
uint32_t lastMeasurementMillis;

float currentTemperature;
float currentHumidity;
float currentCurrent5v;
float currentCurrent12v;

bool ready;

float getAcsMeasurement(int pin) {
  float SensorRead = analogRead(pin) * (5.0 / 1023.0);
  return (SensorRead - 2.5) / acsMultiplier;
}

void setup() {
  Serial.begin(9600);

  uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x51, 0x06 };
  Ethernet.begin(mac);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  server.begin();
  Serial.print(F("Local IP: "));
  Serial.println(Ethernet.localIP());

  // Initialize device.
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
}

void loop() {
  Mb.Run();
  if (millis() - lastMeasurementMillis > delayMS) {
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
    }
    currentTemperature = event.temperature;
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
    }
    currentHumidity = event.relative_humidity;

    ready = true;
    currentCurrent5v = getAcsMeasurement(A0);
    currentCurrent12v = getAcsMeasurement(A1);

    Mb.R[0] = (int)(currentTemperature * 100);
    Mb.R[1] = (int)(currentHumidity * 100);
    Mb.R[2] = (int)(currentCurrent5v * 1000);
    Mb.R[3] = (int)(currentCurrent12v * 1000);

    lastMeasurementMillis = millis();
  }

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    int lineId = 0;
    String firstLine = "";
    Serial.println("new client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (lineId == 0) {
          firstLine += String(c);
        }
        
        if (c == '\n') {
          Serial.print("lineId = ");
          Serial.println(lineId, DEC);

          if (lineId == 0) {
            if (firstLine.indexOf("/health ") > 0) {
              client.println(F("HTTP/1.1 204 No Content"));
              client.println();
            } else if (firstLine.indexOf("/metrics ") > 0) {
              // send a standard http response header
              client.println(F("HTTP/1.1 200 OK"));
              client.println(F("Content-Type: text/plain"));
              client.println(F("Connection: close"));
              client.println();

              client.print(F("cluster_rack_temperature "));
              client.print(currentTemperature);
              client.print("\n");
              client.print(F("cluster_rack_humidity "));
              client.print(currentHumidity);
              client.print("\n");
              client.print(F("cluster_rack_current{bus=\"5v\"} "));
              client.print(currentCurrent5v);
              client.print("\n");
              client.print(F("cluster_rack_current{bus=\"12v\"} "));
              client.print(currentCurrent12v);
              client.print("\n");
              break;
            } else {
              client.println(F("HTTP/1.1 400 Bad Request"));
              client.println(F("Content-Type: text/plain"));
              client.println(F("Connection: close"));
              client.println();
              client.println("Bad Request");
              break;
            }
          }

          // you're starting a new line
          lineId++;
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
