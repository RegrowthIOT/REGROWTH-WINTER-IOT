//#define LORA_BAND    433
#define LORA_BAND    868
//#define LORA_BAND    915

#define OLED_SDA    21
#define OLED_SCL    22
#define OLED_RST    16

#define LORA_SCK     5   
#define LORA_MISO    19   
#define LORA_MOSI    27   
#define LORA_SS      18   
#define LORA_RST     23  
#define LORA_DI0     26   

void LoRa_rxMode() {
  LoRa.disableInvertIQ();
  LoRa.receive();
}

