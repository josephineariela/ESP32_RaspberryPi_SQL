/*Program for ESP-A
Created by: Josephine Ariella*/

/*Global Library*/
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <PubSubClient.h>
#include "RBDdimmerESP32.h"
#include "RTClib.h"

/*HTTP Protocol Access Point*/
const char* ssid = "pipino";
const char* password = "cilukbaa";
//const char* ssid = "Opor Ayam";
//const char* password = "10110678";

/*Server SQL & API Key identification*/
const char* server_from_OffSQL          = "http://192.168.43.184/phpmyadmin/SQL_toESP-A.php"; //getting actualdata & optimumdata from local server
const char* server_from_OnSQL           = "http://35.240.254.221/SQL_toESP-A.php"; //getting actualdata & optimumdata from online server

const char* server_to_OffSQL_ManData    = "http://192.168.43.184/phpmyadmin/ESP-A_toSQL.php"; //sending manualdata to local server
const char* server_to_OnSQL_ManData     = "http://35.240.254.221/ESP-A_toSQL.php"; //manualdata
const char* server_to_OffSQL_OptData    = "http://192.168.43.184/phpmyadmin/ESP-A_toSQL_2.php"; //sending optimumdata to local server
String apiKeyValue          = "tPmAT5Ab3j7F9";

/*MQTT Declarations*/
WiFiClient espClient;
PubSubClient client(espClient);
//const char* mqttServer = "tailor.cloudmqtt.com";
//const int mqttPort = 14131;
//const char* mqttUser = "hhaqsitb";
//const char* mqttPassword = "MP7TFv0i040Q";
const char* mqttServer = "tailor.cloudmqtt.com";
const int mqttPort = 15478;
const char* mqttUser = "jnnmjrfb";
const char* mqttPassword = "fTHdJEKxaFTx";

/*Pin Declaration*/
    #define HEATER_PWM              2
    #define HEATER_ZC               15
    //SWITCH-BD140
    #define MICROPUMP1              13 //pwm
    #define MICROPUMP2              12 //pwm
    #define MICROPUMP3              14 //pwm
    #define MICROPUMP4              25 //pwm
    #define MICROPUMP5              26 //pwm
    #define LED                     27 //switch
    #define KIPAS_HEATER            33 //pwm
    #define KIPAS_COOLER            32 //pwm
    #define KIPAS_RAD_COOLER        23 //switch
    #define PUMP_COOLER             19 //switch
    #define KIPAS_HUMIDIFIER        18 //pwm
    #define KIPAS_DEHUMIDIFIER      5  //pwm
    #define KIPAS_PLT_DEHUMIDIFIER  26  //switch
    //SWITCH-RELAY
    #define PLT_COOLER              17 //switch
    #define MIST_HUMIDIFIER         16 //switch
    #define PLT_DEHUMIDIFIER        4  //switch

    
/*Variables & Declaration for Heater Driver*/
    dimmerLampESP32 dimmer(HEATER_PWM, HEATER_ZC);

/*Variables & Declaration for RTC Module*/
    RTC_DS3231 rtc;
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/*Setting PWM properties*/
    const int RES                           = 8;
    const unsigned long int FREQ_MICROPUMP           = 500000; //Threshold Dutycycle starts from 80/255
    const unsigned long int FREQ_KIPAS_HEATER        = 500000;//Threshold Dutycyle starts from 50/255
    const unsigned long int FREQ_KIPAS_COOLER        = 500000; //Threshold Dutycycle starts from 27-31/255
    const unsigned long int FREQ_KIPAS_HUMIDIFIER    = 500000; //blm dicoba
    const unsigned long int FREQ_KIPAS_DEHUMIDIFIER  = 500000; //blm dicoba
    
    const int CHANNEL_MICROPUMP1            = 0;
    const int CHANNEL_MICROPUMP2            = 1;
    const int CHANNEL_MICROPUMP3            = 2;
    const int CHANNEL_MICROPUMP4            = 3;
    const int CHANNEL_MICROPUMP5            = 4;
    const int CHANNEL_KIPAS_HEATER          = 5;
    const int CHANNEL_KIPAS_COOLER          = 6;
    const int CHANNEL_KIPAS_HUMIDIFIER      = 7;
    const int CHANNEL_KIPAS_DEHUMIDIFIER    = 8;

/*Manual Control Variables*/
    #define DISABLE 0
    #define ENABLE 1
    int F_LED = DISABLE;
    int F_HEATER = DISABLE;
    int F_COOLER = DISABLE;
    int F_DEHUMI = DISABLE;
    int F_HUMI = DISABLE;
    int F_PUMP1 = DISABLE;
    int F_PUMP2 = DISABLE;
    int F_PUMP3 = DISABLE;
    int F_PUMP4 = DISABLE;
    int F_PUMP5 = DISABLE;
    
/*Man / Auto Control Identification*/
    String temperature_mode = "auto";
    String humidity_mode = "auto";
    String moisture_mode = "auto";
    String light_mode = "auto";

/*Setting PID Controller*/
    const float Kp = 10.0;
    const float Ki = 0.2;
    const float Kd = 0.01;
    int max_dutyCycle = 255;
    int min_dutyCycle = 0;
    float tolerance_on = 1.0; //error>1 baru aktuator nyala
    float tolerance_off = 0.5; //error<=0.5 baru aktuator mati
    float error_heater, error_cooler, error_humidifier, error_dehumidifier;
    float error_pump1, error_pump2,error_pump3, error_pump4, error_pump5;
    float error_sum_heater, error_sum_cooler, error_sum_humidifier, error_sum_dehumidifier = 0;
    float error_sum_pump1, error_sum_pump2, error_sum_pump3, error_sum_pump4, error_sum_pump5 = 0;
    float error_last_heater, error_last_cooler, error_last_humidifier, error_last_dehumidifier = 0;
    float error_last_pump1, error_last_pump2, error_last_pump3, error_last_pump4, error_last_pump5 = 0;
    float error_rate_heater, error_rate_cooler, error_rate_humidifier, error_rate_dehumidifier = 0;
    float error_rate_pump1, error_rate_pump2, error_rate_pump3, error_rate_pump4, error_rate_pump5 = 0;
    int heater_dutyCycle, dimmer_dutyCycle, cooler_dutyCycle, humidifier_dutyCycle, dehumidifier_dutyCycle;
    int pump1_dutyCycle, pump2_dutyCycle, pump3_dutyCycle, pump4_dutyCycle, pump5_dutyCycle;
    int heater_dutyCycle_initial = 40;
    int cooler_dutyCycle_initial = 150;
    int pump1_dutyCycle_initial = 50;
    int pump2_dutyCycle_initial = 50;
    int pump3_dutyCycle_initial = 50;
    int pump4_dutyCycle_initial = 50;
    int pump5_dutyCycle_initial = 50;
    
