#include <FirebaseESP32.h>
#include <WiFi.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "PacketInfo.h"
#include "DisplayFunctions.h"
#include <Pangodream_18650_CL.h>
#include "GatewayLora.h"

#define I2C_ADDRESS_OF_SCREEN   0x3C

#define WIFI_SSID "Trio"
#define WIFI_PASSWORD "DanaAmalAida"
// #define WIFI_SSID "Pilot2"
// #define WIFI_PASSWORD "carolteresafarah"
// #define WIFI_SSID "TechPublic"
// #define WIFI_PASSWORD ""

// #define WIFI_SSID "Dana(phone)"
// #define WIFI_PASSWORD "dana3004"

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

int chickenCount = 0;
int pigCount = 0;
int sheepCount = 0;
int goatCount = 0;

std::list <PacketInfo> PacketsBuffer = std::list <PacketInfo>();
std::map <String, ANIMAL_TYPE> Nodes = std::map <String, ANIMAL_TYPE>(); //key = device_name (node)
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
  int rssi= LoRa.packetRssi();
  Serial.println("NO SEG1");
  fillPacketsBuffer(&PacketsBuffer ,message, rssi);
}

/* ----------------------------------------------------------------------------------------------------------------------------------*/

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 2*3600; //+2 UTC
const int   daylightOffset_sec = 3600;


SPIClass spi = SPIClass();

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND); 
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

String is_am(String time) {
  int hour = time.substring(11,12).toInt() ;
  return (hour < 12) ? " am" : " pm";
}

