/*Program for ESP-S
Created by: Josephine Ariella*/

/*Global Library*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

/*Sensor Library*/
#include <Adafruit_Sensor.h>
#include <Adafruit_SHT31.h>
#include <MAX44009.h>

/*HTTP Protocol Access Point*/
//const char* ssid     = "TP-Link_F4A6";
//const char* password = "15798039";
//const char* ssid = "Opor Ayam";
//const char* password = "10110678";
const char* ssid = "pipino";
const char* password = "cilukbaa";

/*Global variable for sensor*/
//TwoWire I2CSHT = TwoWire();
Adafruit_SHT31 sht31_1 = Adafruit_SHT31();
Adafruit_SHT31 sht31_2 = Adafruit_SHT31();
MAX44009 light;

/*Pin Declaration*/
#define soilPin1 36 //D39, D36, D34, D35, D32, D33 bisa untuk soil
#define soilPin2 39
#define soilPin3 34
#define soilPin4 35
#define soilPin5 32
#define waterPin 33

/*Dry Air & Moist Air Soil Sensor Calibration*/
const int AirValue1 = 3791;
const int WaterValue1 = 1262;
const int AirValue2 = 3809;
const int WaterValue2 = 1169;
const int AirValue3 = 3663;
const int WaterValue3 = 1193;
const int AirValue4 = 3789;
const int WaterValue4 = 1155;
const int AirValue5 = 3761;
const int WaterValue5 = 1168;

/*Variable initialization*/
int val1, val2, val3, val4, val5, val6;
float t1, t2, h1, h2, t_sum, h_sum;
String t, t_back, t_right = "25.5";
String h, h_back, h_right = "78";
String m1, m2, m3, m4, m5 = "75";
String l = "50";
String water = "NO";
int waterLevel;

/*Server & API Key identification*/
const char* server_toSQL      = "http://192.168.43.184/phpmyadmin/ESP-S_toSQL.php"; //TP_Link-F4A6 Xampp Windows 10
const char* server_toSQL_2    = "http://35.240.254.221/ESP-S_toSQL.php"; //TP_Link-F4A6 Xampp Windows 10
String apiKeyValue = "tPmAT5Ab3j7F9";

/*Task Priority for RTOS*/ 
#define priorityTask1 3
#define priorityTask2 2
#define priorityTask3 1

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //setting up Wifi Connection
  setup_wifi();
  
  //setting up GY-49 sensor
  if(light.begin()){
    Serial.println("Could not find a valid MAX44009 sensor, check wiring!");
    while(1);
  }
  
  //setting up SHT31 sensor
  if (! sht31_1.begin(0x44)) {
    Serial.println("Couldn't find SHT31 1");
    while (1) 
      delay(1);
   }

  if (! sht31_2.begin(0x45)) { //ADR pin connect to VIN pin for getting 0x45 address
    Serial.println("Couldn't find SHT31 2");
    while (1) 
      delay(1);
  }
  
  /*RTOS Configuration*/
  xTaskCreatePinnedToCore( get_sensor_data , "Task 1" , 2048 , NULL , priorityTask1 , NULL , 0);
  xTaskCreatePinnedToCore( send_sensor_data , "Task 2" , 2048 , NULL , priorityTask2 , NULL , 0);
  xTaskCreatePinnedToCore( send_sensor_data_2 , "Task 3" , 2048 , NULL , priorityTask3 , NULL , 0);

}

