#ifndef PACKET_INFO
#define PACKET_INFO

#include <iostream>
//#include <map>
#include <list>
#include <string>

#include "FS.h"
#include "SD.h"
#include <time.h>
#include <WiFi.h>

                //on TTGO - paxcounter v1.6 change pins as follows:
#define SD_SCK  14   // 14 //ON ESP32 18
#define SD_MISO  2   // 2  //ON ESP32 19
#define SD_MOSI  15  // 15 //ON ESP32 23
#define SD_CS  13    // 13 //ON ESP32 5
#define MAX_STR 10
//#define FILENAME_SIZE 20 // this was determined according to this string "/+ MONTH DAY + am/pm +.txt"
#define DUMMY_PACKET "DUMMY"
#define BATTERY_LOW "0000000000"

SPIClass spi = SPIClass();

/**
 * a variable to indicate what type of animal the node is tracking
 */
typedef enum{
  CHICKEN,
  PIG,
  SHEEP,
  GOAT
} ANIMAL_TYPE;

/**
 * the variable we use to represent a packet and all the information transferred within it:
 * the node name, the number for the specific animal, its weight, the temperature recorded, the humidity,
 * the voltage of the node, its charge percentage (SOC) and the strength of the signal for packet received (RSSI)
*/
class PacketInfo{
public:
    String device_name;
    String rfid_reading;
    double animal_weight;
    double temperature;
    double humidity;
    double voltage;
    double soc;
    int RSSI;
	  String time_received;
	  bool low_battery_report=false;
	
	PacketInfo(String device_name_r, String rfid_reading_r, double animal_weight_r, double temperature_r, double humidity_r,double voltage_r, double soc_r, int RSSI_r, String time_received_r)
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

  /**
   * Printing the contents of the packets to the serial monitor
   */
  void PrintPacket(){
    Serial.println( "Device: " + device_name
                     +" ,Animal ID: "+ rfid_reading
                     +" ,Animal Weight: "+ String(animal_weight) 
                     +" ,Temperature: "+ String(temperature)
                     +" ,Humidity: " + String(humidity) 
                     +"\nVoltage Reading: " + String(voltage) 
                     +" ,SoC: " + String(soc)
                     +" ,RSSI:" + String(RSSI)
                     +" ,Time Received: " + time_received 
                      );
  }
  
};

/**
 * Lists the contents of a directory into the serial port
 * @param fs - a class for the file system, in our case it is the SD fs
 * @param dirname - name of directory
 * @param levels - how many levels into the directory we would to read
 */
void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
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

/**
 * Writing a message into a target file using path
 * @param fs - a class for the file system, in our case it is the SD fs
 * @param path - an absolute path for the target file
 * @param message - the message to be written into the file
 */
