# Introduction

An IoT project for livestock farming purposes, that transmits data received from nodes (sensor stations located in farms, that take measurements of animal’s weight, temperature, log date and other environmental metrics) to a central server. Our job is to build a gateway between that sensor station system and a firebase.
Features: 
Receiving and transmitting packets using LoRa (long range radio communication technique) supported hardware. 
OLED Display – Ability to display to a screen many data needed, for an example: connection status, battery status, date. 
Data Accessibility – Connecting to a firebase and sending the needed data.
Micro SD - Logs all the information received from nodes. 


## About Us

We are three students studying at the Technion - Israel Institute of Technology at the faculty of computer science, our names are:
* Dana Asfour
* Aida Mwais 
* Eitan Manor

The course that this project is a part of is 236333 - project in IOT. The course is a part of the ICST lab at the faculty.


### Dependencies and Repos Layout 

The repository is split into two main components:

* Unit tests: The code used for testing all the hardware, for every device there is a library that includes the firmware used specifically to test it. 
* Work Files: The firmware that makes the project, all the code that is flashed on the esp32 devices that manages the communication, logging packets, the display, and the transmission to the firebase end.

### Libraries Used 

| Library | By | Function |
| :---         |     :---:      |          ---: |
|SD - https://github.com/arduino-libraries/SD  | Arduino version  2.0.0    | Manages the SD memory card    |
| Battery18650Stats    |   Danilo Penotti version 1.0.0   | Library to caclculate battery change level    |
| esp8266-oled-ssd1306    |   ThingPulse version 4.3.0  | This library drives the OLED display running on the Arduino/ESP8266 & ESP32 platforms |
| LoRa |   Sandeep Mistry version 0.8.0  | An Arduino library for sending and receiving data using LoRa Radio waves   |
| esp32  |   Espressif Systems version 1.0.6   | Library that allows working with ESP32  |
| FirebaseESP32 | Mobizt version 4.3.6 | Google's Firebase Realtime Database Arduino Library for ESP32 | 





### Documentation and photos 

Can be found under: https://github.com/RegrowthIOT/REGROWTH-WINTER-IOT/blob/Master/Units%20Test/documentation.docx