void setup_wifi(){
   WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void get_sensor_data(void *pvParam){
  (void) pvParam;
  while(1){
    /*Getting Sensor Data*/
    val1 = analogRead(soilPin1);
    val2 = analogRead(soilPin2);
    val3 = analogRead(soilPin3);
    val4 = analogRead(soilPin4);
    val5 = analogRead(soilPin5);
    t1 = sht31_1.readTemperature();
    h1 = sht31_1.readHumidity();
    t2 = sht31_2.readTemperature();
    h2 = sht31_2.readHumidity();
    l = light.get_lux();

    /*SHT31 data calculation for 2 sensors*/
    t_sum = (t1 + t2)/2;
    h_sum = (h1 + h2)/2;
    t_back = String(t1);
    h_back = String(h1);
    t_right = String(t2);
    h_right = String(h2);
    t = String(t_sum);
    h = String(h_sum);

    /*Water level identification*/
    waterLevel = analogRead(waterPin);
    if (waterLevel < 1250){
      water = "YES";
    }else{
      water = "NO";
    }

    /*Soil sensor data conversion to %RH*/
    m1 = map(val1, AirValue1, WaterValue1, 0, 100);
    m2 = map(val2, AirValue2, WaterValue2, 0, 100);
    m3 = map(val3, AirValue3, WaterValue3, 0, 100);
    m4 = map(val4, AirValue4, WaterValue4, 0, 100);
    m5 = map(val5, AirValue5, WaterValue5, 0, 100);

    Serial.println("ActTemp 1 = " + t_back + " C");
    Serial.println("ActHum 1 = " + h_back + " %RH");
    Serial.println("ActTemp 2 = " + t_right + " C");
    Serial.println("ActHum 2 = " + h_right + " %RH");
    Serial.println("ActTemp Total = " + t + " C");
    Serial.println("ActHum Total = " + h + " %RH");
    Serial.println("ActMoist 1 = " + m1 + " %RH");
    Serial.println("ActMoist 2 = " + m2 + " %RH");
    Serial.println("ActMoist 3 = " + m3 + " %RH");
    Serial.println("ActMoist 4 = " + m4 + " %RH");
    Serial.println("ActMoist 5 = " + m5 + " %RH");
    Serial.println("ActLight = " + l + " Lux");
    Serial.print("Water Level = ");
    Serial.println(waterLevel);
    Serial.println("Do we need to add water? " + water);
    Serial.println("-----------------");   
    //It means sensors keep reading datas & saved in temporary variables
    vTaskDelay (5000);
  }
}

/*At certain delay (every 30 seconds), Actual Datas are sent to online Database*/
void send_sensor_data(void *pvParam){
  (void) pvParam;
  while(1){
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(server_toSQL_2);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
      // Prepare your HTTP POST request data
      String httpRequestData_2 = "api_key= " + apiKeyValue
                          + " &temp= " + t
                          + " &hum= " + h
                          + " &moist1= " + m1
                          + " &moist2= " + m2
                          + " &moist3= " + m3
                          + " &moist4= " + m4
                          + " &moist5= " + m5
                          + " &light= " + l 
                          + " &water= " + water + "";
      Serial.println("Sensor Data Online : ");
      Serial.println(httpRequestData_2);
      Serial.println("-----------------");   

      // Send HTTP POST request
      int httpResponseCode_2 = http.POST(httpRequestData_2);
        
      if (httpResponseCode_2>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode_2);
        Serial.println("-----------------");   
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode_2);
        Serial.println("-----------------");   
      }
      // Free resources
      http.end();
      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (30000);
  }
}

/*At certain delay (every 30 seconds), Actual Datas are sent to local Database*/
void send_sensor_data_2(void *pvParam){
  (void) pvParam;
  while(1){
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(server_toSQL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
      // Prepare your HTTP POST request data
      String httpRequestData = "api_key= " + apiKeyValue
                          + " &temp= " + t
                          + " &hum= " + h
                          + " &moist1= " + m1
                          + " &moist2= " + m2
                          + " &moist3= " + m3
                          + " &moist4= " + m4
                          + " &moist5= " + m5
                          + " &light= " + l 
                          + " &water= " + water + "";
      Serial.println("Sensor Data Offline : ");
      Serial.println(httpRequestData);
      Serial.println("-----------------");   

      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
        
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        Serial.println("-----------------");   
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        Serial.println("-----------------");   
      }
      // Free resources
      http.end();
      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (30000);
  }
}

void loop() {
}
