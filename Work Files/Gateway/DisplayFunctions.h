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

//Pins for the OLED display using I2C protocol
#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_RST    16

#define FIRST_POSITION 10

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

/*
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


/**
 * Obtaining the updated number of connected nodes for each type of animal 
 * from the firebase
 * @param fbdo - firebase data object
 * @param user - the user's name as saved in the firebase
 * @param chickenCount - number of chickens that will be displayed on the screen
 * @param pigCount - number of pigs that will be displayed on the screen
 * @param sheepCount - number of sheep that will be displayed on the screen
 * @param goatCount - number of goats that will be displayed on the screen
 */
void updateNodeCount(FirebaseData* fbdo, String user, int* chickenCount, int* pigCount, int* sheepCount, int* goatCount) {
    int fbchicken = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Chicken/Number");// ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str();
    int fbpig = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Pigs/Number");
    int fbsheep = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Sheep/Number");
    int fbgoat = Firebase.getInt(*fbdo, "/test/Users/" + user + "/Data/Animal/Goat/Number");

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

#endif