/*Global Variable Initialization*/
    /*Variables for convert data (actual, optimal, manual) from SQL*/
    int i, k, end_ActTemp, end_ActHum, end_ActMoist1, end_ActMoist2, end_ActMoist3, end_ActMoist4, end_ActMoist5, end_ActLight;
    int end_OptTemp, end_OptHum, end_OptMoist, end_OptLightStart1, end_OptLightStart2, end_OptLightEnd1, end_OptLightEnd2;
    int j = 0;
    char ActTemp[5], ActHum[5], ActMoist1[5], ActMoist2[5], ActMoist3[5], ActMoist4[5], ActMoist5[5], ActLight[10];
    char OptTemp[5], OptHum[5], OptMoist[5], OptLightStart1[10], OptLightStart2[10], OptLightEnd1[10], OptLightEnd2[10];
    /*Variables for Actual & Optimal datas in float & Manual datas in String*/
    float fActTemp, fActHum, fActMoist1,  fActMoist2,  fActMoist3,  fActMoist4,  fActMoist5, fActLight;
    float fOptTemp, fOptHum, fOptMoist, fOptLightStart1, fOptLightStart2, fOptLightEnd1, fOptLightEnd2;
    String sActTemp, sActHum, sActMoist1,  sActMoist2,  sActMoist3,  sActMoist4,  sActMoist5, sActLight = " ";
    String sOptTemp, sOptHum, sOptMoist, sOptLightStart, sOptLightStart1, sOptLightStart2, sOptLightEnd, sOptLightEnd1, sOptLightEnd2 = " ";
    int iOptLightStart1, iOptLightStart2, iOptLightEnd1, iOptLightEnd2, iStopLight;
    bool lightActive = false;
    String heater_status_resp, cooler_status_resp, humi_status_resp, dehumi_status_resp,pump1_status_resp, pump2_status_resp, pump3_status_resp, pump4_status_resp, pump5_status_resp = "OFF";
    String led_status_resp = "OFF";
    
