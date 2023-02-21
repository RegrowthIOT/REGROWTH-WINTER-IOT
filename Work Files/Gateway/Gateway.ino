#include <FirebaseESP32.h>
#include <WiFi.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "PacketInfo.h"
#include "DisplayFunctions.h"
#include <Pangodream_18650_CL.h>
#include "GatewayLora.h"

#define I2C_ADDRESS_OF_SCREEN 0x3C

// #define WIFI_SSID "Trio"
// #define WIFI_PASSWORD "DanaAmalAida"
#define WIFI_SSID "EHABV24"
#define WIFI_PASSWORD "15011501"
// #define WIFI_SSID "Pilot2"
// #define WIFI_PASSWORD "carolteresafarah"
// #define WIFI_SSID "TechPublic"
// #define WIFI_PASSWORD ""


#define USER_EMAIL "iot.regrowth@gmail.com"
#define USER_PASSWORD "Regrowth123"



/* 2. Define the API Key */
#define API_KEY "0vh0uCSsK39x2AUAAavXKk8cRfkcGrFck3rpc6gf"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://regrowth-c498e-default-rtdb.europe-west1.firebasedatabase.app/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app


//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String main = "";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 2 * 3600;  //+2 UTC
const int daylightOffset_sec = 3600;

unsigned long previousMillis = 0;
const long interval = 43200000;  // interval at which to send data (12 hours in milliseconds)

int chickenCount = 0;
int pigCount = 0;
int sheepCount = 0;
int goatCount = 0;

std::list<PacketInfo> PacketsBuffer = std::list<PacketInfo>();
std::map<String, ANIMAL_TYPE> Nodes = std::map<String, ANIMAL_TYPE>();  //key = device_name (node)
                                                                        // data = animal_type (in node)

String current_log_filename;
SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);
Pangodream_18650_CL BL;

void onReceive(int packetSize) {
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  Serial.print("Gateway Receive: ");
  Serial.println(message);
  int rssi = LoRa.packetRssi();
  // Serial.println("NO SEG1");
  fillPacketsBuffer(&PacketsBuffer, message, rssi);
}



/************************************************************************************************************************************/

/**
 * displayBattery - show user the current percentage battery has left
 * @param percent - int from 0 to 100 to represent the battery percentage
 * @param display - pointer to the OLED screen where the message will be printed
 */
void displayBattery(uint8_t percent, SSD1306* display) {
  // display.clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);

  display->drawProgressBar(100, 22, 20, 5, percent);

  String percentStr = String(percent, DEC);
  percentStr += "%";
  display->drawString(100, 27, percentStr);
  display->display();
}

/**
 * displayPacketsBuffer - if PacketsBuffer has packets save them to SD and print them to OLED
 * @param PacketsBuffer - where we keep the packets we still in need to be printed and be put in the SD and printed
 * @param display - pointer to the OLED screen where the message will be printed
 */
void displayPacketsBuffer(std::list<PacketInfo>* PacketsBuffer, SSD1306* display, String& current_log_filename) {
  if (PacketsBuffer->size() > 0) {
    int time_on_screen = 1500;

    //print about packet received
    String line_str = "Received,Node ";
    line_str += PacketsBuffer->front().device_name;
    line_str += ",rssi ";
    line_str += String(PacketsBuffer->front().RSSI, DEC);

    //in case of "low battery" message
    if (PacketsBuffer->front().rfid_reading == BATTERY_LOW) {
      line_str = "Node ";
      line_str += PacketsBuffer->front().device_name;
      line_str += " battery low ";
      line_str += String(((int)(PacketsBuffer->front().soc * 100)), DEC);
      line_str += "%";
      time_on_screen = 4000;
    }

    display->drawString(0, 50, line_str);

    //print
    display->display();

    //delay
    delay(time_on_screen);

    //log to sd

    log_packet_sd(PacketsBuffer->front(), current_log_filename);

    //pop
    PacketsBuffer->pop_front();
  } else {
    display->drawString(0, 50, "Ready to Receive");
    display->display();
  }
}

