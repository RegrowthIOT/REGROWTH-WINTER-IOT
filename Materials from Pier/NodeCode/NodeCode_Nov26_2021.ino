#include <ETH.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>

/*
 
  Date  : Nov 26, 2021

  Software Requirements:
  ----------------------
  -Arduino IDE
  #include <WiFi.h>
  #include <SPI.h>
  #include <LoRa.h>
  #include "DHT.h"
  #include "HX711.h"

  Hardware Requirements:
  ----------------------
  -ESP32
  -LoRa
  -HX711
  -DHT22

  Project Requirents:
  -------------------
  Node Device
*/

#include <WiFi.h>
#include <SPI.h>
#include <LoRa.h>
#include "DHT.h"
#include "HX711.h"

const char WiFiAPPSK[] = "1234567";
WiFiServer server(80);

String deviceID = "device001";
float calibration_factor = -237;

#define DHTPIN 4
#define DHTTYPE DHT11
#define STX 2
#define ETX 3
#define BAND 866E6
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 15

HX711 scale;
DHT dht(DHTPIN, DHTTYPE);

int rx_counter;
byte rx_data[31], rx_data_temp[31];
float temp , hum , weight;
String tagID , lastTagID;
unsigned long timer;
float units;

void setup() {
  Serial.begin(9600);
  setupWiFi();
  loraInit();
  weightInit();
  rfidInit();
  dhtInit() ;
}

void loop() {
  readRfid();
}

bool readRfid() {
  bool tagFound = false;
  if (Serial2.available()) {
    String input = "";
    while (Serial2.available()) {
      char data = Serial2.read();
      input = input + String(data , HEX); //tag id
    }
    if (input.length() >= 30) { //why larger than 30?
      input = hexToAscii(input);
      tagID = input.substring(1 , input.length() + 9);
      if (lastTagID != lastTagID) {
        readDht();
        readWeight();
        String data =  "api_key=tPmAT5Ab3j7F9&systemId=" + String(deviceID) + "&animalId=" + String(tagID) + "&temp=" + String(temp) + "&weight=" + String(weight) + "&hum=" + String(hum);
        LoRa_sendMessage(data);
        Serial.println(data);
      }
    }
  }
}

String hexToAscii( String hex )
{
  uint16_t len = hex.length();
  String ascii = "";
  for ( uint16_t i = 0; i < len; i += 2 )
    ascii += (char)strtol( hex.substring( i, i + 2 ).c_str(), NULL, 16 );
  return ascii;
}

void readWeight() {
  units = scale.get_units();
}

void readDht() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  if (isnan(temp)) {
    temp = 0;
  }
  if (isnan(hum)) {
    hum = 0;
  }
}

void LoRa_txMode() {
  LoRa.idle();
  LoRa.disableInvertIQ();
}

void LoRa_sendMessage(String message) {
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket(true);
}

void loraInit() {
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa init succeeded.");
  LoRa_txMode();
}

void dhtInit() {
  dht.begin();
}

void rfidInit() {
  Serial2.begin(9600);
}

void weightInit() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();
}

void setupWiFi()
{
  String mac = WiFi.macAddress();
  int x = mac.length();
  String macID = String(mac[x - 5]) + String(mac[x - 4]) +
                 String(mac[x - 2]) + String(mac[x - 1]);
  String deviceID = "SYS" + macID;
  Serial.println("Device ID: " + deviceID);
  char data[deviceID.length() + 1];
  deviceID.toCharArray(data , deviceID.length() + 1);
  WiFi.softAP(data , WiFiAPPSK);
}
