//ESP32 Dev Module
#include "DHTesp.h"
#include <Wire.h>
#include <Ticker.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Firebase_ESP_Client.h>
#include "string.h"
#include <TimeLib.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// กำหนดขาของ DHT22
#define DHT_PIN 15 //pin 15
#define BH1750_ADDR  (0x5C) //RX
#define I2C_SDA_PIN  (21) //pin 21 green
#define I2C_SCL_PIN  (22) //pin 22 blue

#define DATABASE_URL "https://farmself-1beda-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define API_KEY "AIzaSyBg-VCiyYgtsqYAViFtxjD6lQmjLHSFdi8"


DHTesp dht;
Ticker tempTicker;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
AsyncWebServer server(80);
// กำหนดขาของเซ็นเซอร์ความชืนในดิน
const int soilMoisturePin = 34; // pin 34
const int relayPumpPin = 18; // ตั้งค่า pin ของ Relay สำหรับปั้มน้ำ
const int relayLightPin = 19; // ตั้งค่า pin ของ Relay สำหรับไฟ
String  user_pumpstatus = "1"; // สถานะผู้ใช้ (1: เปิด, 0: ปิด)
String  user_lightstatus = "1";
int user_moistureThreshold = 1000;
uint16_t user_luxThreshold = 1000;
int soilMoisture =0;
float humidity = 0;
float temperature = 0;
// Read light level from BH1750
uint16_t lux;
bool signupOK = false;
unsigned long sendDataPrevMillis = 0;
//Setwifi
String stationSSID = "I PHONEs";//I PHONE
String stationPassword= "086020DB";
const char* AP_ssid="ESP32AP";
const char* AP_password ="1234567890";

void setup() {
  Serial.begin(115200);
  // ให้ ESP32 เชื่อมต่อกับ NTP server เพื่อรับข้อมูลเวลา
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    delay(1000);
    Serial.println("Waiting for time sync...");
  }
  Wire.begin(I2C_SDA_PIN,I2C_SCL_PIN);
  // DHT Setup
  dht.setup(DHT_PIN, DHTesp::DHT22);
  // BH1750 on
  Wire.setClock( 400000 );// set I2C speed to 400kHz
  tempTicker.attach(20, getTemperature);// Start reading temperature and humidity every 20 seconds
  pinMode(soilMoisturePin, INPUT); // ตั้งค่าขาที่เชื่อมต่อกับเซ็นเซอร์ความชืนในดินเป็นขาอินพุต
  pinMode(relayPumpPin, OUTPUT); // ตั้งค่าขา Relay สำหรับปั้มน้ำ เป็น Output
  pinMode(relayLightPin, OUTPUT); // ตั้งค่าขา Relay สำหรับไฟ เป็น Output
  //Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_ssid, AP_password);
  Serial.println("Access Point started");
  // Redirect to the configuration page
  Serial.println("Redirecting to configuration page");
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  IPAddress AP_IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(AP_IP);
  //station
  connectToWiFi();
  //Firebase
  //Assign the api key (required) 
  config.api_key = API_KEY;
  //Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
   /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}

void loop() {
  // Read temperature and humidity from DHT22
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();

  Firebase_GET();
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
    if (lux <= 1000 && user_lightstatus == "1") {
      digitalWrite(relayLightPin, HIGH); // เปิด Relay สำหรับไฟ
      Serial.println("relayLightPin = HIGH");
    } 
    else 
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
  if (soilMoisture <= 1000 && user_pumpstatus == "1" ) {
    digitalWrite(relayPumpPin, HIGH); // เปิด Relay สำหรับปั้มน้ำ
    Serial.println("relayPumpPin = HIGH");
  } 
  else
  {
    digitalWrite(relayPumpPin, LOW); // ปิด Relay สำหรับปั้มน้ำ
    Serial.println("relayPumpPin = LOW");
  }
  //รับข้อมูลเวลาปัจจุบัน
  time_t now = time(nullptr);
  // แปลงเวลาเป็นโครงสร้าง tm
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  // Read soil moisture
  soilMoisture = analogRead(soilMoisturePin);
  //แสดงผล เวลา
   Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo->tm_year + 1900,
                timeinfo->tm_mon + 1,
                timeinfo->tm_mday,
                timeinfo->tm_hour+7,
                timeinfo->tm_min,
                timeinfo->tm_sec);
  Serial.println("________________________________________");
  Firebase_SET();
  delay(5000);
}
void Firebase_SET(){
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    //Temperature
    if (Firebase.RTDB.setFloat(&fbdo, "Data/Temperature", temperature)){
      Serial.printf("PASSED: %.2f\n", temperature);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //Humidity
    if (Firebase.RTDB.setFloat(&fbdo, "Data/Humidity", humidity)){
      Serial.printf("PASSED: %.2f\n",humidity);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 
    //soilMoisture
    if (Firebase.RTDB.setFloat(&fbdo, "Data/Soilmoisture", soilMoisture)){
      Serial.printf("PASSED: %.2f\n",soilMoisture);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 
    //Lux
    if (Firebase.RTDB.setFloat(&fbdo, "Data/Lux", lux)){
      Serial.printf("PASSED: %u\n", lux);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    } 
  }
  Serial.println("Complete");
}

void Firebase_GET(){
  if (Firebase.RTDB.getString(&fbdo, "users/LightStatus/value")) {
    user_lightstatus = fbdo.stringData().c_str();
    Serial.print("LightStatus: ");
    Serial.println(user_lightstatus);
  }
  else {
    Serial.println(fbdo.errorReason());
  }
  
  //PumpStatus
  if (Firebase.RTDB.getString(&fbdo, "users/PumpStatus/value")) {
    user_pumpstatus = fbdo.stringData().c_str();
    Serial.print("PumpStatus: ");
    Serial.println(user_pumpstatus);
  }
  else {
    Serial.println(fbdo.errorReason());
  }
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
  delay(150); // wait at least 150 msec.
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

void connectToWiFi() {
  // ใช้ค่า SSID และ Password ของ Station จากตัวแปร
  WiFi.begin(stationSSID.c_str(), stationPassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(stationSSID);
    Serial.println(stationPassword);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void handleRoot(AsyncWebServerRequest *request){
  String html = "<html><body>";
  html += "<h1>ESP32 WiFi Configuration</h1>";
  html += "<form action='/save' method='POST'>";
  html += "SSID: <input type='text' name='ssid' value='" + stationSSID + "'><br>";  // เพิ่ม value='" + stationSSID + "'
  html += "Password: <input type='password' name='password' value='" + stationPassword + "'><br>";  // เพิ่ม value='" + stationPassword + "'
  html += "<input type='submit' value='Save'>";
  html += "</form></body></html>";
  request->send(200, "text/html", html);
}

void handleSave(AsyncWebServerRequest *request){
  stationSSID = request->arg("ssid");
  stationPassword = request->arg("password");
  Serial.println("New SSID:" + stationSSID);
  Serial.println("New Password:" + stationPassword);

  //Connect to the updated WiFi
  connectToWiFi();

  request->send(200, "text/plain", "Configuration saved successfully");
}
  