/*Task Priority for RTOS*/
//Greater number means more important
#define priorityTask1 6 //LED
#define priorityTask2 5 //Temperature
#define priorityTask3 4 //Humidity
#define priorityTask4 3 //Moisture
#define priorityTask5 1 //printStatus
#define priorityTask6 7 //getting data from SQL
#define priorityTask7 2 //sending data to SQL

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  /*RTC Module Identification*/
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  rtc.adjust(DateTime(__DATE__, __TIME__));
  
  pinMode(LED, OUTPUT);
  pinMode(PLT_COOLER, OUTPUT);
  pinMode(KIPAS_RAD_COOLER, OUTPUT);
  pinMode(PUMP_COOLER, OUTPUT);
  pinMode(MIST_HUMIDIFIER, OUTPUT);
  pinMode(PLT_DEHUMIDIFIER, OUTPUT);
  pinMode(KIPAS_PLT_DEHUMIDIFIER, OUTPUT);
  dimmer.begin(NORMAL_MODE, ON);
  
  /* configure PWM PIN functionalitites*/
  ledcSetup(CHANNEL_MICROPUMP1, FREQ_MICROPUMP, RES);
  ledcSetup(CHANNEL_MICROPUMP2, FREQ_MICROPUMP, RES);
  ledcSetup(CHANNEL_MICROPUMP3, FREQ_MICROPUMP, RES);
  ledcSetup(CHANNEL_MICROPUMP4, FREQ_MICROPUMP, RES);
  ledcSetup(CHANNEL_MICROPUMP5, FREQ_MICROPUMP, RES);
  ledcAttachPin(MICROPUMP1, CHANNEL_MICROPUMP1);
  ledcAttachPin(MICROPUMP2, CHANNEL_MICROPUMP2);
  ledcAttachPin(MICROPUMP3, CHANNEL_MICROPUMP3);
  ledcAttachPin(MICROPUMP4, CHANNEL_MICROPUMP4);
  ledcAttachPin(MICROPUMP5, CHANNEL_MICROPUMP5);
  ledcSetup(CHANNEL_KIPAS_HEATER, FREQ_KIPAS_HEATER, RES);
  ledcAttachPin(KIPAS_HEATER, CHANNEL_KIPAS_HEATER);
  ledcSetup(CHANNEL_KIPAS_COOLER, FREQ_KIPAS_COOLER, RES);
  ledcAttachPin(KIPAS_COOLER, CHANNEL_KIPAS_COOLER);
  ledcSetup(CHANNEL_KIPAS_HUMIDIFIER, FREQ_KIPAS_HUMIDIFIER, RES);
  ledcAttachPin(KIPAS_HUMIDIFIER, CHANNEL_KIPAS_HUMIDIFIER);
  ledcSetup(CHANNEL_KIPAS_DEHUMIDIFIER, FREQ_KIPAS_DEHUMIDIFIER, RES);
  ledcAttachPin(KIPAS_DEHUMIDIFIER, CHANNEL_KIPAS_DEHUMIDIFIER);

  /*RTOS Configuration*/
  xTaskCreatePinnedToCore( get_SQL_data , "Task 5" , 2048 , NULL , priorityTask6 , NULL , 0);
  xTaskCreatePinnedToCore( lightTask , "Task 1" , 2048 , NULL , priorityTask1 , NULL , 1);
  xTaskCreatePinnedToCore( temperatureTask , "Task 2" , 2048 , NULL , priorityTask2 , NULL , 1);
  xTaskCreatePinnedToCore( humidityTask , "Task 3" , 2048 , NULL , priorityTask3 , NULL , 0);
  xTaskCreatePinnedToCore( moistureTask , "Task 4" , 2048 , NULL , priorityTask4 , NULL , 1);
  xTaskCreatePinnedToCore( print_status , "Task 6" , 2048 , NULL , priorityTask5 , NULL , 0);
  xTaskCreatePinnedToCore( send_SQL_data, "Task 7", 2048, NULL, priorityTask7, NULL, 1);
  
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
 
  delay(3000);
  
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  int messageInt;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  messageInt = messageTemp.toFloat();
  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "temperature_mode"){
    Serial.print("Control Temperature : ");
    Serial.println(messageTemp);
    if (messageTemp == "auto"){
        temperature_mode = "auto";
    }
    else if (messageTemp == "man"){
        temperature_mode = "man";
    }
  }
  else if (String(topic) == "humidity_mode"){
    Serial.print("Control Humidity : ");
    Serial.println(messageTemp);
    if (messageTemp == "auto"){
        humidity_mode = "auto";
    }
    else if (messageTemp == "man"){
        humidity_mode = "man";
    }
  }
  else if (String(topic) == "moisture_mode"){
    Serial.print("Control Moisture : ");
    Serial.println(messageTemp);
    if (messageTemp == "auto"){
        moisture_mode = "auto";
    }
    else if (messageTemp == "man"){
        moisture_mode = "man";
    }
  }
  else if (String(topic) == "light_mode"){
    Serial.print("Control Light : ");
    Serial.println(messageTemp);
    if (messageTemp == "auto"){
        light_mode = "auto";
    }
    else if (messageTemp == "man"){
        light_mode = "man";
    }
  }
  else if ((String(topic) == "light")){
    Serial.print("Light Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_LED = ENABLE;
    }
    else if (messageTemp == "off"){
        F_LED = DISABLE;
    }
  }
  else if ((String(topic) == "heater")){
    Serial.print("Heater Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_HEATER = ENABLE;
        F_COOLER = DISABLE;
    }
    else if (messageTemp == "off")
    {
       F_HEATER = DISABLE;
    }
  }
  else if ((String(topic) == "cooler")){
    Serial.print("Cooler Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_COOLER = ENABLE;
        F_HEATER = DISABLE;
    }
    else if (messageTemp == "off")
    {
       F_COOLER = DISABLE;
    }
  }
  else if ((String(topic) == "humidifier")){
    Serial.print("Humidifier Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_HUMI = ENABLE;
        F_DEHUMI = DISABLE;
    }
    else if (messageTemp == "off")
    {
       F_HUMI = DISABLE;
    }
  }
  else if ((String(topic) == "dehumidifier")){
    Serial.print("Dehumidifier Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_DEHUMI = ENABLE;
        F_HUMI = DISABLE;
    }
    else if (messageTemp == "off")
    {
       F_DEHUMI = DISABLE;
    }
  }
  else if ((String(topic) == "pump1")){
    Serial.print("Pump 1 Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_PUMP1 = ENABLE;
    }
    else if (messageTemp == "off")
    {
       F_PUMP1 = DISABLE;
    }
  }
  else if ((String(topic) == "pump2")){
    Serial.print("Pump 2 Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_PUMP2 = ENABLE;
    }
    else if (messageTemp == "off")
    {
       F_PUMP2 = DISABLE;
    }
  }
  else if ((String(topic) == "pump3")){
    Serial.print("Pump 3 Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_PUMP3 = ENABLE;
    }
    else if (messageTemp == "off")
    {
       F_PUMP3 = DISABLE;
    }
  }
  else if ((String(topic) == "pump4")){
    Serial.print("Pump 4 Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_PUMP4 = ENABLE;
    }
    else if (messageTemp == "off")
    {
       F_PUMP4 = DISABLE;
    }
  }
  else if ((String(topic) == "pump5")){
    Serial.print("Pump 5 Status : ");
    Serial.println(messageTemp);
    if (messageTemp == "on"){
        F_PUMP5 = ENABLE;
    }
    else if (messageTemp == "off")
    {
       F_PUMP5 = DISABLE;
    }
  }
  
  // Feel free to add more if statements to control more GPIOs with MQTT
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client_Aktuator", mqttUser, mqttPassword )) {
      Serial.println("connected");
      // Subscribe topics
      client.subscribe("light");
      client.subscribe("heater");
      client.subscribe("cooler");
      client.subscribe("humidifier");
      client.subscribe("dehumidifier");
      client.subscribe("pump1");
      client.subscribe("pump2");
      client.subscribe("pump3");
      client.subscribe("pump4");
      client.subscribe("pump5");
      client.subscribe("temperature_mode");
      client.subscribe("humidity_mode");
      client.subscribe("moisture_mode");
      client.subscribe("light_mode");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void get_SQL_data(void *pvParam){
  (void) pvParam;
  while(1){
     //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(server_from_OnSQL); //nanti dr onSQL harusnya
    
      //Getting the response code
      int httpCode = http.GET();
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);

      //Getting data from SQL Database
      String DataFromSQL = http.getString();
      Serial.println("Data from SQL server: " + DataFromSQL);
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
        }else if (charDataFromSQL[i] == 'i'){ //partition for ActLight Data is i
          end_ActLight = i;
        }else if (charDataFromSQL[i] == 'j'){ //partition for OptTemp Data is j
          end_OptTemp = i;
        }else if (charDataFromSQL[i] == 'k'){ //partition for OptHum Data is k
          end_OptHum = i;
        }else if (charDataFromSQL[i] == 'h'){ //partition for OptMoist Data is h
          end_OptMoist = i;
        }else if (charDataFromSQL[i] == ':'){ //partition for OptLightStart Hour Data is :
          end_OptLightStart1 = i;
        }else if (charDataFromSQL[i] == 'm'){ //partition for OptLightStart Minute Data is m
          end_OptLightStart2 = i;
        }else if (charDataFromSQL[i] == ';'){ //partition for OptLightEnd Hour Data is :
          end_OptLightEnd1 = i;
        }else if (charDataFromSQL[i] == 'n'){ //partition for OptLightEnd Minute Data is n
          end_OptLightEnd2 = i;
        }  
     }
    
     for (i=0; i<=5; i++){
       ActTemp[i] = 0;
       ActHum[i] = 0;
       ActMoist1[i] = 0;
       ActMoist2[i] = 0;
       ActMoist3[i] = 0;
       ActMoist4[i] = 0;
       ActMoist5[i] = 0;
       OptTemp[i] = 0;
       OptHum[i] = 0;
       OptMoist[i] = 0;
       OptLightStart1[i] = 0;
       OptLightStart2[i] = 0;
       OptLightEnd1[i] = 0;
       OptLightEnd2[i] = 0;
     }

      
     //Convert array of char(charDataFromSQL) to float for each parameters
     for (i=0; i<end_ActTemp; i++){
       ActTemp[j] = charDataFromSQL[i];
       j++;
     }
     fActTemp = atof(ActTemp);
     j=0;
     sActTemp = String(fActTemp);

     for (i=end_ActTemp+1; i<end_ActHum; i++){
       ActHum[j] = charDataFromSQL[i];
       j++;
     }
     fActHum = atof(ActHum);
     j=0;
     sActHum = String(fActHum);
 
     for (i=end_ActHum+1; i<end_ActMoist1; i++){
       ActMoist1[j] = charDataFromSQL[i];
       j++;
     }
     fActMoist1 = atof(ActMoist1);
     j=0;
     sActMoist1 = String(fActMoist1);

     for (i=end_ActMoist1+1; i<end_ActMoist2; i++){
       ActMoist2[j] = charDataFromSQL[i];
       j++;
     }
     fActMoist2 = atof(ActMoist2);
     j=0;
     sActMoist2 = String(fActMoist2);
 
     for (i=end_ActMoist2+1; i<end_ActMoist3; i++){
       ActMoist3[j] = charDataFromSQL[i];
       j++;
     }
     fActMoist3 = atof(ActMoist3);
     j=0;
     sActMoist3 = String(fActMoist3);

     for (i=end_ActMoist3+1; i<end_ActMoist4; i++){
       ActMoist4[j] = charDataFromSQL[i];
       j++;
     }
     fActMoist4 = atof(ActMoist4);
     j=0;
     sActMoist4 = String(fActMoist4);

     for (i=end_ActMoist4+1; i<end_ActMoist5; i++){
       ActMoist5[j] = charDataFromSQL[i];
       j++;
     }
     fActMoist5 = atof(ActMoist5);
     j=0;
     sActMoist5 = String(fActMoist5);

     for (i=end_ActMoist5+1; i<end_ActLight; i++){
       ActLight[j] = charDataFromSQL[i];
       j++;
     }
     fActLight = atof(ActLight);
     j=0;
     sActLight = String(fActLight);

     for (i=end_ActLight+1; i<end_OptTemp; i++){
       OptTemp[j] = charDataFromSQL[i];
       j++;
     }
     fOptTemp = atof(OptTemp);
     j=0;
     sOptTemp = String(fOptTemp);

     for (i=end_OptTemp+1; i<end_OptHum; i++){
       OptHum[j] = charDataFromSQL[i];
       j++;
     }
     fOptHum = atof(OptHum);
     j=0;
     sOptHum = String(fOptHum);

     for (i=end_OptHum+1; i<end_OptMoist; i++){
      OptMoist[j] = charDataFromSQL[i];
      j++;
     }
     fOptMoist = atof(OptMoist);
     j=0;
     sOptMoist = String(fOptMoist);

     for (i=end_OptMoist+1; i<end_OptLightStart1; i++){
       OptLightStart1[j] = charDataFromSQL[i];
       j++;
     }
     fOptLightStart1 = atof(OptLightStart1);
     iOptLightStart1 = (int)fOptLightStart1;
     j=0;
     sOptLightStart1 = String(iOptLightStart1);

     for (i=end_OptLightStart1+1; i<end_OptLightStart2; i++){
       OptLightStart2[j] = charDataFromSQL[i];
       j++;
     }
     fOptLightStart2 = atof(OptLightStart2);
     iOptLightStart2 = (int)fOptLightStart2;
     j=0;
     sOptLightStart2 = String(iOptLightStart2);
 
     if (iOptLightStart2 == 0){
       sOptLightStart = sOptLightStart1 + ":" + sOptLightStart2 + "0"; 
     }else if (iOptLightStart2 < 10){
       sOptLightStart = sOptLightStart1 + ":0" + sOptLightStart2; 
     }else{
       sOptLightStart = sOptLightStart1 + ":" + sOptLightStart2; 
     }

     for (i=end_OptLightStart2+1; i<end_OptLightEnd1; i++){
       OptLightEnd1[j] = charDataFromSQL[i];
       j++;
     }
     fOptLightEnd1 = atof(OptLightEnd1);
     iOptLightEnd1 = (int)fOptLightEnd1;
     j=0;
     sOptLightEnd1 = String(iOptLightEnd1);

     for (i=end_OptLightEnd1+1; i<end_OptLightEnd2; i++){
       OptLightEnd2[j] = charDataFromSQL[i];
       j++;
     }
     fOptLightEnd2 = atof(OptLightEnd2);
     iOptLightEnd2 = (int)fOptLightEnd2;
     j=0;
     sOptLightEnd2 = String(iOptLightEnd2);

     if (iOptLightEnd2 == 0){
       sOptLightEnd = sOptLightEnd1 + ":" + sOptLightEnd2 + "0"; 
     }else if (iOptLightEnd2 < 10){
       sOptLightEnd = sOptLightEnd1 + ":0" + sOptLightEnd2; 
     }else{
      sOptLightEnd = sOptLightEnd1 + ":" + sOptLightEnd2; 
     }
      
     // Free resources
     http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    
    vTaskDelay (5000); //Penerimaan Data setiap 5 DETIK SEKALI
  }
}

