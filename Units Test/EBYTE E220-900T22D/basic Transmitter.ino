// Transmitter code
HardwareSerial SerialAT(1);
#define RXD 34
#define TXD 35
void setup() {  
  SerialAT.begin(9600,SERIAL_8N1,RXD,TXD);
}

void loop() {  
  SerialAT.println(12);
  delay(500);   
}