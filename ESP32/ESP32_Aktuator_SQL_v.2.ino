/*Program for ESP-A
Created by: Josephine Ariella*/

//Update: tambah task get_SQL_data utk ambil data dari SQL


//Global Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "RBDdimmerESP32.h"

//Hotspot Identity
const char* ssid     = "pipino";
const char* password = "cilukbaa";

//Server & API Key identification
const char* serverName = "http://192.168.43.159/SQL_toESP-A.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

// Pin Definitions 
    #define LED 5 
    #define MICROPUMP 32
    #define HEATER_PWM 36
    #define HEATER_ZC 25
    #define KIPAS_HEATER 33
    #define COOLER 4
    #define KIPAS_COOLER 14
    #define DEHUMIDIFIER 13
    #define KIPAS_DEHUMIDIFIER 26
    #define HUMIDIFIER 21

//Enable-Disable Macro
    #define DISABLE 0
    #define ENABLE 1

//Heating-Cooling State
    #define HEAT 0
    #define WAIT_TEMP 1
    #define COOL 2

//Humidifying-Dehumidifying State
    #define HUMID 0
    #define WAIT_HUMID 1
    #define DEHUMID 2

//Moisture State
    #define PUMP_ON 0
    #define WAIT_ON 1
    #define WAIT_MOIST 2

//Light State
    #define LIGHT_OFF 0
    #define LIGHT_ON 1
    
//Manual-On Control Flag
    int F_HEATER = DISABLE;
    int F_COOLER = DISABLE;
    int F_DEHUMID = DISABLE;
    int F_HUMID = DISABLE;
    int F_MOIST = DISABLE;
    int lightFlag;

//State Declaration
    int tempState = WAIT_TEMP;
    int humidState = WAIT_HUMID;
    int moistState = WAIT_MOIST;
    int lightState = LIGHT_OFF;
    int coolerState, heaterState, humidifierState, dehumidifierState, ledState, pumpState;
    
//Global Variable & Macro
    #define TEMP_THRESHOLD 1
    #define HUMID_THRESHOLD 1
    #define MOIST_THRESHOLD 1
    #define DELAY_ON 10
    #define DELAY_WAIT 30+DELAY_ON

    
dimmerLampESP32 dimmer(HEATER_PWM, HEATER_ZC);

//Global Variable Initialization
int i, end_ActTemp, end_ActHum, end_ActMoist, end_ActLight;
int j =0;
float fActTemp, fActHum, fActMoist, fActLight;
char ActTemp[5], ActHum[5], ActMoist[5], ActLight[10];

    float OptTemp = 25;
    float OptHum = 60;
    float OptMoist = 60;
    int heaterDutyCycle = 0;
    long activeStart;
    char heaterS[8], coolerS[8], humidS[8], dehumidS[8], pumpS[8], lightS[8];
    char tStr2[8], hStr2[8], mStr2[8], lStr2[8];
    String cooler_status, heater_status, humid_status, dehumid_status, pump_status, light_status;

//Task Priority
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
  pinMode(KIPAS_COOLER, OUTPUT);
  pinMode(DEHUMIDIFIER, OUTPUT);
  pinMode(KIPAS_DEHUMIDIFIER, OUTPUT);
  pinMode(HUMIDIFIER, OUTPUT);
  
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
      http.begin(serverName);
    
      //Getting the response code
      int httpCode = http.GET();
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);

      //Getting data from SQL Database
      String dataActual = http.getString();
      Serial.println("Data Actual: " + dataActual);

      //Convert string data(dataActual) to array of char(charDataActual)
      char charDataActual[dataActual.length()];
      for (i = 0; i< sizeof(charDataActual); i++){
        charDataActual[i] = dataActual[i];
        if (charDataActual[i] == 'v'){ //partition for ActTemp Data is v
          end_ActTemp = i;
        }else if (charDataActual[i] == 'w'){ //partition for ActTemp Data is w
          end_ActHum = i;
        }else if (charDataActual[i] == 'x'){ //partition for ActTemp Data is x
          end_ActMoist = i;
        }else if (charDataActual[i] == 'y'){ //partition for ActTemp Data is y
          end_ActLight = i;
        }
      }

      //Convert array of char(charDataActual) to float for each parameters
      for (i=0; i<end_ActTemp; i++){
        ActTemp[j] = charDataActual[i];
        j++;
      }
      fActTemp = atof(ActTemp);
//      Serial.println();
//      Serial.print("ActTemp = ");
//      Serial.print(fActTemp);
      j=0;

      for (i=end_ActTemp+1; i<end_ActHum; i++){
        ActHum[j] = charDataActual[i];
        j++;
      }
      fActHum = atof(ActHum);
