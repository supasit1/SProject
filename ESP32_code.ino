#include "DHTesp.h"
#include <Wire.h>
#include <Ticker.h>

// กำหนดขาของ DHT22
#define DHT_PIN 15 //pin 15
#define BH1750_ADDR  (0x5C)
#define I2C_SDA_PIN  (21) //pin 21 green
#define I2C_SCL_PIN  (22) //pin 22 blue
DHTesp dht;


Ticker tempTicker;

// กำหนดขาของเซ็นเซอร์ความชืนในดิน
const int soilMoisturePin = 34; // pin 34
const int relayPumpPin = 18; // ตั้งค่า pin ของ Relay สำหรับปั้มน้ำ
const int relayLightPin = 19; // ตั้งค่า pin ของ Relay สำหรับไฟ
int user = 1; // สถานะผู้ใช้ (1: เปิด, 0: ปิด)
int user_moistureThreshold = 1000;
uint16_t user_luxThreshold = 1000;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN,I2C_SCL_PIN);
  // DHT Setup
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // BH1750 on
  Wire.setClock( 400000 );// set I2C speed to 400kHz

  // Start reading temperature and humidity every 20 seconds
  tempTicker.attach(20, getTemperature);

  pinMode(soilMoisturePin, INPUT); // ตั้งค่าขาที่เชื่อมต่อกับเซ็นเซอร์ความชืนในดินเป็นขาอินพุต

  pinMode(relayPumpPin, OUTPUT); // ตั้งค่าขา Relay สำหรับปั้มน้ำ เป็น Output
  pinMode(relayLightPin, OUTPUT); // ตั้งค่าขา Relay สำหรับไฟ เป็น Output

}

void loop() {
  // Read temperature and humidity from DHT22
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  // Read light level from BH1750
  uint16_t lux;
  // Read soil moisture
  int soilMoisture = analogRead(soilMoisturePin);
  
  //แสดงผล
  Serial.print("Temperature (DHT22): ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humidity (DHT22): ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Light (BH1750): ");
  if ( BH1750_read(BH1750_ADDR, &lux) ) 
  {
    // ควบคุม Relay สำหรับไฟ
    if (lux <= 1000 && user ==1) {
      digitalWrite(relayLightPin, HIGH); // เปิด Relay สำหรับไฟ
      Serial.println("relayLightPin = HIGH");
    } 
    else if(lux > 1000 ) 
    {
      digitalWrite(relayLightPin, LOW); // ปิด Relay สำหรับไฟ
      Serial.println("relayLightPin = LOW");
    }
    Serial.print(lux);
    Serial.println(" lux"); 
  } 
  else 
  {
    Serial.println( "Sensor reading error!" );
  }
  Serial.print("Soil Moisture: ");
  Serial.println(soilMoisture);
  // ควบคุม Relay สำหรับปั้มน้ำ
  if (soilMoisture <= 1000 && user ==1 ) {
    digitalWrite(relayPumpPin, HIGH); // เปิด Relay สำหรับปั้มน้ำ
    Serial.println("relayPumpPin = HIGH");
  } 
  else
  {
    digitalWrite(relayPumpPin, LOW); // ปิด Relay สำหรับปั้มน้ำ
    Serial.println("relayPumpPin = LOW");
  }
  Serial.println("________________________________________");

  delay(5000);
}

bool BH1750_read( uint8_t addr, uint16_t *lux ) 
{
  uint8_t buf[2];
  *lux = 0.0;
  Wire.beginTransmission( addr ); // send the addr/write byte
  // One-shot, Hi-Resolution Mode (1 Lux Resolution) 
  Wire.write( 0x20 ); // send the instruction to start measurement
  if( Wire.endTransmission() > 0 ) {
    Serial.println( "No response from the device!" );
    return false;
  }
  delay(150); // wait at least 120 msec.
  Wire.requestFrom( addr, 2, true );
  if ( Wire.available() == 2 ) {
    buf[0] = Wire.read(); 
    buf[1] = Wire.read(); 
  } else {
    return false;
  }
  uint32_t value = buf[0];
  value  = (value << 8) | buf[1];
  value /= 1.2; // convert raw data to Lux
  *lux = value;
  return true;
}

void getTemperature() {
  // Empty function to trigger temperature reading from DHT22
}