//At certain delay (every 30 seconds), Actual Datas are sent to Database
void send_SQL_data(void *pvParam){
  (void) pvParam;
  while(1){
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
//      http.begin(server_to_OffSQL_ManData);
//      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
//    
//      // Prepare your HTTP POST request data
//      String httpRequestData = "api_key= " + apiKeyValue
//                          + " &heater= " + heater_status_resp
//                          + " &cooler= " + cooler_status_resp
//                          + " &humidifier= " + humi_status_resp
//                          + " &dehumidifier= " + dehumi_status_resp
//                          + " &led= " + led_status_resp
//                          + " &micropump1= " + pump1_status_resp
//                          + " &micropump2= " + pump2_status_resp
//                          + " &micropump3= " + pump3_status_resp
//                          + " &micropump4= " + pump4_status_resp
//                          + " &micropump5= " + pump5_status_resp + "";
//      Serial.println("Manual Data to local : ");
//      Serial.println(httpRequestData);
//
//      // Send HTTP POST request
//      int httpResponseCode = http.POST(httpRequestData);
//        
//      if (httpResponseCode>0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode);
//        Serial.println("-----------------");   
//      }
//      else {
//        Serial.print("Error code: ");
//        Serial.println(httpResponseCode);
//        Serial.println("-----------------");   
//      }
//      // Free resources
//      http.end();

//      http.begin(server_to_OnSQL_ManData);
//      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
//    
//      // Prepare your HTTP POST request data
//      String httpRequestData_2 = "api_key= " + apiKeyValue
//                          + " &heater= " + heater_status_resp
//                          + " &cooler= " + cooler_status_resp
//                          + " &humidifier= " + humi_status_resp
//                          + " &dehumidifier= " + dehumi_status_resp
//                          + " &led= " + led_status_resp
//                          + " &micropump1= " + pump1_status_resp
//                          + " &micropump2= " + pump2_status_resp
//                          + " &micropump3= " + pump3_status_resp
//                          + " &micropump4= " + pump4_status_resp
//                          + " &micropump5= " + pump5_status_resp + "";
//      Serial.println("Manual Data to server : ");
//      Serial.println(httpRequestData_2);
//
//      // Send HTTP POST request
//      int httpResponseCode_2 = http.POST(httpRequestData_2);
//        
//      if (httpResponseCode_2>0) {
//        Serial.print("HTTP Response code: ");
//        Serial.println(httpResponseCode_2);
//        Serial.println("-----------------");   
//      }
//      else {
//        Serial.print("Error code: ");
//        Serial.println(httpResponseCode_2);
//        Serial.println("-----------------");   
//      }
//      // Free resources
//      http.end();

      http.begin(server_to_OffSQL_OptData);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
      // Prepare your HTTP POST request data
      String httpRequestData_3 = "api_key= " + apiKeyValue
                          + " &temp= " + sOptTemp
                          + " &hum= " + sOptHum
                          + " &moist= " + sOptMoist
                          + " &light_start= " + sOptLightStart
                          + " &light_end= " + sOptLightEnd + "";
      Serial.println("Optimum Data to local : ");
      Serial.println(httpRequestData_3);

      // Send HTTP POST request
      int httpResponseCode_3 = http.POST(httpRequestData_3);
        
      if (httpResponseCode_3>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode_3);
        Serial.println("-----------------");   
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode_3);
        Serial.println("-----------------");   
      }
      // Free resources
      http.end();
      
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    vTaskDelay (120000);
  }
}


