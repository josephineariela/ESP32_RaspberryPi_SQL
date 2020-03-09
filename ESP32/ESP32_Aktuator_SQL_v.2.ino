/*Program for ESP-A
Created by: Josephine Ariella*/

//Update: tambah task get_SQL_data utk ambil data dari SQL, bisa post status aktuator di task print_status, tambah data manual di task get_SQL_data


//Global Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "RBDdimmerESP32.h"

//Hotspot Identity
const char* ssid     = "pipino";
const char* password = "cilukbaa";

//Server SQL & API Key identification
const char* server_fromSQL  = "http://192.168.43.159/SQL_toESP-A.php";
const char* server_toSQL    = "http://192.168.43.159/ESP-A_toSQL.php";
String apiKeyValue          = "tPmAT5Ab3j7F9";

// Pin Definitions 
    #define LED                 5 
    #define MICROPUMP           32
    #define HEATER_PWM          36
    #define HEATER_ZC           25
    #define KIPAS_HEATER        33
    #define COOLER              4
//    #define KIPAS_COOLER        14    
    #define KIPAS_COOLER        23
    #define DEHUMIDIFIER        13
    #define KIPAS_DEHUMIDIFIER  26
    #define HUMIDIFIER          21

//Enable-Disable Macro
    #define DISABLE 0
    #define ENABLE  1

//Heating-Cooling State
    #define HEAT      0
    #define WAIT_TEMP 1
    #define COOL      2

//Humidifying-Dehumidifying State
    #define HUMID      0
    #define WAIT_HUMID 1
    #define DEHUMID    2

//Moisture State
    #define PUMP_ON    0
    #define WAIT_ON    1
    #define WAIT_MOIST 2

//Light State
    #define LIGHT_OFF 0
    #define LIGHT_ON  1
    
//Manual-On Control Flag
    int F_HEATER  = DISABLE;
    int F_COOLER  = DISABLE;
    int F_DEHUMID = DISABLE;
    int F_HUMID   = DISABLE;
    int F_MOIST   = DISABLE;
    int lightFlag;

//State Declaration
    int tempState   = WAIT_TEMP;
    int humidState  = WAIT_HUMID;
    int moistState  = WAIT_MOIST;
    int lightState  = LIGHT_OFF;
    
//Global Variable & Macro
    #define TEMP_THRESHOLD      1
    #define HUMID_THRESHOLD     1
    #define MOIST_THRESHOLD     1
    #define DELAY_ON            10
    #define DELAY_WAIT 30+DELAY_ON

//Setting PWM properties
    const int freq        = 50000;
    const int ledChannel  = 0;
    const int resolution  = 8;
    int min_dutyCycle     = 0;
    int max_dutyCycle     = 255;
    

//Global Variable Initialization
    //Variables for convert data (actual, optimal, manual) from SQL
    int i, end_ActTemp, end_ActHum, end_ActMoist, end_ActLight;
    int end_OptTemp, end_OptHum, end_OptMoist, end_OptLight;
    int end_heater, end_cooler, end_humidifier, end_dehumidifier, end_pump, end_light;
    int j = 0;
    char ActTemp[5], ActHum[5], ActMoist[5], ActLight[10];
    char OptTemp[5], OptHum[5], OptMoist[5], OptLight[10];
    char heater_status[5], cooler_status[5], humidifier_status[5], dehumidifier_status[5], pump_status[5], light_status[5];
    //Variables for Actual & Optimal datas in float & Manual datas in String
    float fActTemp, fActHum, fActMoist, fActLight;
    float fOptTemp, fOptHum, fOptMoist, fOptLight;
    String scooler_status, sheater_status, shumidifier_status, sdehumidifier_status, spump_status, slight_status;
    //Variable for manual data in int (send to SQL database) as feedback from ESP-A to Interface
    int coolerState, heaterState, humidifierState, dehumidifierState, ledState, pumpState;
    //Variables & Declaration for Heater Driver
    dimmerLampESP32 dimmer(HEATER_PWM, HEATER_ZC);
    int heaterDutyCycle = 0;
    long activeStart;
    //char tStr2[8], hStr2[8], mStr2[8], lStr2[8];

