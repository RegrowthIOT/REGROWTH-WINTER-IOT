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


  void displayPacketsBuffer(std::list<PacketInfo>* PacketsBuffer, SSD1306* display)
  {
    if(PacketsBuffer->size()>0)
    {
      //print about packet received
      String line1_str = "Node: ";
      line1_str+= PacketsBuffer->front().device_name;
      line1_str+= ", packet received";
      String line2_str = "RSSI: ";
      line2_str+= String(PacketsBuffer->front().RSSI, DEC);
      display->drawString(0, 40, line1_str);
      display->drawString(0, 50, line2_str);

      //print
      display->display();
      
      //delay
      delay(1500);

      //pop
      PacketsBuffer->pop_front();
    }
    else
    {
      display->drawString(0, 40, "Ready to Receive");
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

void displayNodeCount(SSD1306* display, int chickenCount, int pigCount, int sheepCount) {
  // display.clear();
  int location_pig = 10;
  int location_sheep = 10;
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  String nStr = "Con. Nodes: ";
  nStr += String((chickenCount+pigCount+sheepCount), DEC);
  String cStr = "Chickens: ";
  cStr += String(chickenCount, DEC);
  String pStr = "Pigs: ";
  pStr += String(pigCount, DEC);
  String sStr = "Sheep: ";
  sStr += String(sheepCount, DEC);  
  display->drawString(0, 0, nStr);
  if(chickenCount > 0)
  {    
    display->drawString(0, 10, cStr);
    location_pig = 20;
    location_sheep = 20;
  }
  if(pigCount > 0){
    display->drawString(0, location_pig, pStr);
    location_sheep = location_pig + 10;

  }    
  if(sheepCount > 0){
    display->drawString(0, location_sheep, sStr); 
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


#endif