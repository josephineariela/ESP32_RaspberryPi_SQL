//Global Library
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

//Hotspot Identity
const char* ssid     = "pipino";
const char* password = "cilukbaa";

//Server & API Key identification
const char* serverName = "http://192.168.43.159/SQL_toESP-A.php";
String apiKeyValue = "192001003";

//Global Variable Initialization
int i, end_ActTemp, end_ActHum, end_ActMoist, end_ActLight;
int j =0;
float fActTemp, fActHum, fActMoist, fActLight;
char ActTemp[5], ActHum[5], ActMoist[5], ActLight[10];

void setup() {
  Serial.begin(115200);
  
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

void loop() {
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
    Serial.println();
    Serial.print("ActTemp = ");
    Serial.print(fActTemp);
    j=0;

    for (i=end_ActTemp+1; i<end_ActHum; i++){
      ActHum[j] = charDataActual[i];
      j++;
    }
    fActHum = atof(ActHum);
    Serial.println();
    Serial.print("ActHum = ");
    Serial.print(fActHum);
    j=0;

    for (i=end_ActHum+1; i<end_ActMoist; i++){
      ActMoist[j] = charDataActual[i];
      j++;
    }
    fActMoist = atof(ActMoist);
    Serial.println();
    Serial.print("ActMoist = ");
    Serial.print(fActMoist);
    j=0;

    for (i=end_ActMoist+1; i<end_ActLight; i++){
      ActLight[j] = charDataActual[i];
      j++;
    }
    fActLight = atof(ActLight);
    Serial.println();
    Serial.print("ActLight = ");
    Serial.print(fActLight);
    j=0;

    Serial.println();

    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }
  delay (5000);
}
