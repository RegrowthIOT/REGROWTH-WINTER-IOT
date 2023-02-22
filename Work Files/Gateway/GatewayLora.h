//#define LORA_BAND    433
#define LORA_BAND    868
//#define LORA_BAND    915

//Pins for the Lora radio using SPI protocol
#define LORA_SCK     5   
#define LORA_MISO    19   
#define LORA_MOSI    27   
#define LORA_SS      18   
#define LORA_RST     23  
#define LORA_DI0     26   

void LoRa_rxMode() {
  LoRa.disableInvertIQ(); //normal mode
  LoRa.receive(); //set lora buffer on receive
}