//Task Priority for RTOS
#define priorityTask1 6 //LED
#define priorityTask2 5 //Temperature
#define priorityTask3 4 //Humidity
#define priorityTask4 3 //Moisture
#define priorityTask5 2 //printStatus
#define priorityTask6 1 //getting data from SQL

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(MICROPUMP, OUTPUT);
  pinMode(KIPAS_HEATER, OUTPUT);
  pinMode(COOLER, OUTPUT);
//  pinMode(KIPAS_COOLER, OUTPUT);
  pinMode(DEHUMIDIFIER, OUTPUT);
  pinMode(KIPAS_DEHUMIDIFIER, OUTPUT);
  pinMode(HUMIDIFIER, OUTPUT);

  
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(KIPAS_COOLER, ledChannel);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) { 
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  dimmer.begin(NORMAL_MODE, ON);

  xTaskCreatePinnedToCore( lightTask , "Task 1" , 2048 , NULL , priorityTask1 , NULL , 1);
  xTaskCreatePinnedToCore( temperatureTask , "Task 2" , 2048 , NULL , priorityTask2 , NULL , 1);
  xTaskCreatePinnedToCore( humidityTask , "Task 3" , 2048 , NULL , priorityTask3 , NULL , 0);
  xTaskCreatePinnedToCore( moistureTask , "Task 4" , 2048 , NULL , priorityTask4 , NULL , 1);
  xTaskCreatePinnedToCore( get_SQL_data , "Task 5" , 2048 , NULL , priorityTask5 , NULL , 0);
  xTaskCreatePinnedToCore( print_status , "Task 6" , 2048 , NULL , priorityTask6 , NULL , 0);
  
}

void loop() {
}

