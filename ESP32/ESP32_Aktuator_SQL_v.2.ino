/*Program for ESP-A
Created by: Josephine Ariella*/

/*Update:
 * Temp & Hum actuators are using PI Controller
 * All digital pins are ready to use
 * Using Router TP-Link_F4A6 for HTTP Protocol
 * Using RTC for Light Actuator
 */


//Global Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include "RBDdimmerESP32.h"
#include "RTClib.h"

//HTTP Protocol Access Point
//const char* ssid     = "pipino";
//const char* password = "cilukbaa";
const char* ssid     = "TP-Link_F4A6";
const char* password = "15798039";

//Server SQL & API Key identification
//const char* server_fromSQL  = "http://192.168.0.100/SQL_toESP-A.php"; //TP_Link-F4A6 Raspi
const char* server_fromSQL    = "http://192.168.0.100/phpmyadmin/SQL_toESP-A.php"; //TP_Link-F4A6 Xampp Windows 10

//const char* server_fromSQL  = "http://192.168.43.159/SQL_toESP-A.php"; //pipino raspi
//const char* server_fromSQL  = "http://192.168.43.184/phpmyadmin/SQL_toESP-A.php"; //pipino Xampp Windows 10

//const char* server_toSQL    = "http://192.168.0.102/ESP-A_toSQL.php"; //TP_Link-F4A6 Raspi
String apiKeyValue          = "tPmAT5Ab3j7F9";


// Pin Definitions 
//yg udah fix
    #define HEATER_PWM          25 //jalan
    #define HEATER_ZC           27 //jalan
    #define KIPAS_HEATER        26 //jalan
    #define KIPAS_COOLER        19 //jalan
//pinnya aman tapi blm fix disitu pinnya
    #define LED                 2 //pinnya aman
    #define COOLER_PLT          5 //pinnya aman
    #define COOLER_RAD          18 //pinnya aman
    //D4 untuk cooler
    //D15, D23, D12, D13, D14,D33, D32   bisa dipake
    #define MICROPUMP           32
    #define DEHUMIDIFIER        13
    #define KIPAS_DEHUMIDIFIER  26
    #define HUMIDIFIER          33

    
//Variables & Declaration for Heater Driver
    dimmerLampESP32 dimmer(HEATER_PWM, HEATER_ZC);

//Variables & Declaration for RTC Module
    RTC_DS3231 rtc;
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    int morning_startHour = 9;
    int morning_startMinute = 00;
    int morning_startSecond = 00;
    int noon_startHour = 11;
    int noon_startMinute = 00;
    int noon_startSecond = 00;
    int afternoon_startHour = 15;
    int afternoon_startMinute = 00;
    int afternoon_startSecond = 00;
    int evening_startHour = 18;
    int evening_startMinute = 00;
    int evening_startSecond = 00;
    int light_stopHour = 21;
    int light_stopMinute = 00;
    int light_stopSecond = 00;

//Setting PWM properties
    const unsigned long int FREQ_KIPAS_HEATER        = 500000;//Threshold Dutycyle starts from 50/255
//    const unsigned long int FREQ_KIPAS_COOLER        = 200000; //Threshold Dutycycle starts from 41-45/255
    const unsigned long int FREQ_KIPAS_COOLER        = 250000; //Threshold Dutycycle starts from 27-31/255
//    const unsigned long int FREQ_KIPAS_COOLER        = 150000; //Threshold Dutycycle starts from 95-97/255
    const unsigned long int FREQ_HUMIDIFIER          = 500000; //blm dicoba
    const unsigned long int FREQ_DEHUMIDIFIER        = 500000; //blm dicoba
    const unsigned long int FREQ_MICROPUMP           = 500000; //Threshold Dutycycle starts from 80/255
    const unsigned long int FREQ_LED                 = 50000; //Freq LED 50000 --> bisa kecilan juga
    const int CHANNEL_KIPAS_HEATER  = 0;
    const int CHANNEL_KIPAS_COOLER  = 1;
    const int CHANNEL_HUMIDIFIER  = 2;
    const int CHANNEL_DEHUMIDIFIER  = 3;
    const int CHANNEL_MICROPUMP = 4;
    const int CHANNEL_LED = 4;
    const int RES  = 8;
    int min_dutyCycle     = 0;
    int max_dutyCycle     = 255;

