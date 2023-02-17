#include <FirebaseESP32.h>
#include <WiFi.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "PacketInfo.h"
#include "DisplayFunctions.h"
#include <Pangodream_18650_CL.h>
#include "GatewayLora.h"


#define I2C_ADDRESS_OF_SCREEN   0x3C

std::list <PacketInfo> PacketsBuffer = std::list <PacketInfo>();
std::map <String, ANIMAL_TYPE> Nodes = std::map <String, ANIMAL_TYPE>(); //key = device_name (node)
                                           // data = animal_type (in node)

SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);
Pangodream_18650_CL BL;

void onReceive(int packetSize) {
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  Serial.print("Gateway Receive: ");
  Serial.println(message);

  fillPacketsBuffer(&PacketsBuffer ,message,LoRa.packetRssi());
}

// #define WIFI_SSID "Trio"
// #define WIFI_PASSWORD "DanaAmalAida"
// #define WIFI_SSID "Pilot2"
// #define WIFI_PASSWORD "carolteresafarah"
// #define WIFI_SSID "TechPublic"
// #define WIFI_PASSWORD ""
#define WIFI_SSID "Dana(phone)"
#define WIFI_PASSWORD "dana3004"

#define USER_EMAIL "iot.regrowth@gmail.com"
#define USER_PASSWORD "Regrowth123"

/* 2. Define the API Key */
#define API_KEY "0vh0uCSsK39x2AUAAavXKk8cRfkcGrFck3rpc6gf"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://regrowth-c498e-default-rtdb.europe-west1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app


//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String main="";

unsigned long previousMillis = 0;
const long interval = 43200000; // interval at which to send data (12 hours in milliseconds)

void setup()
{
  Serial.begin(115200);
  delay(2000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
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

  showLogo(&display);
  delay(2000);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa Transmitter");
  display.display();
  delay(2000);


  //Initianting SD card
  spi.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
  delay(200);
  if (!SD.begin(SD_CS,spi)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  init_sdcard_log(&timeinfo);

  // Configure the LoRA radio
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(LORA_BAND*1E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  
  LoRa.onReceive(onReceive);
  LoRa_rxMode();


}

void parseTime(String str, String &date, String &time, String &timeRange) {
  int hour = 0;
  int minute = 0;
  int second = 0;
  //Mon Feb 06 18:33:13 2023
  sscanf(str.c_str(), "%*s %*s %*d %d:%d:%d %*d", &hour, &minute, &second);
  date = str.c_str() + 4;
  date = date.substring(0, 6) + " " + date.substring(15,20);
  time = String(hour) + ":" + String(minute) + ":" + String(second);
  timeRange = (hour >= 12) ? "PM" : "AM";
}


void loop()
{  


  long rssi = WiFi.RSSI();
  display.clear();
  displayWifi(&display,rssi,(WiFi.status()!= WL_CONNECTED));
  displayNodeCount(&display,4,3,2);
  displayBattery(BL.getBatteryChargeLevel(),&display);

  // fillPacketsBuffer(&PacketsBuffer,"A5,050,123,78.2,0.67,3.3,11.3,73");

  //print on incoming nodes
  if(!PacketsBuffer.empty())
    displayPacketsBuffer(&PacketsBuffer,&display);
  
  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  delay(1500);

  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) %interval == 0) { //this means that 12 hours have passed 

    Serial.printf("Reading file: \n"); 
  File file = SD.open(current_log_filename);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  PacketInfo packet("A1","12836",2373,13,0.67,3.3,0.9,28,"Mon Feb 06 18:33:13 2023");
  //bool indicator= false;

  while (get_packet_from_sd(file,&packet)){
  if (Firebase.ready()) 
  { 
    
    trasnmitToFirebase(&packet);
    delay(200);
//     Serial.printf("Get Device Name  %s\n", Firebase.getString(fbdo, "/test/device_name") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     device_name=fbdo.to<String>();
//    Serial.printf("Get Animal Name %s\n", Firebase.getString(fbdo, "/test/animal_ID") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     animal_ID=fbdo.to<String>();
//    Serial.printf("Get Animal Type  %s\n", Firebase.getString(fbdo, "/test/animal_type") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     animal_type=fbdo.to<String>();
//     Serial.printf("Get Animal Weight  %s\n", Firebase.getInt(fbdo, "/test/animal_weight") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
  }
  file.close();
  delay(2500);
  }
  }
}

void trasnmitToFirebase (PacketInfo* packet) {
    String email= USER_EMAIL; 
    int atIndex = email.indexOf("@");
    String user = email.substring(0, atIndex);
    String numOfNode= packet->device_name;
    String animalType;
    animalType= Firebase.getString(fbdo, "/test/Users/"+user+ "/Node/" +numOfNode);
    if (animalType == NULL) {
      animalType="Chicken";
      Firebase.setString (fbdo, "/test/Users/" +user+ "/Node/" +numOfNode, animalType);
    } 
    int weight=packet->animal_weight;
    float humidity=packet->humidity;
    double temperature= packet->temperature;
    String animalNumber= packet->rfid_reading;
    float connection=packet->RSSI;
    float battery=packet->soc;
    double voltage= packet->voltage;
    String date;
    String time;
    String timeRange;
    parseTime(packet->time_received, date, time, timeRange);
    String GWId= WiFi.macAddress();
    bool batteryLow= (((packet->rfid_reading).toInt())==0);    
    Firebase.setFloat(fbdo, "/test/Users/" +user+ "/Data/Animal/" + animalType+ "Number/"+ animalNumber + "/" + date +"/Weight", weight);
    float activity= Firebase.getFloat( fbdo, "/test/Users/" + user+ "/Data/Animal" + animalType + "/Number" +animalNumber + "/" +date + "/Activity");
    if (!activity){ 
      activity=1;
    }
      else 
      activity++;
    Firebase.setFloat(fbdo, "/test/Users/" +user+ "/Data/Animal/" + animalType+ "/"+ animalNumber + "/" + date +"/Activity", activity);
    int count= Firebase.getInt(fbdo, "/test/Users/"+user+ "/Date/Animal/" + animalType + "/Number");
    count++;
    Firebase.setInt (fbdo, "/test/Users/"+ user+ "/Data/Animal/"+ animalType+"/Number",count );
    Firebase.setDouble(fbdo, "/test/Users/" +user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" +date+ "/" +timeRange + "/Humidity", humidity);
    Firebase.setDouble(fbdo, "/test/Users/" +user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" +date+ "/" +timeRange + "/Temperature" , temperature);
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Battery", battery ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Connection", connection ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Tension", voltage ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Gateway", GWId ); 
    Firebase.setBool(fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/BatteryLow", batteryLow ); 

}