void print_status(void *pvParam){
  (void) pvParam;
  while(1){
    // for checking actuator status
    Serial.println("Control Temperature = " + temperature_mode);
    Serial.println("Control Humidity = " + humidity_mode);
    Serial.println("Control Moisture = " + moisture_mode);
    Serial.println("Control Light = " + light_mode);
    Serial.println("-----------------");
    Serial.println("ActTemp = " + sActTemp);
    Serial.println("ActHum = " + sActHum);
    Serial.println("ActMoist1 = " + sActMoist1);
    Serial.println("ActMoist2 = " + sActMoist2);
    Serial.println("ActMoist3 = " + sActMoist3);
    Serial.println("ActMoist4 = " + sActMoist4);
    Serial.println("ActMoist5 = " + sActMoist5);
    Serial.println("-----------------");
    Serial.println("OptTemp = " + sOptTemp);
    Serial.println("OptHum = " + sOptHum);
    Serial.println("OptMoist = " + sOptMoist);
    Serial.println("-----------------");
    
    Serial.println("Lights ON at = " + sOptLightStart);
    Serial.println("Lights OFF at = " + sOptLightEnd);
    Serial.println("LED Status = " + led_status_resp);
    Serial.println("Heater Status = " + heater_status_resp);
    Serial.println("Cooler Status = " + cooler_status_resp);
    Serial.println("Humidifier Status = " + humi_status_resp);
    Serial.println("Dehumidifier Status = " + dehumi_status_resp);
    Serial.println("Pump 1 Status = " + pump1_status_resp);
    Serial.println("Pump 2 Status = " + pump2_status_resp);
    Serial.println("Pump 3 Status = " + pump3_status_resp);
    Serial.println("Pump 4 Status = " + pump4_status_resp);
    Serial.println("Pump 5 Status = " + pump5_status_resp);
    
    Serial.print("heater Dutycycle = ");
    Serial.println(heater_dutyCycle);
    Serial.print("cooler Dutycycle = ");
    Serial.println(cooler_dutyCycle);
    Serial.print("Humidifier Dutycycle = ");
    Serial.println(humidifier_dutyCycle);
    Serial.print("Dehumidifier Dutycycle = ");
    Serial.println(dehumidifier_dutyCycle);
    Serial.print("Pump 1 Dutycycle = ");
    Serial.println(pump1_dutyCycle);
    Serial.print("Pump 2 Dutycycle = ");
    Serial.println(pump2_dutyCycle);
    Serial.print("Pump 3 Dutycycle = ");
    Serial.println(pump3_dutyCycle);
    Serial.print("Pump 4 Dutycycle = ");
    Serial.println(pump4_dutyCycle);
    Serial.print("Pump 5 Dutycycle = ");
    Serial.println(pump5_dutyCycle);
    Serial.println("-----------------");
    
    vTaskDelay(5000);
  }
}