//Setting PID Controller
    const float Kp = 10.0;
    const float Ki = 10.0;
    float error_integral_heater, error_integral_cooler, error_integral_humidifier, error_integral_dehumidifier = 0;
    int heater_dutyCycle, dimmer_dutyCycle, cooler_dutyCycle, humidifier_dutyCycle, dehumidifier_dutyCycle;
    int pump_dutyCycle_on = 180; //cari dutycycle yang cukup saat dinyalakan selama 2 menit --> durasi lama data aktual baru datang
    int pump_dutyCycle_off = 50;
    int led_dutycycle = 0;
    float error_heater, error_cooler, error_humidifier, error_dehumidifier, error_pump;

//Global Variable Initialization
    //Variables for convert data (actual, optimal, manual) from SQL
    int i, k, end_ActTemp, end_ActHum, end_ActMoist1, end_ActMoist2, end_ActMoist3, end_ActMoist4, end_ActMoist5, end_ActMoist6, end_ActLight;
    int end_OptTemp, end_OptHum, end_OptMoist, end_OptLight;
    int j = 0;
    char ActTemp[5], ActHum[5], ActMoist1[5], ActMoist2[5], ActMoist3[5], ActMoist4[5], ActMoist5[5], ActMoist6[5], ActLight[10];
    char OptTemp[5], OptHum[5], OptMoist[5], OptLight[10];
    //Variables for Actual & Optimal datas in float & Manual datas in String
    float fActTemp, fActHum, fActMoist1,  fActMoist2,  fActMoist3,  fActMoist4,  fActMoist5,  fActMoist6, fActLight;
    float fOptTemp, fOptHum, fOptMoist, fOptLight;
    //for pump purpose
    long activeStart;

//Task Priority for RTOS
//Greater number means more important
#define priorityTask1 5 //LED
#define priorityTask2 4 //Temperature
#define priorityTask3 3 //Humidity
#define priorityTask4 2 //Moisture
#define priorityTask5 1 //printStatus
#define priorityTask6 6 //getting data from SQL

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
//  pinMode(LED, OUTPUT);
  pinMode(COOLER_PLT, OUTPUT);
  pinMode(COOLER_RAD, OUTPUT);
//  pinMode(DEHUMIDIFIER, OUTPUT);
  pinMode(KIPAS_DEHUMIDIFIER, OUTPUT);
