#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32Test"); // Nom de votre ESP32
  Serial.println("L'appareil est prêt à être couplé");
}
void loop() {
  lectbluetooth();
}
void lectbluetooth(){
  while(true){
    if (SerialBT.available()) {
    String incoming = SerialBT.readString();
    Serial.print("Message reçu : ");
    Serial.println(incoming);
    break;
    }
    delay(20);
  }
}