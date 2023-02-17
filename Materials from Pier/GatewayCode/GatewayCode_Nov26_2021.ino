/*
 
  Date  : Nov 26, 2021

  Software Requirements:
  ----------------------
  -Arduino IDE
  #include <WiFi.h>
  #include <SPI.h>
  #include <LoRa.h>

  Hardware Requirements:
  ----------------------
  -ESP32
  -LoRa

  Project Requirents:
  -------------------
  Gateway of System
*/

#include <WiFi.h>
#include <SPI.h>
#include <LoRa.h>

const char WiFiAPPSK[] = "1234567";
WiFiServer server1(80);
String deviceID = "Gateway";

const char apn[]      = "internet.it";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char simPIN[]   = "";
const char server[] = "leaftechsystem.com";
const char resource[] = "/update.php";
const int  port = 80;

#define SCK  18
#define MISO 19
#define MOSI 12

// TTGO T-Call pins
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
#define TINY_GSM_MODEM_SIM800      // Modem is SIM800
#define TINY_GSM_RX_BUFFER   1024  // Set RX buffer to 1Kb
#define SerialAT Serial1

#include <Wire.h>
#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TwoWire I2CPower = TwoWire(0);
TinyGsmClient client(modem);

#define uS_TO_S_FACTOR 1000000UL   /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  3600        /* Time ESP32 will go to sleep (in seconds) 3600 seconds = 1 hour */

#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00

#define BAND 866E6
#define csPin    15
#define resetPin 14
#define irqPin   2
String data;

bool setPowerBoostKeepOn(int en) {
  I2CPower.beginTransmission(IP5306_ADDR);
  I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    I2CPower.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    I2CPower.write(0x35); // 0x37 is default reg value
  }
  return I2CPower.endTransmission() == 0;
}

void setup() {
  Serial.begin(9600);
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(BAND)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  LoRa.onReceive(onReceive);
  gsmInit();
  Serial.print("Connecting to APN: ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
  }
  else {
    Serial.println(" OK");
    Serial.print("Connecting to ");
    Serial.print(server);
    if (!client.connect(server, port)) {
      Serial.println(" fail");
    }
    else {
      Serial.println(" OK");
    }
  }
  LoRa_rxMode();
}

void loop() {
  updateData();
}

void updateData() {
  if (data.length() > 0) {
    Serial.print("Connecting to APN: ");
    Serial.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" fail");
    }
    else {
      Serial.println(" OK");
      Serial.print("Connecting to ");
      Serial.print(server);
      if (!client.connect(server, port)) {
        Serial.println(" fail");
      }
      else {
        Serial.println(" OK");
        Serial.println("Performing HTTP POST request...");
        String httpRequestData = data;
        client.print(String("POST ") + resource + " HTTP/1.1\r\n");
        client.print(String("Host: ") + server + "\r\n");
        client.println("Connection: close");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-Length: ");
        client.println(httpRequestData.length());
        client.println();
        client.println(httpRequestData);
        unsigned long timeout = millis();
        while (client.connected() && millis() - timeout < 10000L) {
          while (client.available()) {
            char c = client.read();
            Serial.print(c);
            timeout = millis();
          }
        }
        Serial.println();
        client.stop();
        Serial.println(F("Server disconnected"));
        modem.gprsDisconnect();
        Serial.println(F("GPRS disconnected"));
      }
    }
    //esp_deep_sleep_start();
  }
}

void LoRa_rxMode() {
  LoRa.disableInvertIQ();
  LoRa.receive();
}

void onReceive(int packetSize) {
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  data = message;
  Serial.print("Gateway Receive: ");
  Serial.println(message);
}

void gsmInit() {
  bool isOk = setPowerBoostKeepOn(1);
  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
  Serial.println("Initializing modem...");
  modem.restart();
  if (strlen(simPIN) && modem.getSimStatus() != 3 ) {
    modem.simUnlock(simPIN);
  }
  //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
}
void setupWiFi()
{
  char data[deviceID.length() + 1];
  deviceID.toCharArray(data , deviceID.length() + 1);
  WiFi.softAP(data , WiFiAPPSK);
  server1.begin();
}
