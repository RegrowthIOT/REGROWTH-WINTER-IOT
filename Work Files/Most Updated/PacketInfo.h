#ifndef PACKET_INFO
#define PACKET_INFO

#include <iostream>
#include <map>
#include <list>
#include <string>

#include "FS.h"
#include "SD.h"
#include <time.h>
#include <WiFi.h>

                //on TTGO - paxcounter v1.6 change pins as follows:
#define SD_SCK  14   // 5 //ON ESP32 18
#define SD_MISO  2   // 2 //ON ESP32 19
#define SD_MOSI  15  // 15 //ON ESP32 23
#define SD_CS  13    // 13 //ON ESP32 5
#define MAX_STR 10
#define FILENAME_SIZE 20 // this was determined according to this string "/+ MONTH DAY + am/pm +.txt"



const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 2*3600; //+2 UTC
const int   daylightOffset_sec = 3600;


SPIClass spi = SPIClass(VSPI);
bool am = true;
String packet_array[8]; 

#define BATTERY_LOW "0000000000"

  typedef enum{
    CHICKEN,
    PIG,
    SHEEP,
    GOAT
  } ANIMAL_TYPE;


class PacketInfo{
public:
    String device_name;
    String rfid_reading;
    int animal_weight;
    double temperature;
    double humidity;
    double voltage;
    double soc;
    int RSSI;
	  String time_received;
	  bool low_battery_report=false;
	
	PacketInfo(String device_name_r, String rfid_reading_r, int animal_weight_r, double temperature_r, double humidity_r,double voltage_r, double soc_r, int RSSI_r, String time_received_r)
	{
    this->animal_weight= animal_weight_r;
    this->temperature=temperature_r;
    this->humidity=humidity_r;
    this->voltage=voltage_r;
    this->soc=soc_r;
    this->RSSI=RSSI_r;
    this->device_name = device_name_r;
    this->rfid_reading = rfid_reading_r;
    this->time_received = time_received_r;
		if(rfid_reading == BATTERY_LOW){
			low_battery_report = true;
		}
		else
			low_battery_report = false;
	}

  PacketInfo(const PacketInfo &t)
  {
    device_name = t.device_name;
    rfid_reading = t.rfid_reading;
    animal_weight = t.animal_weight;
    temperature = t.temperature;
    humidity = t.humidity;
    voltage = t.voltage;
    soc = t.soc;
    RSSI = t.RSSI;
	  time_received = t.time_received;
	  low_battery_report= t.low_battery_report;
  }
};

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

void init_sdcard_log(struct tm* timeinfo, char** current_log_filename ){

  String log_name = '/'+String(asctime(timeinfo)).substring(4,10) + (am? " am":" pm") + ".txt"; // "/+ DAY_OF_WEEK MONTH DAY + am/pm +.txt"
  //TODO:: check if file already exists -> 
  listDir(SD, "/", 0);
  writeFile(SD, "/", log_name.c_str());
  appendFile(SD, log_name.c_str(), "Gateway Packet Log\n");
  appendFile(SD, log_name.c_str(), "Gateway Address: "); 
  appendFile(SD,log_name.c_str(),(WiFi.macAddress() +"\n").c_str());
  strcpy(*current_log_filename, log_name.c_str());
  am = (am? false : true);//TODO:: updTE Based on time of day
  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void log_packet_sd(PacketInfo* new_packet, char** current_log_filename){
  
  //current_log_filename++;
  Serial.println("FILENAME:");
  Serial.println(*current_log_filename);
  if(new_packet ->low_battery_report){
    appendFile(SD, *current_log_filename, "Low Battery Report received from device:\n");
    appendFile(SD, *current_log_filename, "Device: "); 
    appendFile(SD,*current_log_filename, new_packet->device_name.c_str());
    appendFile(SD, *current_log_filename, "\nVoltage Reading: "); 
    appendFile(SD,*current_log_filename, String(new_packet->voltage).c_str());
    appendFile(SD, *current_log_filename, "\nTime Received: "); 
    appendFile(SD,*current_log_filename,(new_packet->time_received.c_str()));
    appendFile(SD, *current_log_filename, "\n");                                                                                                          
    return;
  }
  appendFile(SD, *current_log_filename, "Package Received from device:\n");
  appendFile(SD,*current_log_filename, "Device: "); 
  appendFile(SD,*current_log_filename, new_packet->device_name.c_str());
  appendFile(SD, *current_log_filename, "\nAnimal ID: "); 
  appendFile(SD,*current_log_filename, new_packet->rfid_reading.c_str());
  appendFile(SD, *current_log_filename, "\nAnimal Weight: "); 
  appendFile(SD,*current_log_filename, String(new_packet->animal_weight).c_str());
  appendFile(SD, *current_log_filename, "\nTemperature: "); 
  appendFile(SD,*current_log_filename, String(new_packet->temperature).c_str());
  appendFile(SD, *current_log_filename, "\nHumidity: "); 
  appendFile(SD,*current_log_filename, String(new_packet->humidity).c_str());
  appendFile(SD, *current_log_filename, "\nVoltage Reading: "); 
  appendFile(SD,*current_log_filename, String(new_packet->voltage).c_str());
  appendFile(SD, *current_log_filename, "\nSoC: "); 
  appendFile(SD,*current_log_filename, String(new_packet->soc).c_str());
  appendFile(SD, *current_log_filename, "\nTime Received: "); 
  appendFile(SD,*current_log_filename,(new_packet->time_received.c_str()));
  appendFile(SD, *current_log_filename, "\n");

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


void parsePacket (const String& str, String array[8]) {
    String element;
    int i = 0;
    int j=0;
    while (str[i] != '\0'){
      if(str[i] == ',') {
        j+=1;
        i++;
      }
      else{
        array[j]+=str[i];
        i++;
      }
    }
}

void fillPacketsBuffer(std::list<PacketInfo>* packetBuffer, String& incomingPacket, int rssi, char** current_log_filename)
{
   parsePacket(incomingPacket, packet_array);
   Serial.println(*current_log_filename);
  Serial.println(incomingPacket.substring(0,2));
  Serial.println(incomingPacket.substring(3,13));
Serial.println((incomingPacket.substring(14,17)).toInt());
Serial.println((incomingPacket.substring(18,21)).toFloat());
Serial.println((incomingPacket.substring(22,24)).toFloat());
Serial.println((incomingPacket.substring(25,27)).toDouble());
Serial.println((incomingPacket.substring(28,30)).toDouble());
Serial.println(rssi);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println(asctime(&timeinfo));

  Serial.println("no seg here1");
  PacketInfo new_packet(incomingPacket.substring(0,2),incomingPacket.substring(3,13), (incomingPacket.substring(14,17)).toInt(), (incomingPacket.substring(18,21)).toDouble(), (incomingPacket.substring(22,24)).toDouble(),(incomingPacket.substring(25,27)).toDouble(), (incomingPacket.substring(28,30)).toDouble(), rssi, String(asctime(&timeinfo)));
  //PacketInfo new_packet("A0", "003", 50, 25, 0.73,0.5,0.3, -53,"Mon Feb 06 ");
  packetBuffer->push_back(new_packet);
//   //packetBuffer->push_back(PacketInfo("A0", "003", 50, 25, 0.73,0.5,0.3, -53,"Mon Feb 06 "));
//   for (auto it = packetBuffer->begin(); it != packetBuffer->end(); it++) {
//     Serial.println("Device name: " + it->device_name);
// }
  Serial.println("no seg here2");
  log_packet_sd(&new_packet,current_log_filename);
  Serial.println("no seg here3");

}



#endif