/**
 * Node Packages Format (each field is seperated by the number 0x2C ): [ 
 *            Weight_scale [2  + 1 byte][ 0->2 ] - Device name 1 ,
 *            data_rfid[10 + 1 byte][ 3->13 ] - current rfid reading 2(tag),
 *            //status_rfid - used for rfid (node internal),
 *            val_weight [3 + 1 byte][ 14->17 ] - current reading of the scale 3,
 *            temp [3 + 1 byte][ 18->21 ]- temperature 4,
 *            hum [2 + 1 byte][ 22->24 ]- humidity 5,
 *            vbatt [2 + 1 byte][ 25->27 ] - voltage 6,
 *            soc [2 + 1 byte][ 28->30 ]- humidity param 7,
 *            count [5  byte][ 31->35 ]- counter of reads 8 ,
 *            [36] = carriage return,
 *            [37] = 0X0A,
 *            [38 ] = 0) ]
 *
 ****** While receiving a packet OLED indicator, if it is irrelevant (random lora)
 ***** print an appropriate message as well  *************
 * AFTER RECEIVING A PACKET, LOG THE INFO ON THE SD CARD + add time stamp(tbd)
 */

/**
 * FIREBASE Format : [ "BEGIN",
 *              "ANIMAL TYPE",
 *              "AnimalID" (tag),
 *              "AnimalWeight",
 *              "Temperature",
 *              "Humidity",
 *              "END" ]
 */


#include <iostream>
#include <string> 

 
 
class PacketInfo{
public:
    String device_name;
    String rfid_reading;
    int animal_weight;
    float temperature;
    float humidity;
    double voltage;
    double soc;
    int RSSI;
};

/*std::ostream& operator<<(std::ostream &s, const PacketInfo &packet_info) {
    return s << "Packet received from the node is:" << std::endl << "(" << packet_info.device_name << ", " << packet_info.rfid_reading << packet_info.animal_weight <<  ", " 
    << packet_info.temperature << ", "  << packet_info.humidity << ", "  << packet_info.voltage << ", " << packet_info.soc << ", " 
    << packet_info.RSSI << ")";
}*/


void setup(){
  Serial.begin(9600);
  Serial.println("HELLO");
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
      array[j].concat(str[i]);
      //Serial.println(array[j]);
      i++;
    }
}

void loop() {
    PacketInfo packet_info;
    String array[8];
    String packetFormat = "ID3048265389262,some reading,123,78.2,0.67,3.3,11.3,73";
    parsePacket(packetFormat, array);
  
    packet_info.device_name=array[0];
    packet_info.rfid_reading=array[1];
    packet_info.animal_weight= array[2].toInt(); //string to int    
    packet_info.temperature=array[3].toFloat(); //string to float
    packet_info.humidity=array[4].toFloat(); //string to float
    packet_info.voltage=array[5].toDouble(); //string to double
    packet_info.soc=array[6].toDouble(); //string to double
    packet_info.RSSI=array[7].toInt();


    //std::cout << packet_info << std::endl;

    Serial.println(packet_info.device_name);
    Serial.println(packet_info.rfid_reading);
    Serial.println(packet_info.animal_weight);
    Serial.println(packet_info.temperature);
    Serial.println(packet_info.humidity);
    Serial.println(packet_info.voltage);
    Serial.println(packet_info.soc);
    Serial.println(packet_info.RSSI);
  
   //return 0;
}