void init_sdcard_log(struct tm* timeinfo,String& filename){
  String parsed_time = String(asctime(timeinfo));
  String log_name = '/'+parsed_time.substring(4,10) + is_am(parsed_time) + ".txt"; // "/+ DAY_OF_WEEK MONTH DAY + am/pm +.txt"
  listDir(SD, "/", 0);
  
  if( !SD.exists(log_name) ){
    writeFile(SD, "/", log_name.c_str());
    appendFile(SD, log_name.c_str(), "Gateway Packet Log\n");
    appendFile(SD, log_name.c_str(), "Gateway Address: "); 
    appendFile(SD,log_name.c_str(),(WiFi.macAddress() +"\n").c_str());
  }

  filename = log_name;

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void log_packet_sd(PacketInfo& new_packet, String& current_log_filename){

  Serial.printf("Appending to file: %s\n", current_log_filename);
  if(new_packet.low_battery_report){
    appendFile(SD, current_log_filename.c_str(), "Low Battery Report received from device:\n");
    appendFile(SD, current_log_filename.c_str(), "Device: "); 
    appendFile(SD, current_log_filename.c_str(), new_packet.device_name.c_str());
    appendFile(SD, current_log_filename.c_str(), "\nVoltage Reading: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.voltage).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nTime Received: "); 
    appendFile(SD, current_log_filename.c_str(),(new_packet.time_received.c_str()));
    appendFile(SD, current_log_filename.c_str(), "\n");  
    Serial.printf("Append Success\n");                                                                                                        

  }
  else{
    appendFile(SD, current_log_filename.c_str(), "Package Received from device:\n");
    appendFile(SD, current_log_filename.c_str(), "Device: "); 
    appendFile(SD, current_log_filename.c_str(), new_packet.device_name.c_str());
    appendFile(SD, current_log_filename.c_str(), "\nAnimal ID: "); 
    appendFile(SD, current_log_filename.c_str(), new_packet.rfid_reading.c_str());
    appendFile(SD, current_log_filename.c_str(), "\nAnimal Weight: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.animal_weight).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nTemperature: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.temperature).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nHumidity: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.humidity).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nVoltage Reading: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.voltage).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nSoC: "); 
    appendFile(SD, current_log_filename.c_str(), String(new_packet.soc).c_str());
    appendFile(SD, current_log_filename.c_str(), "\nTime Received: "); 
    appendFile(SD, current_log_filename.c_str(),(new_packet.time_received.c_str()));
    appendFile(SD, current_log_filename.c_str(), "\n");
    Serial.printf("Append Success\n"); 
  }


}

String get_packet_parameter(String line){
  return line.substring(line.indexOf(':') + 2,line.indexOf('\n') - 1); //after the first ': ' and before the '\n'
}

bool get_packet_from_sd(File file, PacketInfo* packet_to_fill){

  while( file.available() ){
    String line = file.readStringUntil('\n');
    //Serial.println(line);
    if(line == "Package Received from device:"){
      line = file.readStringUntil('\n'); //"Device: "
      packet_to_fill->device_name = get_packet_parameter(String(line));
      //Serial.println(packet_to_fill->device_name);

      line = file.readStringUntil('\n'); //"Animal ID: "
      packet_to_fill->rfid_reading = get_packet_parameter(String(line));
      //Serial.println(packet_to_fill->rfid_reading);

      line = file.readStringUntil('\n');//Animal Weight: "
      packet_to_fill->animal_weight = get_packet_parameter(String(line)).toInt();
      //Serial.println(packet_to_fill->animal_weight);

      line = file.readStringUntil('\n');//Temperature: "
      packet_to_fill->temperature = get_packet_parameter(String(line)).toDouble();
      //Serial.println(packet_to_fill->temperature);

      line = file.readStringUntil('\n');//humidity "
      packet_to_fill->humidity = get_packet_parameter(String(line)).toDouble();
      //Serial.println(packet_to_fill->humidity);

      line = file.readStringUntil('\n');//Voltage Reading: "
      packet_to_fill->voltage = get_packet_parameter(String(line)).toDouble();
      //Serial.println(packet_to_fill->voltage);

      line = file.readStringUntil('\n');//"SoC: "
      packet_to_fill->soc = get_packet_parameter(String(line)).toDouble();
      //Serial.println(packet_to_fill->soc);

      line = file.readStringUntil('\n');//"Time Received: "
      packet_to_fill->time_received = get_packet_parameter(String(line));
      //Serial.println(packet_to_fill->time_received);

      line = file.readStringUntil('\n');//"\n"
      //file.close();
      return true;
    }
    else if(line == "Low Battery Report received from device:"){
            
      line = file.readStringUntil('\n'); //"Device: "
      packet_to_fill->device_name = get_packet_parameter(String(line));
      //Serial.println(packet_to_fill->device_name);

      line = file.readStringUntil('\n');//Voltage Reading: "
      packet_to_fill->voltage = get_packet_parameter(String(line)).toDouble();
      //Serial.println(packet_to_fill->voltage);

      line = file.readStringUntil('\n');//"Time Received: "
      packet_to_fill->time_received = get_packet_parameter(String(line));
      //Serial.println(packet_to_fill->time_received);

      packet_to_fill -> rfid_reading = BATTERY_LOW;

      line = file.readStringUntil('\n');//"\n"
      //file.close();
      return true;
    }
  }
  return false;
}

/**
 *  fillPacketsBuffer - a function which takes an incoming packet (as string) and puts it (creating a new PacketInfo)
 *  in the packetBuffer for it to be printed and then saved to the sd card
 * @param packetBuffer - where we keep the packets we still in need to be printed and be put in the SD and printed
 * @param incomingPacket - the packet received as one string
 */
void fillPacketsBuffer(std::list<PacketInfo>* packetBuffer, String& incomingPacket, int rssi)
{

  // Serial.println(incomingPacket.substring(0,2));
  // Serial.println(incomingPacket.substring(3,13));
  // Serial.println((incomingPacket.substring(14,17)).toInt());
  // Serial.println((incomingPacket.substring(18,21)).toFloat());
  // Serial.println((incomingPacket.substring(22,24)).toFloat());
  // Serial.println((incomingPacket.substring(25,27)).toDouble());
  // Serial.println((incomingPacket.substring(28,30)).toDouble());
  // Serial.println(rssi);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println(asctime(&timeinfo));

 // Serial.println("no seg here1");
  PacketInfo new_packet(incomingPacket.substring(0,2),incomingPacket.substring(3,13),  (incomingPacket.substring(14,17)).toInt()/100 , (incomingPacket.substring(18,21)).toDouble()/10 , (incomingPacket.substring(22,24)).toDouble(),(incomingPacket.substring(25,27)).toDouble()/10, (incomingPacket.substring(28,30)).toDouble(), rssi, String(asctime(&timeinfo)));
  //PacketInfo new_packet("A0", "003", 50, 25, 0.73,0.5,0.3, -53,"Mon Feb 06 ");
  packetBuffer->push_back(new_packet);

  //Serial.println("no seg here2");

}



/*--------------------------------------------------------------------------------------------------------------------------------------*/





/************************************************************************************************************************************/

/**
 * displayBattery - show user the current percentage battery has left
 * @param percent - int from 0 to 100 to represent the battery percentage
 * @param display - pointer to the OLED screen where the message will be printed
 */
void displayBattery(uint8_t percent,SSD1306* display) {
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
void displayPacketsBuffer(std::list<PacketInfo>* PacketsBuffer, SSD1306* display, String& current_log_filename){
    if(PacketsBuffer->size()>0)
    {
      int time_on_screen = 1500;

      //print about packet received
      String line_str = "Received,Node ";
      line_str+= PacketsBuffer->front().device_name;
      line_str+= ",rssi ";
      line_str+= String(PacketsBuffer->front().RSSI, DEC);
      
      //in case of "low battery" message
      if(PacketsBuffer->front().rfid_reading == BATTERY_LOW)
      {
        line_str = "Node ";
        line_str+= PacketsBuffer->front().device_name;
        line_str+= " battery low ";
        line_str+= String(((int)(PacketsBuffer->front().soc*100)), DEC);
        line_str+= "%";        
        time_on_screen = 4000;
      }
      
      display->drawString(0, 50, line_str);

      //print
      display->display();
      
      //delay
      delay(time_on_screen);

      //log to sd

      log_packet_sd(PacketsBuffer->front(),current_log_filename);

      //pop
      PacketsBuffer->pop_front();
    }
    else
    {
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
void displayWifi(SSD1306* display, long rssi,bool notConnected) {
  // display.clear();
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  // String rssi_str = String(rssi, DEC);
  // display.drawString(65, 0, rssi_str);

  if(notConnected){
    if(rssi == 0){
      display->drawString(90, 0, "Wifi Not");
      display->drawString(90, 10, "Found");
    }else{
      display->drawString(90, 0, "Not");
      display->drawString(75, 10, "Connected"); 
    }    
  }else{
    if(rssi == 0){
      display->drawString(90, 0, "Wifi Not");
      display->drawString(90, 10, "Found");    
    }else if (rssi >= -50) { 
      display->fillRect(102,7,4,1);
      display->fillRect(107,6,4,2);
      display->fillRect(112,4,4,4);
      display->fillRect(117,2,4,6);
      display->fillRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -50 & rssi >= -65) {
      display->fillRect(102,7,4,1);
      display->fillRect(107,6,4,2);
      display->fillRect(112,4,4,4);
      display->fillRect(117,2,4,6);
      display->drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -65 & rssi >= -75) {
      display->fillRect(102,8,4,1);
      display->fillRect(107,6,4,2);
      display->fillRect(112,4,4,4);
      display->drawRect(117,2,2,6);
      display->drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -75 & rssi >= -85) {
      display->fillRect(102,8,4,1);
      display->fillRect(107,6,4,2);
      display->drawRect(112,4,4,4);
      display->drawRect(117,2,4,6);
      display->drawRect(122,0,4,8);
      // display.sendBuffer();
    } else if (rssi < -85 & rssi >= -96) {
      display->fillRect(102,8,4,1);
      display->drawRect(107,6,4,2);
      display->drawRect(112,4,4,4);
      display->drawRect(117,2,4,6);
      display->drawRect(122,0,4,8);
      // display.sendBuffer();
    } else {
      display->drawRect(102,8,4,1);
      display->drawRect(107,6,4,2);
      display->drawRect(112,4,4,4);
      display->drawRect(117,2,4,6);
      display->drawRect(122,0,4,8);
      // display.sendBuffer();
    }
  }


  display->display();
}

void updateNodeCount(FirebaseData* fbdo ,String user,int* chickenCount, int* pigCount, int* sheepCount, int* goatCount)
{
  int fbchicken = Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Chicken/Number/");
  int fbpig = Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Pigs/Number/");
  int fbsheep = Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Sheep/Number/");
  int fbgoat = Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Goat/Number/");  
  *chickenCount = ( fbchicken == 0 ? *chickenCount : fbchicken );
  *pigCount = ( fbpig == 0 ? *pigCount : fbpig );
  *sheepCount = ( fbsheep == 0 ? *sheepCount : fbsheep );
  *goatCount = ( fbgoat == 0 ?  *goatCount : fbgoat);
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
void displayNodeCount(SSD1306* display,int chickenCount, int pigCount, int sheepCount, int goatCount) {

  int next_location = FIRST_POSITION;

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  String nStr = "Con. Nodes: ";
  nStr += String((chickenCount+pigCount+sheepCount+goatCount), DEC);
  String cStr = "Chickens: ";
  cStr += String(chickenCount, DEC);
  String pStr = "Pigs: ";
  pStr += String(pigCount, DEC);
  String sStr = "Sheep: ";
  sStr += String(sheepCount, DEC); 
  String gStr = "Goats: ";
  gStr += String(goatCount, DEC);  
 
  display->drawString(0, 0, nStr);
  if(chickenCount > 0)
  {    
    display->drawString(0, FIRST_POSITION, cStr);
    next_location += 10;
  }
  if(pigCount > 0){
    display->drawString(0, next_location, pStr);
    next_location += 10;
  }    
  if(sheepCount > 0){
    display->drawString(0, next_location, sStr);
    next_location += 10;
  }
  if(goatCount > 0){
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
    else {
      activity++;
    }
    Firebase.setFloat(fbdo, "/test/Users/" +user+ "/Data/Animal/" + animalType+ "/"+ animalNumber + "/" + date +"/Activity", activity);
    int count= Firebase.getInt(fbdo, "/test/Users/"+user+ "/Date/Animal/" + animalType + "/Number") ? fbdo.to<int>() :0 ;///FOR DISPLAY HERE USE TODO::/
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

void setup()
{
  Serial.begin(115200);
  delay(2000);
  /* Establishing Wifi connection */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

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

  /* configuring time according to timezone based on UTC */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  /* Lora device set-up */

  // Configure the LoRA radio
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(LORA_BAND*1E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  
  /* Creating a log on sd*/
  init_sdcard_log(&timeinfo,current_log_filename);

  //test:
  //listDir(SD, "/", 0);
  
  //Serial.println(temp);
  Serial.println(current_log_filename);

  previousMillis = millis();
  
  /* Overloading OnReceive function and setting the Lora to receive mode */
  LoRa.onReceive(onReceive);
  LoRa_rxMode();

}

void loop()
{  
    
  String email= USER_EMAIL; 
  int atIndex = email.indexOf("@");
  String user = email.substring(0, atIndex);
  if( (WiFi.status()== WL_CONNECTED) && (Firebase.ready()) ){
    updateNodeCount(&fbdo, user, &chickenCount, &pigCount, &sheepCount, &goatCount);
  }

  display.clear();
  long rssi = WiFi.RSSI();
  displayWifi(&display,rssi,(WiFi.status()!= WL_CONNECTED));

  displayBattery(BL.getBatteryChargeLevel(),&display);
  displayNodeCount(&display,chickenCount,pigCount,sheepCount,goatCount);

  //print on incoming nodes
  displayPacketsBuffer(&PacketsBuffer,&display,current_log_filename);
  
  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  unsigned long currentMillis = millis();
  if ( ( (currentMillis - previousMillis) > interval) && (WiFi.status()== WL_CONNECTED) ) { //this means that 12 hours have passed 
    previousMillis = currentMillis;
    PacketInfo packet("","",0,0,0,0,0,0,DUMMY_PACKET); //fictive packet, will be filled with real data.
    //bool indicator= false;
    Serial.printf("Reading file: \n"); 
    File file = SD.open(current_log_filename);
    if(!file){
      Serial.println(current_log_filename);
      Serial.println("Failed to open file for reading");
      return;
    }

    while (get_packet_from_sd(file,&packet)){
      Serial.println("no enter");
      if (Firebase.ready()) {
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
    }
  }
  delay(1000);
}