/*Light Control Procedure Call*/
void lightTask(void *pvParam){
  (void) pvParam;
  while(1){
    if (light_mode == "man"){
      if (F_LED){
        digitalWrite(LED, HIGH);
        led_status_resp = "ON";
      }else{
        digitalWrite(LED, LOW);
        led_status_resp = "OFF";
      }
    }
    else if (light_mode == "auto"){
      light_control();
    }
    
    vTaskDelay(1000);
  }
}


/*Temperature Control Procedure Call*/
void temperatureTask(void *pvParam){
  (void) pvParam;
  while(1){   
    if (temperature_mode == "man"){
       if (F_HEATER){
          dimmer_dutyCycle = 50;
          dimmer.setPower(dimmer_dutyCycle);
          ledcWrite(CHANNEL_KIPAS_HEATER, max_dutyCycle);
          ledcWrite(CHANNEL_KIPAS_COOLER, min_dutyCycle);
          digitalWrite(KIPAS_RAD_COOLER, LOW);
          digitalWrite(PLT_COOLER, LOW);
          digitalWrite(PUMP_COOLER, LOW);
          heater_status_resp = "ON";
          cooler_status_resp = "OFF";
       }else if (F_COOLER){
          dimmer_dutyCycle = 0;
          dimmer.setPower(dimmer_dutyCycle);
          ledcWrite(CHANNEL_KIPAS_HEATER, min_dutyCycle);
          ledcWrite(CHANNEL_KIPAS_COOLER, max_dutyCycle);
          digitalWrite(KIPAS_RAD_COOLER, HIGH);
          digitalWrite(PLT_COOLER, HIGH);
          digitalWrite(PUMP_COOLER, HIGH);
          heater_status_resp = "OFF";
          cooler_status_resp = "ON";
       }
    }
    else if (temperature_mode == "auto"){
      temperature_control();
    }
    
    vTaskDelay(3500);
  }
}

/*Humidity Control Procedure Call*/
void humidityTask(void *pvParam){
  (void) pvParam;
  while(1){
    if (humidity_mode == "man"){
      if (F_HUMI){
        ledcWrite(CHANNEL_KIPAS_HUMIDIFIER, max_dutyCycle);
        ledcWrite(CHANNEL_KIPAS_DEHUMIDIFIER, min_dutyCycle);
        digitalWrite(MIST_HUMIDIFIER, HIGH);
        digitalWrite(PLT_DEHUMIDIFIER, LOW);
        digitalWrite(KIPAS_PLT_DEHUMIDIFIER, LOW);
        humi_status_resp = "ON";
        dehumi_status_resp = "OFF";
      }else if (F_DEHUMI){
        ledcWrite(CHANNEL_KIPAS_HUMIDIFIER, min_dutyCycle);
        ledcWrite(CHANNEL_KIPAS_DEHUMIDIFIER, max_dutyCycle);
        digitalWrite(MIST_HUMIDIFIER, LOW);
        digitalWrite(PLT_DEHUMIDIFIER, HIGH);  
        digitalWrite(KIPAS_PLT_DEHUMIDIFIER, HIGH);
        humi_status_resp = "OFF";
        dehumi_status_resp = "ON";    
      }
    }else if (humidity_mode == "auto"){
      humidity_control();
    }
    
    vTaskDelay(3500);
  }
}

/*HMoisture Control Procedure Call*/
void moistureTask(void *pvParam){
  (void) pvParam;
  while(1){
    if (moisture_mode == "man"){
      if (F_PUMP1){
        ledcWrite(CHANNEL_MICROPUMP1, max_dutyCycle);
        pump1_status_resp = "ON";
      }else {
        ledcWrite(CHANNEL_MICROPUMP1, min_dutyCycle);
        pump1_status_resp = "OFF";
      }
      if (F_PUMP2){
        ledcWrite(CHANNEL_MICROPUMP2, max_dutyCycle);
        pump2_status_resp = "ON";
      }else {
        ledcWrite(CHANNEL_MICROPUMP2, min_dutyCycle);
        pump2_status_resp = "OFF";
      }
      if (F_PUMP3){
        ledcWrite(CHANNEL_MICROPUMP3, max_dutyCycle);
        pump3_status_resp = "ON";
      }else {
        ledcWrite(CHANNEL_MICROPUMP3, min_dutyCycle);
        pump3_status_resp = "OFF";
      }
      if (F_PUMP4){
        ledcWrite(CHANNEL_MICROPUMP4, max_dutyCycle);
        pump4_status_resp = "ON";
      }else {
        ledcWrite(CHANNEL_MICROPUMP4, min_dutyCycle);
        pump4_status_resp = "OFF";
      }
      if (F_PUMP5){
        ledcWrite(CHANNEL_MICROPUMP5, max_dutyCycle);
        pump5_status_resp = "ON";
      }else{
        ledcWrite(CHANNEL_MICROPUMP5, min_dutyCycle);
        pump5_status_resp = "OFF";
      }
    }else if (moisture_mode == "auto"){
      moisture_control();
    }

    vTaskDelay(2500);
  }
}

