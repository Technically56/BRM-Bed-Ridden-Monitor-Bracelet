//libraries
#include <MedianFilterLib.h>
#include <BMI160.h>
#include <BMI160Gen.h>
#include <CurieIMU.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "MAX30100_PulseOximeter.h"
#include <Wire.h>
#include <MAX30100.h>


#define temppin 26
#define gsrpin 27
#define REPORTING_PERIOD_MS 3000
//define the variables and pointers
char const *ssid = "Dincer_Ailesi_Ev_Agi";
char const *password = "pusat2004@";
char const *firebase_url = "https://health-tracking-7476d-default-rtdb.firebaseio.com/";
char const *firebase_token = "YDvpfxmdhogaDMfZWzS9VzWDMlLNQER7WorAd9Zg";
uint32_t tsLastReport = 0;
uint32_t tsaLastReport = 0;
float hr = 0;
int oxy = 0;
int medianOxy;
int medianHR;
int gxRaw, gyRaw, gzRaw;
int sumGSR;
float gx, gy, gz;
//define the objects
WiFiClientSecure wifiClient;
HTTPClient http;
PulseOximeter pox;
MAX30100 oxsensor;
MedianFilter<float> medianFilterGZ(5);
MedianFilter<float> medianFilterGY(5);
MedianFilter<float> medianFilterGX(5);
MedianFilter<float> medianFilterTEMP(10);
MedianFilter<float> medianFilterHR(10);
MedianFilter<int> medianFilterOXY(10);
void initialize() {
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  delay(1000);
  http.setInsecure();
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("connecting");
    }
  Serial.println("Connected to Wifi Network");
} 

String get_from_fb(String path){
  String url = firebase_url;
  String payload;
  url = url + path + ".json?auth=" + firebase_token;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(url);
      int httpResponseCode = http.GET();
      delay(500);
      Serial.println(httpResponseCode);
      if(httpResponseCode >0){
        Serial.println(httpResponseCode);
        payload = http.getString();  
        Serial.println(payload);
      }    
    }
  return payload;
}

int send_to_fb(String path,String data){
  int httpResponseCode;
  String payload;
  String url = firebase_url;
  url = url + path + ".json?auth=" + firebase_token;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(url);
      httpResponseCode = http.PUT(data);
      delay(500);
      Serial.println(httpResponseCode);
      if(httpResponseCode >0){
        Serial.println(httpResponseCode);
        payload = http.getString();  
        Serial.println(payload);
      }    
    }
  return httpResponseCode;
  }
int delete_fb(String path){
 int httpResponseCode;
  String url = firebase_url;
  url = url + path + ".json?auth=" + firebase_token;
  if(WiFi.status() == WL_CONNECTED){
      http.begin(url);
      httpResponseCode = http.DELETE();
      delay(500);
      Serial.println(httpResponseCode);
      if(httpResponseCode >0){
        Serial.println(httpResponseCode);
        String payload = http.getString();  
        Serial.println(payload);
      }    
    }
  return httpResponseCode;
  }
void readOximeter(){
   if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        hr = pox.getHeartRate();
        oxy = pox.getSpO2();
        medianHR = medianFilterHR.AddValue(hr);
        medianOxy = medianFilterOXY.AddValue(oxy);
        Serial.println(medianOxy);
        Serial.println(medianHR);
        tsLastReport = millis();
        if(medianHR > 20){
          send_to_fb("Sensors/Oxy/HR",String(medianHR));
          delay(50);
        }
        if(medianOxy > 50){
          
          send_to_fb("Sensors/Oxy/SpO2",String(medianOxy));
          delay(50);
        }  
    }
}
void readGyro(){
  BMI160.readGyro(gxRaw, gyRaw, gzRaw);
  gx = convertGyro(gxRaw);
  gy = convertGyro(gyRaw);
  gz = convertGyro(gzRaw);
  
  send_to_fb("Sensors/Gyro/gx",String(gx));
  delay(50);
  send_to_fb("Sensors/Gyro/gy",String(gy));
  delay(50);
  send_to_fb("Sensors/Gyro/gz",String(gz));
  delay(50);
}
float convertGyro(int gRaw) {
  float g = (gRaw * 250.0) / 32768.0;
  return g;
}
void readTemps(){
  int tempraw = analogRead(temppin);
  float temp = map(tempraw, 0, 4096, 0.0, 110.0);
  send_to_fb("Sensors/Temp/",String(temp));
}
void readGSR(){
  for(int i=0;i<10;i++){
    int sensorValue = analogRead(gsrpin);
    sumGSR += sensorValue;
  }
  sumGSR = sumGSR/10;
  send_to_fb("Sensors/GSR/",String(sumGSR));
}
void setup() {
  initialize();
  pinMode(temppin,INPUT);
  pinMode(gsrpin,INPUT);
  analogReadResolution(12);
  BMI160.begin(BMI160GenClass::I2C_MODE);
  delay(500);
  BMI160.setGyroRange(250);
  delay(500);

}
void loop(){
  readGyro();
  delay(3000);
  readOximeter();
  delay(2000);
  readTemps();
  delay(2000);
  readGSR();
  delay(2000);

}
void setup1(){
  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_27_1MA);
}
void loop1() {
  pox.update();
}