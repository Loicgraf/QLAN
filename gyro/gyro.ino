//Libraries
#include <Wire.h>//https://www.arduino.cc/en/reference/wire
#include <Adafruit_MPU6050.h>//https://github.com/adafruit/Adafruit_MPU6050

//Objets
Adafruit_MPU6050 mpu;

//-----------------------------------------//
//Variables liés différents axes du gyroscope/accelerometre

float ax=0;
float ay=0;
float az=0;
float SeuilVerticalite = 250;
float acceleration_total=0;
float gx=0;

//----------------------------------------//

void setup() {
   //Init Serial USB
  Serial.begin(9600);
  Serial.println(F("Initialize System"));
 if (!mpu.begin(0x68)) { // Change address if needed
      Serial.println("Failed to find MPU6050 chip");
      while (1) {
          delay(100);
      }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

}

//Fonction du gyroscope les valeurs de l'acceleromètre/gyroscope
void readMPU() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    ax = a.acceleration.x;
    ay = a.acceleration.y;
    az = a.acceleration.z;
    gx = g.gyro.x*(180.0 / PI);

    acceleration_total = sqrt(ax * ax + ay * ay + az * az);
    
    Serial.print("Acceleration total : ");
    Serial.println(acceleration_total);
    Serial.print("Gx");
    Serial.println(gx);

    if (abs(gx) >= SeuilVerticalite){
      Serial.println("Perte de vertialité");
      Serial.print("Gx :");
      Serial.println(gx);
    }
}
    
void loop() {
    readMPU(); // Read MPU data
    delay(20);
}
