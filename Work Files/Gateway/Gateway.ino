#include <FirebaseESP32.h>
#include <WiFi.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "PacketInfo.h"
#include "DisplayFunctions.h"
#include <Pangodream_18650_CL.h>
#include "GatewayLora.h"

#define I2C_ADDRESS_OF_SCREEN 0x3C

#define WIFI_SSID "Trio"
#define WIFI_PASSWORD "DanaAmalAida"

#define USER_EMAIL "iot.regrowth@gmail.com"
#define USER_PASSWORD "regrowth123"

#define API_KEY "AIzaSyAbULIZmjSP87utoUVa5Mf1Jgz13Ffuolk"
#define DATABASE_SECRET "0vh0uCSsK39x2AUAAavXKk8cRfkcGrFck3rpc6gf"
#define DATABASE_URL "regrowth-c498e-default-rtdb.europe-west1.firebasedatabase.app"

//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String main = "";

// variables for obtaining the local time via wifi
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600;  //+2 UTC
const int daylightOffset_sec = 3600;

//variables for the time interval of sending the report to the firebase
unsigned long previousMillis = 0;
const long interval = 60000;// 43200000;  // interval at which to send data (12 hours in milliseconds = 12*60*60*1000)

int chickenCount = 0;
int pigCount = 0;
int sheepCount = 0;
int goatCount = 0;

std::list<PacketInfo> PacketsBuffer = std::list<PacketInfo>();
//std::map<String, ANIMAL_TYPE> Nodes = std::map<String, ANIMAL_TYPE>();  //key = device_name (node)
                                                                        // data = animal_type (in node)

String current_log_filename;
SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);
Pangodream_18650_CL BL;

/**
 * A function that is called once the lora buffer is filled with the received packet
 * @param packetSize - size of the packet received through the lora
 * note: This function is called unsynchronously
 */
void onReceive(int packetSize) {
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  Serial.print("Gateway Receive: ");
  Serial.println(message);
  int rssi = LoRa.packetRssi();
  fillPacketsBuffer(&PacketsBuffer, message, rssi);
}

/**
 * obtaining the values of a target time string, the date, time and timerange
 * the string is in the format "DAY-OF-WEEK MONTH DAY HH:MM:SS YEAR"
 * (e.g. Mon Feb 02 18:33:13 2023)
 * @param str - target time string to be parsed
 * @param date - a string containing the date
 * @param time - a string containing hours:minutes:seconds
 * @param timeRange - a variable specifying whether it's AM or PM
 */
void parseTime(String str, String& date, String& time, String& timeRange) {
  int hour = 0;
  int minute = 0;
  int second = 0;

  sscanf(str.c_str(), "%*s %*s %*d %d:%d:%d %*d", &hour, &minute, &second);
  date = str.c_str() + 4;
  date = date.substring(0, 6) + " " + date.substring(15, 20);
  time = String(hour) + ":" + String(minute) + ":" + String(second);
  timeRange = (hour >= 12) ? "PM" : "AM";
}

/**
 * Transmitting packet information received from nodes to the firebase
 * @param packet - a class that contains all the data from the packet
 */