void writeFile(fs::FS& fs, const char* path, const char* message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (!file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

/**
 * Appending a message into a target file using path
 * @param fs - a class for the file system, in our case it is the SD fs
 * @param path - an absolute path for the target file
 * @param message - the message to be written into the file
 */
void appendFile(fs::FS& fs, const char* path, const char* message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

/**
 * Deleting a target file from the file system
 * @param fs - a class for the file system, in our case it is the SD fs
 * @param path - an absolute path for the target file
 */
void deleteFile(fs::FS& fs, const char* path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

/**
 * Specified whether the time, whether it is AM or PM
 * @param time - the current time
 * @return - am or pm based on the current time
 */
String getTime(String time) {
  int hour = time.substring(11, 13).toInt();
  return (hour < 12) ? " am" : " pm";
}

/**
 * Creates a new file to log incoming packets onto.
 * The new file is named based on the time of day in the following format
 * "DAY_OF_WEEK MONTH DAY am/pm.txt"
 * @param timeinfo - a struct containing the current time
 * @param filename - a return variable which will contain the name of the file
 * that was created.
 *
 * note: if there already is a file with the same name, we will not create
 * another one.
 */
void init_sdcard_log(struct tm* timeinfo, String& filename) {
  String parsed_time = String(asctime(timeinfo));
  //for the sake of the presentation we changed the filename to include the time as well so we'll send it every few minutes 
  // it was parsed_time.substring(4,10)
  String log_name = '/' + parsed_time.substring(4, 10) +  getTime(parsed_time) + ".txt";  // "/+ DAY_OF_WEEK MONTH DAY + am/pm +.txt" 
  listDir(SD, "/", 0);

  if (!SD.exists(log_name)) {
    writeFile(SD, "/", log_name.c_str());
    appendFile(SD, log_name.c_str(), "Gateway Packet Log\nGateway Address: ");
    appendFile(SD, log_name.c_str(), (WiFi.macAddress() + "\n").c_str());
  }

  filename = log_name;

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

/**
 * Logging a new packet into the current log file
 * @param new_packet - a class containing the information of the packet
 * @param current_log_filename - the name of the file logging the packets
 */
void log_packet_sd(PacketInfo& new_packet, String& current_log_filename) {

  Serial.println("Appending to file:" + current_log_filename);
  if (new_packet.low_battery_report) {

    appendFile(SD, current_log_filename.c_str(), ( "Low Battery Report received from device:\nDevice: " + new_packet.device_name
                                                   +"\nVoltage Reading: " + String(new_packet.voltage) 
                                                   +"\nTime Received: " + new_packet.time_received +"\n").c_str() );
    Serial.printf("Append Success\n");

  } else {
    appendFile(SD, current_log_filename.c_str(), ( "Package Received from device:\nDevice: " + new_packet.device_name
                                                    +"\nAnimal ID: "+ new_packet.rfid_reading
                                                    +"\nAnimal Weight: "+ String(new_packet.animal_weight) 
                                                    +"\nTemperature: "+ String(new_packet.temperature)
                                                    +"\nHumidity: " + String(new_packet.humidity) 
                                                    +"\nVoltage Reading: " + String(new_packet.voltage) 
                                                    +"\nSoC: " + String(new_packet.soc)
                                                    +"\nTime Received: " + new_packet.time_received +"\n").c_str());

    Serial.printf("Append Success\n");
  }
}

String get_packet_parameter(String line) {
  return line.substring(line.indexOf(':') + 2, line.indexOf('\n') - 1);  //after the first ': ' and before the '\n'
}

/**
 * Reading the contents of the log file into a packetInfo struct, to be sent
 * to the firebase
 * @param file - the log file
 * @param packet_to_fill - packetInfo to be filed with the data from the file
 * @return
 * true - in case we filled the packet_to_fill with data to be transmitted
 * false - in case of EOF
 *
 * note: the file is opened outside of the function so we can get the packets
 * one at a time without resetting the seek pointer.
 */
bool get_packet_from_sd(File file, PacketInfo& packet_to_fill) {

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line == "Package Received from device:") {
      line = file.readStringUntil('\n');  //"Device: "
      packet_to_fill.device_name = get_packet_parameter(String(line));

      line = file.readStringUntil('\n');  //"Animal ID: "
      packet_to_fill.rfid_reading = get_packet_parameter(String(line));

      line = file.readStringUntil('\n');  //Animal Weight: "
      packet_to_fill.animal_weight = get_packet_parameter(String(line)).toDouble();

      line = file.readStringUntil('\n');  //Temperature: "
      packet_to_fill.temperature = get_packet_parameter(String(line)).toDouble();

      line = file.readStringUntil('\n');  //humidity "
      packet_to_fill.humidity = get_packet_parameter(String(line)).toDouble();

      line = file.readStringUntil('\n');  //Voltage Reading: "
      packet_to_fill.voltage = get_packet_parameter(String(line)).toDouble();

      line = file.readStringUntil('\n');  //"SoC: "
      packet_to_fill.soc = get_packet_parameter(String(line)).toDouble();

      line = file.readStringUntil('\n');  //"Time Received: "
      packet_to_fill.time_received = get_packet_parameter(String(line));
      
      line = file.readStringUntil('\n');  //"\n"
      
      return true;
    } else if (line == "Low Battery Report received from device:") {
      
      line = file.readStringUntil('\n');  //"Device: "
      packet_to_fill.device_name = get_packet_parameter(String(line));
      
      line = file.readStringUntil('\n');  //Voltage Reading: "
      packet_to_fill.voltage = get_packet_parameter(String(line)).toDouble();
      
      line = file.readStringUntil('\n');  //"Time Received: "
      packet_to_fill.time_received = get_packet_parameter(String(line));
      
      packet_to_fill.rfid_reading = BATTERY_LOW;
      line = file.readStringUntil('\n');  //"\n"
      
      return true;
    }
  }

  return false;
}

/**
 * fillPacketsBuffer - a function which takes an incoming packet (as string) and puts it (creating a new PacketInfo)
 * in the packetBuffer for it to be printed and then saved to the sd card
 * @param packetBuffer - where we keep the packets we still in need to be printed and be put in the SD and printed
 * @param incomingPacket - the packet received as one string
 */
void fillPacketsBuffer(std::list<PacketInfo>* packetBuffer, String& incomingPacket, int rssi) {
    
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("Saving new packet in display list ");
  PacketInfo new_packet(incomingPacket.substring(0, 2), incomingPacket.substring(3, 13), (incomingPacket.substring(14, 17)).toDouble() / 100, (incomingPacket.substring(18, 21)).toDouble() / 10, (incomingPacket.substring(22, 24)).toDouble(), (incomingPacket.substring(25, 27)).toDouble() / 10, (incomingPacket.substring(28, 30)).toDouble(), rssi, String(asctime(&timeinfo)));
  packetBuffer->push_back(new_packet);
  new_packet.PrintPacket();
  Serial.println("Saving Success");
}


#endif