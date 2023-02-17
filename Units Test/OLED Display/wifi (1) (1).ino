#include <SPI.h>
#include <Wire.h>
#include <SSD1306.h>
#include <LoRa.h>
#include "images.h"
#include <WiFi.h>
#include <esp_wifi.h>

//#define LORA_BAND    433
//#define LORA_BAND    868
#define LORA_BAND    915

#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_RST    16

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define I2C_ADDRESS_OF_SCREEN   0x3C

// String SECRET_SSID = "Trio";
// String SECRET_PASS = "DanaAmalAida";

#define SECRET_SSID "TechPublic"
#define SECRET_PASS ""
// #define SECRET_SSID "Trio"
// #define SECRET_PASS "DanaAmalAida"
// #define SECRET_SSID "wrong"
// #define SECRET_PASS "wrong"


SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);


// Forward declarations
void displayLoraData(String countStr);
void showLogo();

void setup() {
  Serial.begin(115200);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Transmitter");

  // Configure the LED an an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Configure OLED by setting the OLED Reset HIGH, LOW, and then back HIGH
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(100);
  digitalWrite(OLED_RST, LOW);
  delay(100);
  digitalWrite(OLED_RST, HIGH);

  display.init();
  display.flipScreenVertically();

  showLogo();
  delay(2000);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa Transmitter");
  display.display();
  delay(2000);

  // Configure the LoRA radio
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(LORA_BAND * 1E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("init ok");
}

void loop() {
  static int counter = 0;

  long rssi = WiFi.RSSI();



  display.clear();
  displayWifi(rssi);
  displayNodeCount(4,3,0);

  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  counter++;
  delay(1500);
}

void displayWifi(long rssi) {
  // display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  String rssi_str = String(rssi, DEC);
  display.drawString(65, 0, rssi_str);

  if(WiFi.status()!= WL_CONNECTED){
    if(rssi == 0){
      display.drawString(90, 0, "Wifi Not");
      display.drawString(90, 10, "Found");
    }else{
      display.drawString(90, 0, "Not");
      display.drawString(75, 10, "Connected"); 
    }    
  }else{
    if(rssi == 0){
      display.drawString(90, 0, "Wifi Not");
      display.drawString(90, 10, "Found");    
    }else if (rssi >= -50) { 
      display.fillRect(102,7,4,1);
      display.fillRect(107,6,4,2);
      display.fillRect(112,4,4,4);
      display.fillRect(117,2,4,6);
      display.fillRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -50 & rssi >= -65) {
      display.fillRect(102,7,4,1);
      display.fillRect(107,6,4,2);
      display.fillRect(112,4,4,4);
      display.fillRect(117,2,4,6);
      display.drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -65 & rssi >= -75) {
      display.fillRect(102,8,4,1);
      display.fillRect(107,6,4,2);
      display.fillRect(112,4,4,4);
      display.drawRect(117,2,2,6);
      display.drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -75 & rssi >= -85) {
      display.fillRect(102,8,4,1);
      display.fillRect(107,6,4,2);
      display.drawRect(112,4,4,4);
      display.drawRect(117,2,4,6);
      display.drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -85 & rssi >= -96) {
      display.fillRect(102,8,4,1);
      display.drawRect(107,6,4,2);
      display.drawRect(112,4,4,4);
      display.drawRect(117,2,4,6);
      display.drawRect(122,0,4,8);
      // display.sendBuffer();
    } else {
      display.drawRect(102,8,4,1);
      display.drawRect(107,6,4,2);
      display.drawRect(112,4,4,4);
      display.drawRect(117,2,4,6);
      display.drawRect(122,0,4,8);
      // display.sendBuffer();
    }
  }


  display.display();
}




void displayLoraData(String countStr) {
  // display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  display.drawString(0, 0, "Sending packet: ");
  display.drawString(90, 0, countStr);
  display.display();
}

void displayNodeCount(int chickenCount, int pigCount, int sheepCount) {
  // display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  String nStr = "Nodes: ";
  nStr += String((chickenCount+pigCount+sheepCount), DEC);
  String cStr = "C: ";
  cStr += String(chickenCount, DEC);
  String pStr = "P: ";
  pStr += String(pigCount, DEC);
  String sStr = "S: ";
  sStr += String(sheepCount, DEC);  
  display.drawString(0, 0, nStr);
  display.drawString(0, 10, cStr);
  display.drawString(0, 20, pStr);
  display.drawString(0, 30, sStr);  
  display.display();
}


void showLogo() {
  uint8_t x_off = (display.getWidth() - logo_width) / 2;
  uint8_t y_off = (display.getHeight() - logo_height) / 2;

  display.clear();
  display.drawXbm(x_off, y_off, logo_width, logo_height, logo_bits);
  display.display();
}