//  pinMode(HUMIDIFIER, OUTPUT);
  dimmer.begin(NORMAL_MODE, ON);
  
  // configure PWM PIN functionalitites
  ledcSetup(CHANNEL_KIPAS_HEATER, FREQ_KIPAS_HEATER, RES);
  ledcAttachPin(KIPAS_HEATER, CHANNEL_KIPAS_HEATER);
  ledcSetup(CHANNEL_KIPAS_COOLER, FREQ_KIPAS_COOLER, RES);
  ledcAttachPin(KIPAS_COOLER, CHANNEL_KIPAS_COOLER);
  ledcSetup(CHANNEL_HUMIDIFIER, FREQ_HUMIDIFIER, RES);
  ledcAttachPin(HUMIDIFIER, CHANNEL_HUMIDIFIER);
  ledcSetup(CHANNEL_DEHUMIDIFIER, FREQ_DEHUMIDIFIER, RES);
  ledcAttachPin(DEHUMIDIFIER, CHANNEL_DEHUMIDIFIER);
  ledcSetup(CHANNEL_MICROPUMP, FREQ_MICROPUMP, RES);
  ledcAttachPin(MICROPUMP, CHANNEL_MICROPUMP);
  ledcSetup(CHANNEL_LED, FREQ_LED, RES);
  ledcAttachPin(LED, CHANNEL_LED);

  //RTOS Configuration
  xTaskCreatePinnedToCore( get_SQL_data , "Task 5" , 2048 , NULL , priorityTask6 , NULL , 0);
  xTaskCreatePinnedToCore( lightTask , "Task 1" , 2048 , NULL , priorityTask1 , NULL , 1);
  xTaskCreatePinnedToCore( temperatureTask , "Task 2" , 2048 , NULL , priorityTask2 , NULL , 1);
  xTaskCreatePinnedToCore( humidityTask , "Task 3" , 2048 , NULL , priorityTask3 , NULL , 0);
  xTaskCreatePinnedToCore( moistureTask , "Task 4" , 2048 , NULL , priorityTask4 , NULL , 1);
  xTaskCreatePinnedToCore( print_status , "Task 6" , 2048 , NULL , priorityTask5 , NULL , 0);
  
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
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  rtc.adjust(DateTime(__DATE__, __TIME__));
  delay(3000);
  
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
      Serial.println("-----------------");


    //Convert string data(DataFromSQL) to array of char(charDataFromSQL)
    char charDataFromSQL[DataFromSQL.length()];
    for (i = 0; i< sizeof(charDataFromSQL); i++){
      charDataFromSQL[i] = DataFromSQL[i];
      if (charDataFromSQL[i] == 'a'){ //partition for ActTemp Data is a
        end_ActTemp = i;
      }else if (charDataFromSQL[i] == 'b'){ //partition for ActHum Data is b
        end_ActHum = i;
      }else if (charDataFromSQL[i] == 'c'){ //partition for ActMoist1 Data is c
        end_ActMoist1 = i;
      }else if (charDataFromSQL[i] == 'd'){ //partition for ActMoist2 Data is d
        end_ActMoist2 = i;
      }else if (charDataFromSQL[i] == 'e'){ //partition for ActMoist3 Data is e
        end_ActMoist3 = i;
      }else if (charDataFromSQL[i] == 'f'){ //partition for ActMoist4 Data is f
        end_ActMoist4 = i;
      }else if (charDataFromSQL[i] == 'g'){ //partition for ActMoist5 Data is g
        end_ActMoist5 = i;
      }else if (charDataFromSQL[i] == 'h'){ //partition for ActMoist6 Data is h
        end_ActMoist6 = i;
      }else if (charDataFromSQL[i] == 'i'){ //partition for ActLight Data is i
        end_ActLight = i;
      }else if (charDataFromSQL[i] == 'j'){ //partition for OptTemp Data is j
        end_OptTemp = i;
      }else if (charDataFromSQL[i] == 'k'){ //partition for OptHum Data is k
        end_OptHum = i;
      }else if (charDataFromSQL[i] == 'l'){ //partition for OptMoist Data is l
        end_OptMoist = i;
      }else if (charDataFromSQL[i] == 'm'){ //partition for OptLight Data is m
        end_OptLight = i;
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

      for (i=end_ActHum+1; i<end_ActMoist1; i++){
        ActMoist1[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist1 = atof(ActMoist1);
      j=0;

      for (i=end_ActMoist1+1; i<end_ActMoist2; i++){
        ActMoist2[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist2 = atof(ActMoist2);
      j=0;

      for (i=end_ActMoist2+1; i<end_ActMoist3; i++){
        ActMoist3[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist3 = atof(ActMoist3);
      j=0;

      for (i=end_ActMoist3+1; i<end_ActMoist4; i++){
        ActMoist4[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist4 = atof(ActMoist4);
      j=0;

      for (i=end_ActMoist4+1; i<end_ActMoist5; i++){
        ActMoist5[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist5 = atof(ActMoist5);
      j=0;

      for (i=end_ActMoist5+1; i<end_ActMoist6; i++){
        ActMoist6[j] = charDataFromSQL[i];
        j++;
      }
      fActMoist6 = atof(ActMoist6);
      j=0;

      for (i=end_ActMoist6+1; i<end_ActLight; i++){
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

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (5000); //Pengiriman Data setiap 5 DETIK SEKALI
  }
}


void print_status(void *pvParam){
  (void) pvParam;
  while(1){
    // for checking actuator status
//    Serial.print("ActTemp = ");
//    Serial.println(fActTemp);
//    Serial.print("ActHum = ");
//    Serial.println(fActHum);
//    Serial.print("ActMoist1 = ");
//    Serial.println(fActMoist1);    
//    Serial.print("ActMoist2 = ");
//    Serial.println(fActMoist2);   
//    Serial.print("ActMoist3 = ");
//    Serial.println(fActMoist3);   
//    Serial.print("ActMoist4 = ");
//    Serial.println(fActMoist4);   
//    Serial.print("ActMoist5 = ");
//    Serial.println(fActMoist5);
//    Serial.print("ActMoist6 = ");
//    Serial.println(fActMoist6);        
//    Serial.print("ActLight = ");
//    Serial.println(fActLight);
//    Serial.println("-----------------");
//    Serial.print("OptTemp = ");
//    Serial.println(fOptTemp);
//    Serial.print("OptHum = ");
//    Serial.println(fOptHum);
//    Serial.print("OptMoist = ");
//    Serial.println(fOptMoist);      
//    Serial.print("OptLight = ");
//    Serial.println(fOptLight);
//    Serial.println("-----------------");
//    Serial.print("Heater Dutycycle = ");
//    Serial.println(heater_dutyCycle);
//    Serial.print("Dimmer Dutycycle = ");
//    Serial.println(dimmer_dutyCycle);
//    Serial.print("Cooler Dutycycle = ");
//    Serial.println(cooler_dutyCycle);
//    if (error_cooler > 0){
//      Serial.println("Peltier DutyCycle = 100");
//    }else if (error_cooler <=0){
//      Serial.println("Peltier DutyCycle = 0");
//    }
//    Serial.print("Humidifier Dutycycle = ");
//    Serial.println(humidifier_dutyCycle);
//    Serial.print("Dehumidifier Dutycycle = ");
//    Serial.println(dehumidifier_dutyCycle);
//    if(error_pump > 0){
//      Serial.println("Pump status = ON");
//    }else{
//      Serial.println("Pump status = OFF");
//    }
    if(led_dutycycle == 180){
      Serial.println("LED status = MORNING");
    }else if(led_dutycycle == 255){
      Serial.println("LED status = NOON");
    }else if(led_dutycycle == 150){
      Serial.println("LED status = AFTERNOON");
    }else if(led_dutycycle == 80){
      Serial.println("LED status = EVENING");
    }else if(led_dutycycle == 50){
      Serial.println("LED status = OFF");
    }
    Serial.println("-----------------");
    
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
    temperature_control();
    
    vTaskDelay(2500);
  }
}

//Humidity Control Procedure Call
void humidityTask(void *pvParam){
  (void) pvParam;
  while(1){
    humidity_control();
    
    vTaskDelay(2500);
  }
}

void moistureTask(void *pvParam){
  (void) pvParam;
  while(1){
    moisture_control();

    vTaskDelay(2000);
  }
}

void temperature_control(){
    //Heater Controller
    error_heater = fOptTemp - fActTemp;
    if (error_heater > 0) {
      dimmer_dutyCycle = 100;
    }else if (error_heater <= 0){
      dimmer_dutyCycle = 0;
    }
    error_integral_heater += Ki*error_heater;
    heater_dutyCycle = floor(Kp*error_heater + error_integral_heater);
    
    if (heater_dutyCycle > 255){
      heater_dutyCycle = 255;
      error_integral_heater = 255.0;
    }else if(heater_dutyCycle < 0){
      heater_dutyCycle = 0;
      error_integral_heater = 0.0;
    }

    //Cooler Controller
    error_cooler = fActTemp - fOptTemp;
    error_integral_cooler += Ki*error_cooler;
    cooler_dutyCycle = floor(Kp*error_cooler + error_integral_cooler);
    if (cooler_dutyCycle > 255){
      cooler_dutyCycle = 255;
      error_integral_cooler = 255.0;
    }else if(cooler_dutyCycle < 0){
      cooler_dutyCycle = 0;
      error_integral_cooler = 0.0;
    }
    
	//Define Dutycycle for Heater & Cooler
    dimmer.setPower(dimmer_dutyCycle);
    ledcWrite(CHANNEL_KIPAS_HEATER, heater_dutyCycle);
    ledcWrite(CHANNEL_KIPAS_COOLER, cooler_dutyCycle);
    if (error_cooler > 0){
      digitalWrite(COOLER_PLT, HIGH);
      digitalWrite(COOLER_RAD, HIGH);
    }else if (error_cooler <=0){
      digitalWrite(COOLER_PLT, LOW);
      digitalWrite(COOLER_RAD, LOW);
    }
}

void humidity_control(){
    //Humidifier Controller
    error_humidifier = fOptHum - fActHum;
    error_integral_humidifier += Ki*error_humidifier;
    humidifier_dutyCycle = floor(Kp*error_humidifier + error_integral_humidifier);
    
    if (humidifier_dutyCycle > 255){
      humidifier_dutyCycle = 255;
      error_integral_humidifier = 255.0;
    }else if(humidifier_dutyCycle < 0){
      humidifier_dutyCycle = 0;
      error_integral_humidifier = 0.0;
    }

    //Dehumidifier Controller
    error_dehumidifier = fActHum - fOptHum;
    error_integral_dehumidifier += Ki*error_dehumidifier;
    dehumidifier_dutyCycle = floor(Kp*error_dehumidifier + error_integral_dehumidifier);
    if (dehumidifier_dutyCycle > 255){
      dehumidifier_dutyCycle = 255;
      error_integral_dehumidifier = 255.0;
    }else if(dehumidifier_dutyCycle < 0){
      dehumidifier_dutyCycle = 0;
      error_integral_dehumidifier = 0.0;
    }
    
	//Define Dutycycle for Humidifier & Dehumidifier
    ledcWrite(CHANNEL_HUMIDIFIER, humidifier_dutyCycle);
    ledcWrite(CHANNEL_DEHUMIDIFIER, dehumidifier_dutyCycle);
}

void moisture_control(){ 
	//Pump Controller
  DateTime now = rtc.now();
	error_pump = fOptMoist - fActMoist1;
	if (error_pump > 0){
		ledcWrite(CHANNEL_MICROPUMP, pump_dutyCycle_on);
	}else{
		ledcWrite(CHANNEL_MICROPUMP, pump_dutyCycle_off);
	}
}

void light_control(){
	//LED Controller
	
	//Activating RTC Module
	DateTime now = rtc.now();
	Serial.print("Current Time = ");
	Serial.print(now.hour());
	Serial.print(":");  
	Serial.print(now.minute());
	Serial.print(":");
	Serial.println(now.second());
  //Morning: 9-11
 if (((now.hour() == morning_startHour) && (now.minute() == morning_startMinute)) || ((now.hour() < noon_startHour) && (now.hour() > light_stopHour))){
    led_dutycycle = 180;
    ledcWrite(CHANNEL_LED, led_dutycycle);
//    Serial.println("LED status = MORNING");
  } else
  //Noon: 11-15
  if (((now.hour() == noon_startHour) && (now.minute() == noon_startMinute)) || ((now.hour() < afternoon_startHour) && (now.hour() > morning_startHour))){
    led_dutycycle = 255;
    ledcWrite(CHANNEL_LED, led_dutycycle);
//    Serial.println("LED status = NOON");
  } else
  //Afternoon 15-18
  if (((now.hour() == afternoon_startHour) && (now.minute() == afternoon_startMinute)) || ((now.hour() < evening_startHour) && (now.hour() > noon_startHour))){
    led_dutycycle = 150;
    ledcWrite(CHANNEL_LED, led_dutycycle);
//    Serial.println("LED status = AFTERNOON");
  } else
  //Evening: 18-21
  if (((now.hour() == evening_startHour) && (now.minute() == evening_startMinute)) || ((now.hour() < light_stopHour) && (now.hour() > afternoon_startHour))){
    led_dutycycle = 80;
    ledcWrite(CHANNEL_LED, led_dutycycle);
//    Serial.println("LED status = EVENING");
  }  else
  if (((now.hour() == light_stopHour) && (now.minute() == light_stopMinute)) || ((now.hour() < morning_startHour) && (now.hour() > evening_startHour))){
    led_dutycycle = 50;
    ledcWrite(CHANNEL_LED, led_dutycycle);
//    Serial.println("LED status = OFF");
  }
  Serial.println("-----------------");
}
