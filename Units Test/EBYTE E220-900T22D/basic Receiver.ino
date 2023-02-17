// Receiver code
HardwareSerial SerialAT(1);
#define RXD 34
#define TXD 35
void setup() {  
  Serial.begin(9600);
  SerialAT.begin(9600,SERIAL_8N1,RXD,TXD);
}

void loop() {  
  if (SerialAT.available()) {
    Serial.write(SerialAT.read());
  }  
}