/**
* displayWifi - a function to show on screen bars of wifi, to let the user know if there's a connection and how strong
* @param display - pointer to the OLED screen where the message will be printed
* @param rssi - the strength of the wifi signal
* @param notConnected - boolean paramater to tell us if wifi is connected at all
*/
void displayWifi(SSD1306* display, long rssi, bool notConnected) {
  // display.clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  // String rssi_str = String(rssi, DEC);
  // display.drawString(65, 0, rssi_str);

  if (notConnected) {
    if (rssi == 0) {
      display->drawString(90, 0, "Wifi Not");
      display->drawString(90, 10, "Found");
    } else {
      display->drawString(90, 0, "Not");
      display->drawString(75, 10, "Connected");
    }
  } else {
    if (rssi == 0) {
      display->drawString(90, 0, "Wifi Not");
      display->drawString(90, 10, "Found");
    } else if (rssi >= -50) {
      display->fillRect(102, 7, 4, 1);
      display->fillRect(107, 6, 4, 2);
      display->fillRect(112, 4, 4, 4);
      display->fillRect(117, 2, 4, 6);
      display->fillRect(122, 0, 4, 8);
      // display.sendBuffer();
    } else if (rssi < -50 & rssi >= -65) {
      display->fillRect(102, 7, 4, 1);
      display->fillRect(107, 6, 4, 2);
      display->fillRect(112, 4, 4, 4);
      display->fillRect(117, 2, 4, 6);
      display->drawRect(122, 0, 4, 8);
      // display.sendBuffer();
    } else if (rssi < -65 & rssi >= -75) {
      display->fillRect(102, 8, 4, 1);
      display->fillRect(107, 6, 4, 2);
      display->fillRect(112, 4, 4, 4);
      display->drawRect(117, 2, 2, 6);
      display->drawRect(122, 0, 4, 8);
      // display.sendBuffer();
    } else if (rssi < -75 & rssi >= -85) {
      display->fillRect(102, 8, 4, 1);
      display->fillRect(107, 6, 4, 2);
      display->drawRect(112, 4, 4, 4);
      display->drawRect(117, 2, 4, 6);
      display->drawRect(122, 0, 4, 8);
      // display.sendBuffer();
    } else if (rssi < -85 & rssi >= -96) {
      display->fillRect(102, 8, 4, 1);
      display->drawRect(107, 6, 4, 2);
      display->drawRect(112, 4, 4, 4);
      display->drawRect(117, 2, 4, 6);
      display->drawRect(122, 0, 4, 8);
      // display.sendBuffer();
    } else {
      display->drawRect(102, 8, 4, 1);
      display->drawRect(107, 6, 4, 2);
      display->drawRect(112, 4, 4, 4);
      display->drawRect(117, 2, 4, 6);
      display->drawRect(122, 0, 4, 8);
      // display.sendBuffer();
    }
  }


  display->display();
}

void updateNodeCount(FirebaseData* fbdo, String user, int* chickenCount, int* pigCount, int* sheepCount, int* goatCount) {
  int fbchicken = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Chicken/Number/");
  int fbpig = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Pigs/Number/");
  int fbsheep = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Sheep/Number/");
  int fbgoat = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Goat/Number/");
  *chickenCount = (fbchicken == 0 ? *chickenCount : fbchicken);
  *pigCount = (fbpig == 0 ? *pigCount : fbpig);
  *sheepCount = (fbsheep == 0 ? *sheepCount : fbsheep);
  *goatCount = (fbgoat == 0 ? *goatCount : fbgoat);
}


/**
 * displayNodeCount - Display to the screen how many nodes the user has overall and from each animal type
 * @param display - pointer to the OLED screen where the message will be printed
 * @param chickenCount - the number of chicken nodes connected to the gateway
 * @param pigCount - the number of pig nodes connected to the gateway
 * @param sheepCount - the number of sheep nodes connected to the gateway
 * @param goatCount - the number of goat nodes connected to the gateway
 *  If a farm had all 4 types of animals (chickenCount>0,...,goatCount>0), then the overall number is displayed in row 0
    chicken in row 10 (FIRST_POSITION), pig in 20, sheep in 30 and goats in 40, but if one or more of the counts is 0, than
    we won't print it at all. So this is the main functionality here: if chickenCount=0, we won't print "Chickens: #"
    all the ones after it will move one position back (pig->10, sheep->20,...) and so on.
 */
void displayNodeCount(SSD1306* display, int chickenCount, int pigCount, int sheepCount, int goatCount) {

  int next_location = FIRST_POSITION;

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  String nStr = "Con. Nodes: ";
  nStr += String((chickenCount + pigCount + sheepCount + goatCount), DEC);
  String cStr = "Chickens: ";
  cStr += String(chickenCount, DEC);
  String pStr = "Pigs: ";
  pStr += String(pigCount, DEC);
  String sStr = "Sheep: ";
  sStr += String(sheepCount, DEC);
  String gStr = "Goats: ";
  gStr += String(goatCount, DEC);

  display->drawString(0, 0, nStr);
  if (chickenCount > 0) {
    display->drawString(0, FIRST_POSITION, cStr);
    next_location += 10;
  }
  if (pigCount > 0) {
    display->drawString(0, next_location, pStr);
    next_location += 10;
  }
  if (sheepCount > 0) {
    display->drawString(0, next_location, sStr);
    next_location += 10;
  }
  if (goatCount > 0) {
    display->drawString(0, next_location, gStr);
  }
  display->display();
}