void transmitToFirebase(PacketInfo& packet) {
  Serial.println("transmitting to firebase...");
  packet.PrintPacket(); Serial.println();
  
  String email = USER_EMAIL;
  int atIndex = email.indexOf("@");
  String user = email.substring(0, atIndex);

  double weight = packet.animal_weight;
  double humidity = packet.humidity;
  double temperature = packet.temperature;
  String animalNumber = packet.rfid_reading;
  double connection = packet.RSSI;
  double battery = packet.soc;
  double voltage = packet.voltage;
  String date;
  String time;
  String timeRange;
  parseTime(packet.time_received, date, time, timeRange);
  String GWId = WiFi.macAddress();
  bool batteryLow = (((packet.rfid_reading).toInt()) == 0);
 
  String numOfNode = packet.device_name;
  String animalType;

  /** adding the node to the available nodes list under the user name**/
  animalType = Firebase.getString(fbdo, "/test/Users/" + String(user) + "/Node/" + String(numOfNode) + "/") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str();
  //Serial.println(animalType);
  if (animalType == String(fbdo.errorReason().c_str())) {//if the reason is path not found then add empty string
  //if the reason is bad request then return
    animalType = " ";
  }

  //Firebase.setString(fbdo, "/test/Users/Node/", "Testing non dynamic path");
  Firebase.setString(fbdo, "/test/Users/" + user + "/Node/" + numOfNode + "/", animalType);

  if(animalType != " "){

  if (batteryLow) {
    Firebase.setDouble(fbdo, "/test/Users/" + user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" + date + "/" + timeRange + "/Humidity", humidity);
    Firebase.setDouble(fbdo, "/test/Users/" + user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" + date + "/" + timeRange + "/Temperature", temperature);
    Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Battery", battery);
    Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Connection", connection);
    Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Tension", voltage);
    Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/GatewayID", GWId);
    Firebase.setBool(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/BatteryLow", batteryLow);
    return;
  }

  Firebase.setDouble(fbdo, "/test/Users/" + user + "/Data/Animal/" + animalType +  "/" + animalNumber + "/" + date + "/Weight", weight);
  int activity = Firebase.getInt(fbdo, "/test/Users/" + user + "/Data/Animal" + animalType + "/Number" + animalNumber + "/" + date + "/Activity") ? fbdo.to<int>() : 0;
  if (!activity) {
    activity = 1;
  } else{
    activity++;
  }
  Firebase.setInt(fbdo, "/test/Users/" + user + "/Data/Animal/" + animalType + "/" + animalNumber + "/" + date + "/Activity", activity);
  int count = Firebase.getInt(fbdo, "/test/Users/" + user + "/Date/Animal/" + animalType + "/Number") ? fbdo.to<int>() :0;
  count++;
  Firebase.setInt(fbdo, "/test/Users/" + user + "/Data/Animal/" + animalType + "/Number", count);
  Firebase.setDouble(fbdo, "/test/Users/" + user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" + date + "/" + timeRange + "/Humidity", humidity);
  Firebase.setDouble(fbdo, "/test/Users/" + user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" + date + "/" + timeRange + "/Temperature", temperature);
  Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Battery", battery);
  Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Connection", connection);
  Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/Tension", voltage);
  Firebase.setString(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/GatewayID", GWId);
  Firebase.setBool(fbdo, "/test/Users/" + user + "/Animal/" + animalType + "/Node/" + numOfNode + "/BatteryLow", batteryLow);
  }  

}

/**
 * Setup:
 * - Serial ( baud = 115200)
 * - WiFi
 * - Firebase
 * - OLED display
 * - SD
 * - LORA (frequency set in LORA_BAND)
 * - Time configuration
 */
void setup() {
  Serial.begin(115200);
  delay(2000);
  /* Establishing Wifi connection */
 
  // Wifi connection is not needed at all times
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  for(int i=0; i < 10 && (WiFi.status() != WL_CONNECTED) ; i++) {
    Serial.print(".");
    delay(300);
  }
  if((WiFi.status() == WL_CONNECTED)){
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
  else{
    Serial.println();
    Serial.print("No Wifi Connection ");
    Serial.println();
  }


  /* Establishing firebase connection */
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.signer.tokens.legacy_token = "<database secret>";
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Firebase.begin(&config, &auth);
  Firebase.begin(DATABASE_URL, DATABASE_SECRET);
  Firebase.setDoubleDigits(5);

  /*OLED display set-up */

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

  /* displaying initial Logo and "Gateway" */
  showLogo(&display);
  delay(2000);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Gateway");
  display.display();
  delay(2000);


  //Initializing SD card
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(200);
  if (!SD.begin(SD_CS, spi)) {
    Serial.println("Card Mount Failed");
    ESP.restart();
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  /* configuring time according to timezone based on UTC */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  /* Lora device set-up */

  // Configure the LoRA radio
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(LORA_BAND * 1E6)) {
    Serial.println("LoRa init failed. Check your connections.");
  }
  Serial.println("LoRa init succeeded.");

  /* Creating a log on sd*/
  init_sdcard_log(&timeinfo, current_log_filename);

  //Serial.println(temp);
  Serial.println(current_log_filename);

  previousMillis = millis();

  /* Overloading OnReceive function and setting the Lora to receive mode */
  LoRa.onReceive(onReceive);
  LoRa_rxMode();
}


/**
 * Flow:
 * - Display:
 * Number of nodes for each animal, and the overall nodes connected
 * Wifi connection
 * Battery soc
 * if packets were received from nodes, display their nodeID and RSSI
 *
 * - Firebase:
 * The user can specify how often they want a report from the gateway to be sent
 * For each report we create a new log, a file saved on the sd card with all the
 * packets that were received in a given time slot.
 * Once it is time for a new report, we send the contents of the current file
 * to the firebase. afterwards we create a new file for the next report.
 * note: in case of no internet connection when it is time to send a
 * report, we'll continue writing to the previous file and transmit the packets
 * of both (or more) reports, when it is time to send a new report.
 */
void loop() {

  String email = USER_EMAIL;
  int atIndex = email.indexOf("@");
  String user = email.substring(0, atIndex);
  if ((WiFi.status() == WL_CONNECTED) && (Firebase.ready())) {
    updateNodeCount(&fbdo, user, &chickenCount, &pigCount, &sheepCount, &goatCount);
  }

  display.clear();
  long rssi = WiFi.RSSI();
  displayWifi(&display, rssi, (WiFi.status() != WL_CONNECTED));

  displayBattery(BL.getBatteryChargeLevel(), &display);
  displayNodeCount(&display, chickenCount, pigCount, sheepCount, goatCount);

  //print on incoming nodes
  displayPacketsBuffer(&PacketsBuffer, &display, current_log_filename);
  

  unsigned long currentMillis = millis();
  if ( ((currentMillis - previousMillis) > interval) && (WiFi.status() == WL_CONNECTED) && Firebase.ready()) {  //this means that the time interval has passed
    previousMillis = currentMillis;


    PacketInfo packet("", "", 0, 0, 0, 0, 0, 0, DUMMY_PACKET);  //dummy packet, will be filled with real data.
    Serial.printf("Reading file: \n");
    File file = SD.open(current_log_filename);
    if (!file) {
      Serial.println(current_log_filename);
      Serial.println("Failed to open file for reading");
      return;
    }
    
    while (get_packet_from_sd(file, packet)) {
      display.clear();
      long rssi = WiFi.RSSI();
      displayWifi(&display, rssi, (WiFi.status() != WL_CONNECTED));

      displayBattery(BL.getBatteryChargeLevel(), &display);
      displayNodeCount(&display, chickenCount, pigCount, sheepCount, goatCount);
      display.drawString(0, 50, "Transmitting to FireBase");
      //print
      display.display();

      if (Firebase.ready()) {
        transmitToFirebase(packet);
        
      }
      delay(200);
    }
    file.close();

    //once we send all the packets to the firebase we'd start logging on a new file
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }
    init_sdcard_log(&timeinfo, current_log_filename);
    //delay(1500);

  }
  delay(1000);

  
}