//      Serial.println();
//      Serial.print("ActHum = ");
//      Serial.print(fActHum);
      j=0;

      for (i=end_ActHum+1; i<end_ActMoist; i++){
        ActMoist[j] = charDataActual[i];
        j++;
      }
      fActMoist = atof(ActMoist);
//      Serial.println();
//      Serial.print("ActMoist = ");
//      Serial.print(fActMoist);
      j=0;

      for (i=end_ActMoist+1; i<end_ActLight; i++){
        ActLight[j] = charDataActual[i];
        j++;
      }
      fActLight = atof(ActLight);
//      Serial.println();
//      Serial.print("ActLight = ");
//      Serial.print(fActLight);
      j=0;

      Serial.println();

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (5000);
  }
}

void lightTask(void *pvParam){
  (void) pvParam;
  while(1){
    light_control();
    dtostrf(ledState,1,2,lightS);
    if (digitalRead(LED) == HIGH){
      light_status = "ON";
    }else{
      light_status = "OFF";
    }
    
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
        digitalWrite(KIPAS_COOLER,LOW);
    }
    else if (F_COOLER){
        heaterDutyCycle = 0;
        coolerState = 1;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,HIGH);
        digitalWrite(KIPAS_COOLER,HIGH);
    }
    else{
        temperature_control();
    }
    dimmer.setPower(heaterDutyCycle);
    dtostrf(heaterState,1,2,heaterS);
    dtostrf(coolerState,1,2,coolerS);
    if (digitalRead(KIPAS_HEATER) == HIGH){
      heater_status = "ON";
    }else{
      heater_status = "OFF";
    }
    if (digitalRead(COOLER) == HIGH){
      cooler_status = "ON";
    }else{
      cooler_status = "OFF";
    }
    
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
    
    dtostrf(humidifierState,1,2,humidS);
    dtostrf(dehumidifierState,1,2,dehumidS);
    
    if (digitalRead(HUMIDIFIER) == HIGH){
      humid_status = "ON";
    }else{
      humid_status = "OFF";
    }
    if (digitalRead(DEHUMIDIFIER) == HIGH){
      dehumid_status = "ON";
    }else{
      dehumid_status = "OFF";
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
    
    dtostrf(pumpState,1,2,pumpS);
    
    if (digitalRead(MICROPUMP) == HIGH){
      pump_status = "ON";
    }else{
      pump_status = "OFF";
    }
    vTaskDelay(2000);
  }
}

void print_status(void *pvParam){
  (void) pvParam;
  while(1){
   
    // for checking actuator status
    Serial.println("Heater Status : " + heater_status); 
    Serial.println("Cooler Status : " + cooler_status); 
    Serial.println("Humidifier Status : " + humid_status); 
    Serial.println("Dehumidifier Status : " + dehumid_status); 
    Serial.println("Pump Status : " + pump_status); 
    Serial.println("Light Status : " + light_status); 
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
    
    vTaskDelay(3000);
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
        digitalWrite(KIPAS_COOLER,LOW);
        if (fActTemp >= OptTemp) {
          tempState = WAIT_TEMP;
        }
    }
    else if (tempState == WAIT_TEMP){
        heaterDutyCycle = 0;
        coolerState = 0;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,LOW);
        digitalWrite(KIPAS_COOLER,LOW);
        if((OptTemp - fActTemp)>= TEMP_THRESHOLD){
          tempState = HEAT;
        }
        else if((fActTemp - OptTemp)>= TEMP_THRESHOLD){
          tempState = COOL;
        }
    }
    else if (tempState == COOL) {
        heaterDutyCycle = 0;
        coolerState = 1;
        heaterState = 0;
        digitalWrite(KIPAS_HEATER,LOW);
        digitalWrite(COOLER,HIGH);
        digitalWrite(KIPAS_COOLER,HIGH);
        if (fActTemp <= OptTemp){
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
        if (fActHum >= OptHum) {
          humidState = WAIT_HUMID;
        }
    }
    else if (humidState == WAIT_HUMID){
        humidifierState = 0;
        dehumidifierState = 0;
        digitalWrite(HUMIDIFIER,LOW);
        digitalWrite(DEHUMIDIFIER,LOW);
        digitalWrite(KIPAS_DEHUMIDIFIER,LOW);
        if((OptHum - fActHum)>= HUMID_THRESHOLD){
          humidState = HUMID;
        }
        else if((fActHum - OptHum)>= HUMID_THRESHOLD){
          humidState = DEHUMID;
        }
    }
    else if (humidState == DEHUMID) {
        humidifierState = 0;
        dehumidifierState = 1;
        digitalWrite(HUMID,LOW);
        digitalWrite(DEHUMIDIFIER,HIGH);
        digitalWrite(KIPAS_DEHUMIDIFIER,HIGH);
        if (fActHum <= OptHum){
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
        if (((millis() - activeStart) >= (DELAY_WAIT * 1000)) && (fActMoist <= OptMoist)){
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
