
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <time.h>
#include <WiFi.h>
//#include TODO:: add mac library for mac address
                //on TTGO - paxcounter v1.6 change pins as follows:
#define SCK  14   // 5 //ON ESP32 18
#define MISO  2   // 2 //ON ESP32 19
#define MOSI  15  // 15 //ON ESP32 23
#define CS  13    // 13 //ON ESP32 5
#define MAX_STR 10
#define FILENAME_SIZE 20 // this was determined according to this string "/+ MONTH DAY + am/pm +.txt"
#include "PacketInfo.h"

const char* ssid       = "Trio";
const char* password   = "DanaAmalAida";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 2*3600;
const int   daylightOffset_sec = 3600;


SPIClass spi = SPIClass(VSPI);
char* current_log_filename = (char*)malloc(sizeof(char)*FILENAME_SIZE);
bool am = true;

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
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("Message appended");
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

void init_sdcard_log(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  String log_name = '/'+String(asctime(&timeinfo)).substring(4,10) + (am? " am":" pm") + ".txt"; // "/+ DAY_OF_WEEK MONTH DAY + am/pm +.txt"
  
  listDir(SD, "/", 0);
  writeFile(SD, "/", log_name.c_str());
  appendFile(SD, log_name.c_str(), "Gateway Packet Log\n");
  appendFile(SD, log_name.c_str(), "Gateway Address: "); 
  appendFile(SD,log_name.c_str(),(WiFi.macAddress()+'\n').c_str());
  strcpy(current_log_filename, log_name.c_str());
  am = (am? false : true);
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void log_packet_sd(PacketInfo* new_packet){
  
  if(new_packet ->low_battery_report){
    appendFile(SD, current_log_filename, "Low Battery Report received from device:\n");
    appendFile(SD, current_log_filename, "Device: "); appendFile(SD,current_log_filename, new_packet->device_name.c_str());
    appendFile(SD, current_log_filename, "\nVoltage Reading: "); appendFile(SD,current_log_filename, String(new_packet->voltage).c_str());
    appendFile(SD, current_log_filename, "\nTime Received: "); appendFile(SD,current_log_filename,(new_packet->time_received.c_str()));
    appendFile(SD, current_log_filename, "\n");                                                                                                          
    return;
  }
  appendFile(SD, current_log_filename, "Package Received from device:\n");
  appendFile(SD, current_log_filename, "Device: "); appendFile(SD,current_log_filename, new_packet->device_name.c_str());
  appendFile(SD, current_log_filename, "\nAnimal ID: "); appendFile(SD,current_log_filename, new_packet->rfid_reading.c_str());
  appendFile(SD, current_log_filename, "\nAnimal Weight: "); appendFile(SD,current_log_filename, String(new_packet->animal_weight).c_str());
  appendFile(SD, current_log_filename, "\nTemperature: "); appendFile(SD,current_log_filename, String(new_packet->temperature).c_str());
  appendFile(SD, current_log_filename, "\nHumidity: "); appendFile(SD,current_log_filename, String(new_packet->humidity).c_str());
  appendFile(SD, current_log_filename, "\nVoltage Reading: "); appendFile(SD,current_log_filename, String(new_packet->voltage).c_str());
  appendFile(SD, current_log_filename, "\nSoC: "); appendFile(SD,current_log_filename, String(new_packet->soc).c_str());
  appendFile(SD, current_log_filename, "\nTime Received: "); appendFile(SD,current_log_filename,(new_packet->time_received.c_str()));
  appendFile(SD, current_log_filename, "\n");

}

String get_packet_parameter(String line){
  return line.substring(line.indexOf(':') + 2,line.indexOf('\n') - 1); //after the first ': ' and before the '\n'
}

bool get_packet_from_sd(File file, PacketInfo* packet_to_fill){
  
  //ADD THIS TO THE FIREBASE FUNCTION
  /*Serial.printf("Reading file: \n"); 
  File file = SD.open(current_log_filename);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }*/
  //UNTIL HERE AND THE file.close();

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
      packet_to_fill->temperature = get_packet_parameter(String(line)).toFloat();
      //Serial.println(packet_to_fill->temperature);

      line = file.readStringUntil('\n');//humidity "
      packet_to_fill->humidity = get_packet_parameter(String(line)).toFloat();
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
      return true;
    }
  }

  
}

void setup(){
  Serial.begin(115200);
  spi.begin(SCK,MISO,MOSI,CS);
  delay(200);
  if (!SD.begin(CS,spi)) {
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

  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  init_sdcard_log();
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  PacketInfo first("A0", "0000000003", 50, 25, 0.73,0.5,0.3, -53,asctime(&timeinfo));
  log_packet_sd(&first);
  PacketInfo second("A1", "0000000011", 7, 24, 0.71,0.5,0.3, -31,asctime(&timeinfo));
  log_packet_sd(&second);
  PacketInfo third("A0", "002", 52, 25, 0.73,0.5,0.3, -51,asctime(&timeinfo));
  log_packet_sd(&third);
  PacketInfo fourth("A1", "0000000000", 6, 24, 0.70,0.5,0.3, -33,asctime(&timeinfo));
  log_packet_sd(&fourth);
  PacketInfo fifth("A2", "005", 55, 24, 0.75,0.5,0.3, -65,asctime(&timeinfo));
  log_packet_sd(&fifth);
  delay(10000);
  //readFile(SD,current_log_filename);
}

void loop(){
  //ADD THIS DANA PLS
  Serial.printf("Reading file: \n"); 
  File file = SD.open(current_log_filename);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  PacketInfo packet_to_fill("A2", "005", 55, 24, 0.75,0.5,0.3, -65,asctime(&timeinfo));
  while(get_packet_from_sd(file, &packet_to_fill)){
    delay(1000);
  }
  Serial.println("finito");
  file.close();
}