void temperature_control(){
    /*Heater Controller*/
    error_heater = fOptTemp - fActTemp;
    error_sum_heater = error_sum_heater + error_heater;
    error_sum_heater = constrain(error_sum_heater, -1000, 1000); //Range error_sum: -1000 < error_sum < 1000
    error_rate_heater = error_heater - error_last_heater;
    
    //Tolerance
    if (fabs(error_heater)<=tolerance_off){
      heater_dutyCycle = 0;
      dimmer_dutyCycle = 0;
    }else
    if (fabs(error_heater)> tolerance_on) {
      dimmer_dutyCycle = 50;
      heater_dutyCycle = heater_dutyCycle_initial + floor(Kp*error_heater + Ki*error_sum_heater + Kd *error_rate_heater); //heater_dutyCycle_initial utk start awal dutycycle
    }
    
    error_last_heater = error_heater;
    heater_dutyCycle = constrain(heater_dutyCycle, min_dutyCycle, max_dutyCycle);
    
    
    /*Cooler Controller*/
    error_cooler = fActTemp - fOptTemp;
    error_sum_cooler = error_sum_cooler + error_cooler;
    error_sum_cooler = constrain(error_sum_cooler, -1000, 1000);
    error_rate_cooler = error_cooler - error_last_cooler;
    
    //Tolerance
    if (fabs(error_cooler)<=tolerance_off){
      cooler_dutyCycle = 0;
    }else if(fabs(error_cooler>tolerance_on)){
      cooler_dutyCycle = cooler_dutyCycle_initial + floor(Kp*error_cooler + Ki*error_sum_cooler + Kd *error_rate_cooler);
    }
    
    error_last_cooler = error_cooler;
    cooler_dutyCycle = constrain(cooler_dutyCycle, min_dutyCycle, max_dutyCycle);

    
	  //Define Dutycycle for Heater & Cooler
    dimmer.setPower(dimmer_dutyCycle);
    ledcWrite(CHANNEL_KIPAS_HEATER, heater_dutyCycle);
    ledcWrite(CHANNEL_KIPAS_COOLER, cooler_dutyCycle);
    
    if (error_heater > tolerance_on){
      heater_status_resp = "ON";
    }else if (error_heater <= tolerance_off){
      heater_status_resp = "OFF";
    }
    if (error_cooler > tolerance_on){
      digitalWrite(PLT_COOLER, HIGH);
      digitalWrite(KIPAS_RAD_COOLER, HIGH);
      digitalWrite(PUMP_COOLER, HIGH);
      cooler_status_resp = "ON";
    }else if (error_cooler <= tolerance_off){
      digitalWrite(PLT_COOLER, LOW);
      digitalWrite(KIPAS_RAD_COOLER, LOW);
      digitalWrite(PUMP_COOLER, LOW);
      cooler_status_resp = "OFF";
    }
    
}

void humidity_control(){
    /*Humidifier Controller*/
    error_humidifier = fOptHum - fActHum;
    error_sum_humidifier = error_sum_humidifier + error_humidifier;
    error_sum_humidifier = constrain(error_sum_humidifier, -1000, 1000);
    error_rate_humidifier = error_humidifier - error_last_humidifier;

    //Tolerance
    if (fabs(error_humidifier)<=tolerance_off){
      humidifier_dutyCycle = 0;
    }else if (fabs(error_humidifier)> tolerance_on){
      humidifier_dutyCycle = floor(Kp*error_humidifier + Ki*error_sum_humidifier + Kd*error_rate_humidifier);
    }

    error_last_humidifier = error_humidifier;
    humidifier_dutyCycle = constrain(humidifier_dutyCycle, min_dutyCycle, max_dutyCycle);
    

    /*Dehumidifier Controller*/
    error_dehumidifier = fActHum - fOptHum;
    error_sum_dehumidifier = error_sum_dehumidifier + error_dehumidifier;
    error_sum_dehumidifier = constrain(error_sum_dehumidifier , -1000, 1000);
    error_rate_dehumidifier = error_dehumidifier - error_last_dehumidifier;

    if (fabs(error_dehumidifier)<=tolerance_off){
      dehumidifier_dutyCycle = 0;
    }else if (fabs(error_dehumidifier)> tolerance_on){
      dehumidifier_dutyCycle = floor(Kp*error_dehumidifier + Ki*error_sum_dehumidifier + Kd*error_rate_dehumidifier);
    }
    
    error_last_dehumidifier = error_dehumidifier;
    dehumidifier_dutyCycle = constrain(dehumidifier_dutyCycle, min_dutyCycle, max_dutyCycle);

    
	  //Define Dutycycle for Humidifier & Dehumidifier
    ledcWrite(CHANNEL_KIPAS_HUMIDIFIER, humidifier_dutyCycle);
    ledcWrite(CHANNEL_KIPAS_DEHUMIDIFIER, dehumidifier_dutyCycle);
    
    if (error_humidifier > tolerance_on){
      digitalWrite(MIST_HUMIDIFIER, HIGH);
      humi_status_resp = "ON";
    }else if (error_humidifier <= tolerance_off){
      digitalWrite(MIST_HUMIDIFIER, LOW);
      humi_status_resp = "OFF";
    }
    if (error_dehumidifier > tolerance_on){
      digitalWrite(PLT_DEHUMIDIFIER, HIGH);
      digitalWrite(KIPAS_PLT_DEHUMIDIFIER, HIGH);
      dehumi_status_resp = "ON";
    }else if (error_dehumidifier <= tolerance_off){
      digitalWrite(PLT_DEHUMIDIFIER, LOW);
      digitalWrite(KIPAS_PLT_DEHUMIDIFIER, LOW);
      dehumi_status_resp = "OFF";
    }
}

