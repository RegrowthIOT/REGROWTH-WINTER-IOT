#ifndef DISPLAY_FUNCTIONS
#define DISPLAY_FUNCTIONS


  #include <SPI.h>
  #include <Wire.h>
  #include <SSD1306.h>
  #include <LoRa.h>
  #include "images.h"
  #include <WiFi.h>
  #include <esp_wifi.h>
  #include "PacketInfo.h"
  #include <list>

#define FIRST_POSITION 10
/*
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



  void displayPacketsBuffer(std::list<PacketInfo>* PacketsBuffer, SSD1306* display,fs::SDFS& SD, String& current_log_filename)
  {
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
      log_packet_sd(SD,PacketsBuffer->front(),current_log_filename);

      //pop
      PacketsBuffer->pop_front();
    }
    else
    {
      display->drawString(0, 50, "Ready to Receive");
      display->display();
    }      
  }


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

void displayNodeCount(SSD1306* display, FirebaseData* fbdo ,String user, std::list<PacketInfo>* PacketsBuffer) {
//TODO:: make sure there's a connection to the wifi and firebase before getting
  int chickenCount= Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Chicken/Number/");
  int pigCount= Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Pigs/Number/");
  int goatCount= Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Goat/Number/");
  int sheepCount= Firebase.getInt( *fbdo, "/test/Users/" + user+ "/Data/Animal/Sheep/Number/");

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

void showLogo(SSD1306* display) {
  uint8_t x_off = (display->getWidth() - logo_width) / 2;
  uint8_t y_off = (display->getHeight() - logo_height) / 2;

  display->clear();
  display->drawXbm(x_off, y_off, logo_width, logo_height, logo_bits);
  display->display();
}

*/
#endif