/*Program for ESP-S
Created by: Josephine Ariella*/

//Global Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

//Library sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_TSL2561_U.h>

//Hotspot Identity
const char* ssid     = "pipino";
const char* password = "cilukbaa";

//Global variable for sensor
sensors_event_t event;
//TwoWire I2CSHT = TwoWire();
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

//soil sensor pin
#define soil 34    

//Variable initialization
int maxValue = 3057;
int minValue = 1438;
int val;

//Server & API Key identification
const char* serverName = "http://192.168.43.159/ESP-S_toSQL.php";
String apiKeyValue = "192001003";

void setup() {
  Serial.begin(115200);
  
  pinMode(soil, ANALOG);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  //setting up TSL2561 sensor
  tsl.begin();
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  
  //setting up SHT31 sensor
  if (! sht31.begin(0x44)){
    Serial.println("Couldn't find SHT31");
    while (1) 
      delay(1);
  }else
    Serial.println("SHT31 Working!");
}

void loop() {
  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    //getting light & soil sensor data
    tsl.getEvent(&event);
    val = analogRead(soil);
    
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue
                          + "&temp=" + String(sht31.readTemperature())
                          + "&hum=" + String(sht31.readHumidity())
                          + "&moist=" + String(map(val, maxValue, minValue, 0, 100)) 
                          + "&light=" + String(event.light)+ "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
        
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  //Send an HTTP POST request every 30 seconds
  delay(30000);  
}
