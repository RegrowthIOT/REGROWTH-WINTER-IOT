#include <SPI.h>
#include <Wire.h>
#include <SSD1306.h>
#include <LoRa.h>
#include <stdint.h>
#include <WiFi.h>
#include <esp_wifi.h>

//#define LORA_BAND    433
#define LORA_BAND    868
//#define LORA_BAND    915

#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_RST    16

#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define I2C_ADDRESS_OF_SCREEN   0x3C

SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);

void displayLoraData(String countStr);
uint8_t* string_to_transmit();
char* splitString(char* transmitted,bool position);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println();
  Serial.println("Node Packet Simulation");
  Serial.println();
  
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

  delay(2000);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "Node Simulation");
  display.display();
  delay(2000);

  // Configure the LoRA radio
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(LORA_BAND * 1E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("init ok");

}

void loop() {
  static int counter = 0;

  char* transmitted_str = send_packet();

  String countStr = String(counter, DEC);
  //Serial.println(countStr);

  displayLoraData(countStr,transmitted_str);
  free(transmitted_str);
  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  counter++;
  delay(15000);
}

char* send_packet(){
  // send packet
  LoRa.beginPacket();
  char* to_transmit = (char*)string_to_transmit();
  LoRa.print(to_transmit);
  Serial.print(to_transmit);
  //LoRa.print(counter);
  LoRa.endPacket();
  return to_transmit;
}

uint8_t* string_to_transmit(){
    uint8_t* str = (uint8_t*)malloc(39);
    uint16_t grams, t;
    uint8_t h, v, s, c;

    //----------------- Device Name -----------------
    int32_t name;
    str[0] = (uint8_t)(0x41 + (rand()%26)) ; // random capital letter, 0x41 = 'A'
    str[1] = (uint8_t) 0x30 + (rand()%10) ; // random number between 0-9, 0x30 = '0'
    str[2] = 0x2C; //separator 0x2C = ;
    
    //----------------- RFID -----------------

    for (uint8_t i=1; i<11; i++) 
      str[2+i] = 0x30 + (rand()%10); // Animal's RFID_tag reading

    str[13] = 0x2C; //separator 0x2C = ;

    //----------------- Weight -----------------
    
    double val_weight = rand()%1000 ; //0 to 20 kg, in og code it's (*=100) suspected to be 1000
    grams = (uint16_t)val_weight;

    //printf("%d\n", grams);

    str[14] = (grams / 100) + 0x30;
    //printf("%x\n", str[14]);
    str[15] = ((grams - (grams / 100) * 100) / 10) + 0x30;
    //printf("%x\n", str[15]);
    str[16] = (grams - ((grams / 100) * 100) - (((grams - (grams / 100) * 100) / 10) * 10)) + 0x30;
    //printf("%x\n", str[16]);

    str[17] = 0x2C; //separator 0x2C = ;

    //----------------- Temperature -----------------
    double temp = rand()%1000;
    t = (uint16_t)temp;

    str[18] = (t / 100) + 0x30;
    str[19] = ((t - (t / 100) * 100) / 10) + 0x30;
    str[20] = (t - ((t / 100) * 100) - (((t - (t / 100) * 100) / 10) * 10)) + 0x30;

    //printf("Temp = %d%d%d\n", str[18], str[19], str[20]);

    str[21] = 0x2C; //separator 0x2C = ;
    //----------------- Humidity -----------------
    h = (uint8_t)(rand()%31);

    str[22] = (h / 10) + 0x30;
    str[23] = (h - (h / 10) * 10) + 0x30;

    str[24] = 0x2C; //separator 0x2C = ;

    //----------------- Voltage -----------------
    float vbatt =rand()%20;
    v = (uint8_t)vbatt;

    str[25] = (v / 10) + 0x30;
    str[26] = (v - (v / 10) * 10) + 0x30;

    str[27] = 0x2C; //separator 0x2C = ;

    //----------------- Humidity -----------------
    s = (uint8_t)rand()%20;

    str[28] = (s / 10) + 0x30;
    str[29] = (s - (s / 10) * 10) + 0x30;

    str[30] = 0x2C; //separator 0x2C = ;

    //----------------- Counter -----------------
    c = rand()%100000;

    str[31] = c / 10000;
    str[32] = (c - (str[31] * 10000)) / 1000;
    str[33] = ((c - (str[31] * 10000)) - (str[32] * 1000)) / 100;
    str[34] = ((c - (str[31] * 10000)) - (str[32] * 1000) - (str[33] * 100)) / 10;
    str[35] =  ((c - (str[31] * 10000)) - (str[32] * 1000) - (str[33] * 100) - (str[34] * 10));

    for (uint8_t i = 31; i < 36; i++)
    {
        str[i] += 0x30;
    }
    
    str[36] = 0x0D; //separator 0x0D = Carriage return
    str[37] = 0x0A;

    str[38] = 0;

    return str; 
}

char* splitString(char* transmitted,bool position){
  char* position_str=(char*)malloc(39);
  if(position){
    for(int i=0;i<18;i++)
    position_str[i] = transmitted[i];
  }else{
    for(int i=0;i+17<40;i++)
    position_str[i] = transmitted[17+i];
  }
  return position_str;
}

void displayLoraData(String countStr,char* transmitted) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  display.drawString(0, 20, "Sending packet: ");
  display.drawString(90, 20, countStr);
  display.drawString(0, 35, ((String)splitString(transmitted,true)));
  display.drawString(0, 45, ((String)splitString(transmitted,false)));
  display.display();
}
