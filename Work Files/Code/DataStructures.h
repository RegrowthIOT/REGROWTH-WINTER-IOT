#ifndef DATA_STRUCTURES_EITAN
#define DATA_STRUCTURES_EITAN


    #include <iostream>
    #include <map>
    #include <list>
    #include <string>


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
    float temperature;
    float humidity;
    double voltage;
    double soc;
    int RSSI;
  PacketInfo(String device_name, String rfid_reading, int animal_weight, int temperature, float humidity,double voltage, double soc, int RSSI): device_name(device_name), 
  rfid_reading(rfid_reading), animal_weight(animal_weight), temperature(temperature), humidity(humidity),voltage(voltage), soc(soc), RSSI(RSSI){}
  // PacketInfo(PacketInfo& PacketInfo1): device_name(PacketInfo1.device_name),rfid_reading(PacketInfo1.rfid_reading), animal_weight(PacketInfo1.animal_weight), temperature(PacketInfo1.temperature), humidity(PacketInfo1.humidity),voltage(PacketInfo1.voltage), soc(PacketInfo1.soc), RSSI(PacketInfo1.RSSI)
  // {}
  // std::string give_device_name()
  // {
  //   return device_name;  
  // }
  // int give_RSSI()
  // {
  //   return RSSI;    
  // }
  // std::string give_rfid_reading()
  // {
  //   return rfid_reading;
  // }
  };

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

    std::map <std::string, ANIMAL_TYPE> Nodes; //key = device_name (node)
                                                // data = animal_type (in node)
    


    //int main() {
    //    std::cout << "Hello, World!" << std::endl;
    //    Nodes.insert({"AIDA",PIG});
    //    Nodes.insert({"EITAN", GOAT});
    //    Nodes.insert({"DANA",CHICKEN});
    //    if(Nodes["AIDA"]==PIG);
    //    printf("yeh\n");
    //    return 0;
    //}

#endif