/**
 * showLogo - a function to print on screen the LORA symbol while opening the Gateway device
 * @param display - pointer to the OLED screen where the message will be printed
 */
void showLogo(SSD1306* display) {
  uint8_t x_off = (display->getWidth() - logo_width) / 2;
  uint8_t y_off = (display->getHeight() - logo_height) / 2;

  display->clear();
  display->drawXbm(x_off, y_off, logo_width, logo_height, logo_bits);
  display->display();
}

/**********************************************************************************************************************************************/


void parseTime(String str, String& date, String& time, String& timeRange) {
  int hour = 0;
  int minute = 0;
  int second = 0;
  //Mon Feb 06 18:33:13 2023
  sscanf(str.c_str(), "%*s %*s %*d %d:%d:%d %*d", &hour, &minute, &second);
  date = str.c_str() + 4;
  date = date.substring(0, 6) + " " + date.substring(15, 20);
  time = String(hour) + ":" + String(minute) + ":" + String(second);
  timeRange = (hour >= 12) ? "PM" : "AM";
}

void trasnmitToFirebase(PacketInfo& packet) {
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
  String email = USER_EMAIL;
  int atIndex = email.indexOf("@");
  String user = email.substring(0, atIndex);
  String numOfNode = packet.device_name;
  String animalType;



  if (Firebase.ready()) {
    //Serial.print("ready");
    /** adding the node to the available nodes list under the user name**/
    animalType = Firebase.getString(fbdo, "/test/Users/" + String(user) + "/Node/" + String(numOfNode) + "/") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str();
    //Serial.println(animalType);
    if (animalType == String(fbdo.errorReason().c_str())) {
      animalType = "Chicken";
    }
    Firebase.setString(fbdo, "/test/Users/Node/", "Testing non dynamic path");
    Firebase.setString(fbdo, "/test/Users/" + user + "/Node/" + numOfNode + "/", animalType);

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

    Firebase.setDouble(fbdo, "/test/Users/" + user + "/Data/Animal/" + animalType + "Number/" + animalNumber + "/" + date + "/Weight", weight);
    int activity = Firebase.getInt(fbdo, "/test/Users/" + user + "/Data/Animal" + animalType + "/Number" + animalNumber + "/" + date + "/Activity") ? fbdo.to<int>() : 0;
    if (!activity) {
      activity = 1;
    } else{
      activity++;
    }
    Firebase.setInt(fbdo, "/test/Users/" + user + "/Data/Animal/" + animalType + "/" + animalNumber + "/" + date + "/Activity", activity);
    int count = Firebase.getInt(fbdo, "/test/Users/" + user + "/Date/Animal/" + animalType + "/Number") ? fbdo.to<int>() : 0;
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

void setup() {
  Serial.begin(115200);
  delay(2000);
  /* Establishing Wifi connection */
 
  // Wifi connection isnt needed at all times 
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
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  Firebase.begin(&config, &auth);
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


  //Initianting SD card
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(200);
  if (!SD.begin(SD_CS, spi)) {
    Serial.println("Card Mount Failed");
    return;
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

  //test:
  //listDir(SD, "/", 0);

  //Serial.println(temp);
  Serial.println(current_log_filename);

  previousMillis = millis();

  /* Overloading OnReceive function and setting the Lora to receive mode */
  LoRa.onReceive(onReceive);
  LoRa_rxMode();
}

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

  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  unsigned long currentMillis = millis();
  if (((currentMillis - previousMillis) > interval) && (WiFi.status() == WL_CONNECTED) && Firebase.ready()) {  //this means that 12 hours have passed
    previousMillis = currentMillis;
    PacketInfo packet("", "", 0, 0, 0, 0, 0, 0, DUMMY_PACKET);  //fictive packet, will be filled with real data.
    //bool indicator= false;
    Serial.printf("Reading file: \n");
    File file = SD.open(current_log_filename);
    if (!file) {
      Serial.println(current_log_filename);
      Serial.println("Failed to open file for reading");
      return;
    }

    while (get_packet_from_sd(file, &packet)) {
      Serial.println("no enter");
      if (Firebase.ready()) {
        trasnmitToFirebase(packet);
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
    }
  }
  delay(1000);
}