void get_SQL_data(void *pvParam){
  (void) pvParam;
  while(1){
     //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(server_fromSQL);
    
      //Getting the response code
      int httpCode = http.GET();
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);

      //Getting data from SQL Database
      String DataFromSQL = http.getString();
      Serial.println("Data From SQL: " + DataFromSQL);


    //Convert string data(DataFromSQL) to array of char(charDataFromSQL)
    char charDataFromSQL[DataFromSQL.length()];
    for (i = 0; i< sizeof(charDataFromSQL); i++){
      charDataFromSQL[i] = DataFromSQL[i];
      if (charDataFromSQL[i] == 'v'){ //partition for ActTemp Data is v
        end_ActTemp = i;
      }else if (charDataFromSQL[i] == 'w'){ //partition for ActHum Data is w
        end_ActHum = i;
      }else if (charDataFromSQL[i] == 'x'){ //partition for ActMoist Data is x
        end_ActMoist = i;
      }else if (charDataFromSQL[i] == 'y'){ //partition for ActLight Data is y
        end_ActLight = i;
      }else if (charDataFromSQL[i] == 'a'){ //partition for OptTemp Data is a
        end_OptTemp = i;
      }else if (charDataFromSQL[i] == 'b'){ //partition for OptHum Data is b
        end_OptHum = i;
      }else if (charDataFromSQL[i] == 'c'){ //partition for OptMoist Data is c
        end_OptMoist = i;
      }else if (charDataFromSQL[i] == 'd'){ //partition for OptLight Data is d
        end_OptLight = i;
      }else if (charDataFromSQL[i] == 'g'){ //partition for heater_status Data is g
        end_heater = i;
      }else if (charDataFromSQL[i] == 'h'){ //partition for cooler_status Data is h
        end_cooler = i;
      }else if (charDataFromSQL[i] == 'i'){ //partition for humidifier_status Data is i
        end_humidifier = i;
      }else if (charDataFromSQL[i] == 'j'){ //partition for dehumidifier_status Data is j
        end_dehumidifier = i;
      }else if (charDataFromSQL[i] == 'k'){ //partition for pump_status Data is k
        end_pump = i;
      }else if (charDataFromSQL[i] == 'l'){ //partition for light_status Data is l
        end_light = i;
      }
       
    }

      //Convert array of char(charDataFromSQL) to float for each parameters
      for (i=0; i<end_ActTemp; i++){
        ActTemp[j] = charDataFromSQL[i];
        j++;
      }
      fActTemp = atof(ActTemp);
      j=0;

      for (i=end_ActTemp+1; i<end_ActHum; i++){
        ActHum[j] = charDataFromSQL[i];
        j++;
      }
      fActHum = atof(ActHum);
      j=0;

      for (i=end_ActHum+1; i<end_ActMoist; i++){
        ActMoist[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist = atof(ActMoist);
      j=0;

      for (i=end_ActMoist+1; i<end_ActLight; i++){
        ActLight[j] = charDataFromSQL[i];
        j++;
      }
      fActLight = atof(ActLight);
      j=0;

      for (i=end_ActLight+1; i<end_OptTemp; i++){
        OptTemp[j] = charDataFromSQL[i];
        j++;
      }
      fOptTemp = atof(OptTemp);
      j=0;

      for (i=end_OptTemp+1; i<end_OptHum; i++){
        OptHum[j] = charDataFromSQL[i];
        j++;
      }
      fOptHum = atof(OptHum);
      j=0;

      for (i=end_OptHum+1; i<end_OptMoist; i++){
        OptMoist[j] = charDataFromSQL[i];
        j++;
      }
      fOptMoist = atof(OptMoist);
      j=0;

      for (i=end_OptMoist+1; i<end_OptLight; i++){
        OptLight[j] = charDataFromSQL[i];
        j++;
      }
      fOptLight = atof(OptLight);
      j=0;

      for (i=end_OptLight+1; i<end_heater; i++){
        heater_status[j] = charDataFromSQL[i];
        j++;
      }
      sheater_status = (String)heater_status;
      j=0;
      
      for (i=end_heater+1; i<end_cooler; i++){
        cooler_status[j] = charDataFromSQL[i];
        j++;
      }
      scooler_status = (String)cooler_status;
      j=0;
      
      for (i=end_cooler+1; i<end_humidifier; i++){
        humidifier_status[j] = charDataFromSQL[i];
        j++;
      }
      shumidifier_status = (String)humidifier_status;
      j=0;
      
      for (i=end_humidifier+1; i<end_dehumidifier; i++){
        dehumidifier_status[j] = charDataFromSQL[i];
        j++;
      }
      sdehumidifier_status = (String)dehumidifier_status;
      j=0;
      
      for (i=end_dehumidifier+1; i<end_pump; i++){
        pump_status[j] = charDataFromSQL[i];
        j++;
      }
      spump_status = (String)pump_status;
      j=0;
      
      for (i=end_pump+1; i<end_light; i++){
        light_status[j] = charDataFromSQL[i];
        j++;
      }
      slight_status = (String)light_status;
      j=0;
      
      Serial.println();
      //Declaring status of actuators
      if (sheater_status = "ON"){
        F_HEATER = ENABLE;
        F_COOLER = DISABLE;
      }else{
        F_HEATER = DISABLE;
      }
      if (scooler_status == "ON") {
        F_HEATER = DISABLE;
        F_COOLER = ENABLE;
      }else{
        F_COOLER = DISABLE;
      }
      if (shumidifier_status = "ON"){
        F_HUMID = ENABLE;
        F_DEHUMID = DISABLE;
      }else{
        F_HUMID = DISABLE;
      }
      if (sdehumidifier_status = "ON"){
        F_DEHUMID = ENABLE;
        F_HUMID = DISABLE;
      }else{
        F_DEHUMID = DISABLE;
      }
      if (spump_status = "ON"){
        F_MOIST = ENABLE;
      }else{
       F_MOIST = DISABLE;
      }
      if (slight_status = "ON"){
        lightFlag = ENABLE;        
      }else{
        lightFlag = DISABLE;        
      }

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (5000);
  }
}


void print_status(void *pvParam){
  (void) pvParam;
  while(1){
    // for checking actuator status
    Serial.println("Heater Status : " + sheater_status); 
    Serial.println("Cooler Status : " + scooler_status); 
    Serial.println("Humidifier Status : " + shumidifier_status); 
    Serial.println("Dehumidifier Status : " + sdehumidifier_status); 
    Serial.println("Pump Status : " + spump_status); 
    Serial.println("Light Status : " + slight_status); 
    Serial.println("-----------------");
    Serial.print("ActTemp = ");
    Serial.println(fActTemp);
    Serial.print("ActHum = ");
    Serial.println(fActHum);
    Serial.print("ActMoist = ");
    Serial.println(fActMoist);      
    Serial.print("ActLight = ");
    Serial.println(fActLight);
    Serial.println("-----------------");
    Serial.print("OptTemp = ");
    Serial.println(fOptTemp);
    Serial.print("OptHum = ");
    Serial.println(fOptHum);
    Serial.print("OptMoist = ");
    Serial.println(fOptMoist);      
    Serial.print("OptLight = ");
    Serial.println(fOptLight);
    Serial.println("-----------------");

    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(server_toSQL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String httpRequestData = "api_key=" + apiKeyValue
                          + "&heater=" + heaterState
                          + "&cooler=" + coolerState
                          + "&humidifier=" + humidifierState
                          + "&dehumidifier=" + dehumidifierState
                          + "&pump=" + pumpState
                          + "&light=" + ledState + "";
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
    
    vTaskDelay(3000);
  }
}


void lightTask(void *pvParam){
  (void) pvParam;
  while(1){
    light_control();
    
    vTaskDelay(1000);
  }
}


//Temperature Control Procedure Call
void temperatureTask(void *pvParam){
  (void) pvParam;
  while(1){    
    if (F_HEATER){
        heaterDutyCycle = 1;
        heaterState = 1;
        coolerState = 0;
        digitalWrite(KIPAS_HEATER,HIGH);
        digitalWrite(COOLER,LOW);
        ledcWrite(ledChannel, min_dutyCycle);
//        digitalWrite(KIPAS_COOLER,LOW);
    }
    else if (F_COOLER){
        heaterDutyCycle = 0;
        coolerState = 1;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,HIGH);
//        digitalWrite(KIPAS_COOLER,HIGH);
        ledcWrite(ledChannel, max_dutyCycle);
    }
    else{
        temperature_control();
    }
    dimmer.setPower(heaterDutyCycle);
    
    vTaskDelay(2500);
  }
}

//Humidity Control Procedure Call
void humidityTask(void *pvParam){
  (void) pvParam;
  while(1){
    if (F_HUMID){
        humidifierState = 1;
        dehumidifierState = 0;
        digitalWrite(HUMIDIFIER,HIGH);
        digitalWrite(DEHUMIDIFIER,LOW);
        digitalWrite(KIPAS_DEHUMIDIFIER,LOW);
    }
    else if (F_DEHUMID){
        humidifierState = 0;
        dehumidifierState = 1;
        digitalWrite(HUMIDIFIER,LOW);
        digitalWrite(DEHUMIDIFIER,HIGH);
        digitalWrite(KIPAS_DEHUMIDIFIER,HIGH);
    }
    else{
        humidity_control();
    }
    
    vTaskDelay(2500);
  }
}

void moistureTask(void *pvParam){
  (void) pvParam;
  while(1){
    if (F_MOIST){
        pumpState = 1;
        digitalWrite(MICROPUMP,HIGH);
    }
    else{
        moisture_control();
    }       

    vTaskDelay(2000);
  }
}

void temperature_control(){
  //dtostrf(tempState,1,2,tStr2);
    if (tempState == HEAT){
        heaterDutyCycle = 1;
        coolerState = 0;
        heaterState = 1;
        digitalWrite(KIPAS_HEATER,HIGH);
        digitalWrite(COOLER,LOW);
//        digitalWrite(KIPAS_COOLER,LOW);
        ledcWrite(ledChannel, min_dutyCycle);
        if (fActTemp >= fOptTemp) {
          tempState = WAIT_TEMP;
        }
    }
    else if (tempState == WAIT_TEMP){
        heaterDutyCycle = 0;
        coolerState = 0;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,LOW);
//        digitalWrite(KIPAS_COOLER,LOW);
        ledcWrite(ledChannel, min_dutyCycle);
        if((fOptTemp - fActTemp)>= TEMP_THRESHOLD){
          tempState = HEAT;
        }
        else if((fActTemp - fOptTemp)>= TEMP_THRESHOLD){
          tempState = COOL;
        }
    }
    else if (tempState == COOL) {
        heaterDutyCycle = 0;
        coolerState = 1;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,HIGH);
//        digitalWrite(KIPAS_COOLER,HIGH);
        ledcWrite(ledChannel, max_dutyCycle);
        if (fActTemp <= fOptTemp){
          tempState = WAIT_TEMP;
        }
    }
    else {
        Serial.println ("tempState not Defined!");
    }
}