void moisture_control(){ 
	/*Pump 1 Controller*/
	error_pump1 = fOptMoist - fActMoist1;
  error_sum_pump1 = error_sum_pump1 + error_pump1;
  error_sum_pump1 = constrain(error_sum_pump1, -1000, 1000);
  error_rate_pump1 = error_pump1 - error_last_pump1;

  if (fabs(error_pump1 <= tolerance_off)){
      pump1_dutyCycle = 0;
  }else if (fabs(error_pump1 > tolerance_on)){
    pump1_dutyCycle = pump1_dutyCycle_initial + floor(Kp*error_pump1 + Ki*error_sum_pump1 + Kd*error_rate_pump1);
  }
  
  error_last_pump1 = error_pump1;
  pump1_dutyCycle = constrain(pump1_dutyCycle, min_dutyCycle, max_dutyCycle);
  

  /*Pump 2 Controller*/
  error_pump2 = fOptMoist - fActMoist2;
  error_sum_pump2 = error_sum_pump2 + error_pump2;
  error_sum_pump2 = constrain(error_sum_pump2, -1000, 1000);
  error_rate_pump2 = error_pump2 - error_last_pump2;
  
  if (fabs(error_pump2 <= tolerance_off)){
      pump2_dutyCycle = 0;
  }else if (fabs(error_pump2 > tolerance_on)){
    pump2_dutyCycle = pump2_dutyCycle_initial + floor(Kp*error_pump2 + Ki*error_sum_pump2 + Kd*error_rate_pump2);
  }
  
  error_last_pump2 = error_pump2;
  pump2_dutyCycle = constrain(pump2_dutyCycle, min_dutyCycle, max_dutyCycle);

  
  /*Pump 3 Controller*/
  error_pump3 = fOptMoist - fActMoist3;
  error_sum_pump3 = error_sum_pump3 + error_pump3;
  error_sum_pump3 = constrain(error_sum_pump3, -1000, 1000);
  error_rate_pump3 = error_pump3 - error_last_pump3;
  
  if (fabs(error_pump3 <= tolerance_off)){
      pump3_dutyCycle = 0;
  }else if (fabs(error_pump3 > tolerance_on)){
    pump3_dutyCycle = pump3_dutyCycle_initial + floor(Kp*error_pump3 + Ki*error_sum_pump3 + Kd*error_rate_pump3);
  }
  
  error_last_pump3 = error_pump3;
  pump3_dutyCycle = constrain(pump3_dutyCycle, min_dutyCycle, max_dutyCycle);


  /*Pump 4 Controller*/
  error_pump4 = fOptMoist - fActMoist4;
  error_sum_pump4 = error_sum_pump4 + error_pump4;
  error_sum_pump4 = constrain(error_sum_pump4, -1000, 1000);
  error_rate_pump4 = error_pump4 - error_last_pump4;
   
  if (fabs(error_pump4 <= tolerance_off)){
      pump4_dutyCycle = 0;
  }else if (fabs(error_pump4 > tolerance_on)){
    pump4_dutyCycle = pump4_dutyCycle_initial + floor(Kp*error_pump4 + Ki*error_sum_pump4 + Kd*error_rate_pump4);    
  }
  
  error_last_pump4 = error_pump4;
  pump4_dutyCycle = constrain(pump4_dutyCycle, min_dutyCycle, max_dutyCycle);


  /*Pump 5 Controller*/
  error_pump5 = fOptMoist - fActMoist5;
  error_sum_pump5 = error_sum_pump5 + error_pump5;
  error_sum_pump5 = constrain(error_sum_pump5, -1000, 1000);
  error_rate_pump5 = error_pump5 - error_last_pump5;
  
  if (fabs(error_pump5 <= tolerance_off)){
      pump5_dutyCycle = 0;
  }else if (fabs(error_pump5 > tolerance_on)){
    pump5_dutyCycle = pump5_dutyCycle_initial + floor(Kp*error_pump5 + Ki*error_sum_pump5 + Kd*error_rate_pump5);    
  }
  
  error_last_pump5 = error_pump5;
  pump5_dutyCycle = constrain(pump5_dutyCycle, min_dutyCycle, max_dutyCycle);
  


  /*PWM Control*/
  ledcWrite(CHANNEL_MICROPUMP1, pump1_dutyCycle);
  ledcWrite(CHANNEL_MICROPUMP2, pump2_dutyCycle);
  ledcWrite(CHANNEL_MICROPUMP3, pump3_dutyCycle);
  ledcWrite(CHANNEL_MICROPUMP4, pump4_dutyCycle);
  ledcWrite(CHANNEL_MICROPUMP5, pump5_dutyCycle);

  if (error_pump1 > tolerance_on){
    pump1_status_resp = "ON";
  }else if (error_pump1 <= tolerance_off){
    pump1_status_resp = "OFF";
  }
  if (error_pump2 > tolerance_on){
    pump2_status_resp = "ON";
  }else if (error_pump2 <= tolerance_off){
    pump2_status_resp = "OFF";
  }
  if (error_pump3 > tolerance_on){
    pump3_status_resp = "ON";
  }else if (error_pump3 <= tolerance_off){
    pump3_status_resp = "OFF";
  }
  if (error_pump4 > tolerance_on){
    pump4_status_resp = "ON";
  }else if (error_pump4 <= tolerance_off){
    pump4_status_resp = "OFF";
  }
  if (error_pump5 > tolerance_on){
    pump5_status_resp = "ON";
  }else if (error_pump5 <= tolerance_off){
    pump5_status_resp = "OFF";
  }
}

void light_control(){
	/*LED Controller*/
	
	/*Activating RTC Module*/
	DateTime now = rtc.now();
	Serial.print("Current Time = ");
	Serial.print(now.hour());
	Serial.print(":");  
	Serial.print(now.minute());
	Serial.print(":");
	Serial.println(now.second());
  
  if ((now.hour() == iOptLightStart1) && (now.minute() == iOptLightStart2)){
    lightActive = true;
    led_status_resp = "ON";
    digitalWrite(LED, HIGH);
  } else
  if ((now.hour() == iOptLightEnd1)&& (now.minute() == iOptLightEnd2)){
    lightActive = false;
    led_status_resp = "OFF";
    digitalWrite(LED, LOW);
  }
  Serial.println("-----------------");
}