void humidity_control(){
  //dtostrf(humidState,1,2,hStr2);
    if (humidState == HUMID){
        humidifierState = 1;
        dehumidifierState = 0;
        digitalWrite(HUMIDIFIER,HIGH);
        digitalWrite(DEHUMIDIFIER,LOW);
        digitalWrite(KIPAS_DEHUMIDIFIER,LOW);
        if (fActHum >= fOptHum) {
          humidState = WAIT_HUMID;
        }
    }
    else if (humidState == WAIT_HUMID){
        humidifierState = 0;
        dehumidifierState = 0;
        digitalWrite(HUMIDIFIER,LOW);
        digitalWrite(DEHUMIDIFIER,LOW);
        digitalWrite(KIPAS_DEHUMIDIFIER,LOW);
        if((fOptHum - fActHum)>= HUMID_THRESHOLD){
          humidState = HUMID;
        }
        else if((fActHum - fOptHum)>= HUMID_THRESHOLD){
          humidState = DEHUMID;
        }
    }
    else if (humidState == DEHUMID) {
        humidifierState = 0;
        dehumidifierState = 1;
        digitalWrite(HUMID,LOW);
        digitalWrite(DEHUMIDIFIER,HIGH);
        digitalWrite(KIPAS_DEHUMIDIFIER,HIGH);
        if (fActHum <= fOptHum){
          humidState = WAIT_HUMID;
        }
    }
    else {
        Serial.println ("humidState not Defined!");
    }
}

void moisture_control(){
  //dtostrf(moistState,1,2,mStr2);
    if (moistState == PUMP_ON){
        activeStart = millis();
        pumpState = 1;
        digitalWrite(MICROPUMP,HIGH);
        moistState = WAIT_ON;
    }
    else if (moistState == WAIT_ON){
        if((millis() - activeStart) >= (DELAY_ON * 1000)){
            moistState = WAIT_MOIST;
        }
    }
    else if (moistState == WAIT_MOIST) {
        pumpState = 0;
        digitalWrite(MICROPUMP,LOW);
        if (((millis() - activeStart) >= (DELAY_WAIT * 1000)) && (fActMoist <= fOptMoist)){
            moistState = PUMP_ON;
        }
    }
    else {
        Serial.println ("moistState not Defined!");
    }    
}

void light_control(){
  //dtostrf(lightState,1,2,lStr2);
    if (lightState == LIGHT_OFF) {
        ledState = 0;
        digitalWrite(LED, LOW);
        if(lightFlag == ENABLE){
            lightState = LIGHT_ON;
        }
    }
    else if (lightState == LIGHT_ON) {
        ledState = 1;
        digitalWrite(LED, HIGH);
        if(lightFlag == DISABLE){
            lightState = LIGHT_OFF;
        }
    }
    else
    {
       Serial.println ("lightState not Defined!");
    }    
}
