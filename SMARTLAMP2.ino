// 2497101


#include <ESP8266WiFi.h>
#include "secrets.h"
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>

#define led1 D4
#define led2 D2
#define led3 D1
// char ssid[] = SECRET_SSID;   // your network SSID (name) 
// char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiClient  client;
#ifndef STASSID
#define STASSID "KELUARGA02"
#define STAPSK "jomblo00"
#endif

const char ssid[] = STASSID;
const char pass[] = STAPSK;
// Weather station channel details
unsigned long weatherStationChannelNumber = SECRET_CH_ID_WEATHER_STATION;

//
//int statusCode = 0;
//int field[8] = {1,2,3,4,5,6,7,8};
int led_pin = D7;
#define N_DIMMERS 1
int dimmer_pin[] = { D0, D0, 15 };
bool wm_nonblocking =true;
bool stateAuto;
int  stateLed1;
int  stateLed2;
int  stateLed3;
#define VIN 3.3 // V power voltage, 3.3v in case of NodeMCU
#define R 10000 // Voltage devider resistor value
const int Analog_Pin = 0; // Analog pin A0
bool stateLedAuto1,stateLedAuto2,stateLedAuto3;
bool lastStateAuto;
int LDR_Val; // Analog value from the LDR
int Iluminance; //Lux value
bool stateRun = false;
bool sRun = false;
int lastConnectionAttempt = millis();
int connectionDelay = 5000; // try to reconnect every 5 seconds
WiFiManager wm;

void setup() {
  WiFi.mode(WIFI_STA);
  //wm.resetSettings();
  Serial.begin(115200); 
  pinMode(led_pin, OUTPUT);
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(led3,OUTPUT);
  digitalWrite(led_pin, LOW);
  if(wm_nonblocking) wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(30);
  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wm.autoConnect("smart","00000000"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect or hit timeout");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }

  /* switch off led */
      digitalWrite(led_pin, HIGH);


      //digitalWrite(led_pin, LOW);
      /* configure dimmers, and OTA server events */
      analogWriteRange(1000);
      analogWrite(led_pin, 50);
    
      for (int i = 0; i < N_DIMMERS; i++) {
        pinMode(dimmer_pin[i], OUTPUT);
        analogWrite(dimmer_pin[i], HIGH);
      }
    
      //ArduinoOTA.setHostname(host);
      ArduinoOTA.onStart([]() {  // switch off all the PWMs during upgrade
        for (int i = 0; i < N_DIMMERS; i++) {
          analogWrite(dimmer_pin[i], 0);
        }
        digitalWrite(led_pin, 255);
      });
    
      ArduinoOTA.onEnd([]() {  // do a fancy thing with our board led at end
        for (int i = 0; i < 30; i++) {
          analogWrite(led_pin, (i * 100) % 1001);
          delay(50);
        }
      });
    
      ArduinoOTA.onError([](ota_error_t error) {
        (void)error;
        ESP.restart();
      });
       //Serial.println("Ready");
       ThingSpeak.begin(client); 
       ArduinoOTA.begin();
       Serial.println("Ready");
}
unsigned long sv=0;
void loop() {
  if(wm_nonblocking) wm.process(); // avoid delays() in loop when non-blocking and other long running code  
  ArduinoOTA.handle();
  int statusCode = 0; 
  stateWIFI();
 // LDR_Val = analogRead(Analog_Pin);
//  Iluminance = conversion(LDR_Val);
  if(millis() - sv > 1000){
    sv = millis();
  int state = ThingSpeak.readIntField(weatherStationChannelNumber,1); // Field 1
  statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200)
    { 
      stateAuto = state;
      Serial.println("stateAuto: " + String(stateAuto)); 
    }
  else{
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }

  stateLed1 = ThingSpeak.readIntField(weatherStationChannelNumber,2); // Field 2
  statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200)
    {
      Serial.println(String()+"stateled1:"+stateLed1);
      if(stateLed1 == 1 && stateAuto == 0){ digitalWrite( led1,LOW);}
      else if (stateLed1 == 0 && stateAuto == 0){ digitalWrite( led1,HIGH); }
    }
  else{
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }

  stateLed2 = ThingSpeak.readIntField(weatherStationChannelNumber,3); // Field 3
  statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200)
    {
      Serial.println(String()+"stateled2:"+stateLed2);
      if(stateLed2 == 1 && stateAuto == 0){ digitalWrite( led2,LOW);}
      else if(stateLed2 == 0 && stateAuto == 0){ digitalWrite( led2,HIGH); }
    }
  else{
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }
    
  stateLed3 = ThingSpeak.readIntField(weatherStationChannelNumber,4); // Field 4
  statusCode = ThingSpeak.getLastReadStatus();
  if(statusCode == 200)
    {
      Serial.println(String()+"stateled3:"+stateLed3);
      if(stateLed3 == 1 && stateAuto == 0){ digitalWrite( led3,LOW);}
      else if(stateLed3 == 0 && stateAuto == 0){ digitalWrite( led3,HIGH); }
    }
  else{
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }
  }


  

if(stateAuto){ 
      blink(1); 
      kalkulasi(); 
    }
    else if(!stateAuto){ 
      blink(0); 
      stateRun = 0; 
    }
  

  

  

  digitalWrite(dimmer_pin[0],stateRun);

}

int conversion(int raw_val){
  // Conversion rule
  float Vout = float(raw_val) * (VIN / float(1023));// Conversion analog to voltage
  float RLDR = (R * (VIN - Vout))/Vout; // Conversion voltage to resistance
  int lux = 500/(RLDR/1000); // Conversion resitance to lumen
  return lux;
}

void kalkulasi(){
  
//  if(LDR_Val <= 450 ){
//    
//     stateLedAuto2 = 1;
//     stateLedAuto3 = 1;
//  }
//
//  else if(LDR_Val >= 1000 ){
//    stateLedAuto1 = 0;
//     stateLedAuto2 = 0;
//     stateLedAuto3 = 0;
//  }

  if(stateLed1 == 1 && stateAuto == 1){ digitalWrite( led1,LOW);}
  else if (stateLed1 == 0 && stateAuto == 1){ digitalWrite( led1,HIGH); }

  if(stateLed2 == 1 && stateAuto == 1){ digitalWrite( led2,LOW);}
  else if (stateLed2 == 0 && stateAuto == 1){ digitalWrite( led2,HIGH); }

  if(stateLed3 == 1 && stateAuto == 1){ digitalWrite( led3,LOW);}
  else if(stateLed3 == 0 && stateAuto == 1){ digitalWrite( led3,HIGH); }
}






void stateWIFI() {
  // check WiFi connection:
  if (WiFi.status() != WL_CONNECTED)
  {
    // (optional) "offline" part of code
    sRun = false;
    analogWrite(led_pin,LOW);
    // check delay:
    if (millis() - lastConnectionAttempt >= connectionDelay)
    {
      lastConnectionAttempt = millis();
        Serial.println(String()+"disconnect");
      // attempt to connect to Wifi network:
        //WiFi.begin((char*)ssid, (char*)pass);
        //WiFi.begin(WiFi.SSID(), WiFi.psk());
        wm.setConfigPortalTimeout(5);
       if(!wm.autoConnect())
       {
         Serial.println("auto connect failed");
         delay(1000);
         ESP.restart();
       }
       else{
        ThingSpeak.begin(client); 
        ArduinoOTA.begin();
        Serial.println("RECONNECT");
       }
    }
  }
  else if(WiFi.status() == WL_CONNECTED){ blinkRun(1); }
}

/*
void checkButton()
{
  if(digitalRead(button) == HIGH)
  {
    stateWifi = !stateWifi;
    EEPROM.write(305,stateWifi);
    EEPROM.commit();
    Serial.println(String() + "button ditekan,stateWifi:" + stateWifi + " stateMode:" + stateMode);
  }
     
  if(stateMode != stateWifi)
  {  
    Serial.println("mode berubah");
    for (int i = 0; i < 40; i++) 
   {
     analogWrite(led_pin, (i * 100) % 1001);
     analogWrite(dimmer_pin[0], (i * 100) % 1001);
//      for(int a=0;a<2;a++){ strip.setPixelColor(dot2[a],strip.Color((i * 100) % 1001,0,0)); strip.setPixelColor(dot1[a],strip.Color((i * 100) % 1001,0,0)); strip.show();}
//      if(i % 2){buzzer(1);}
//      else{buzzer(0);}
     delay(50);
   }
    //buzzer(1);
    delay(1000);
    ESP.restart();
  }  
}*/

void blink(int state){
  if(!state) return;

  unsigned long tmr = millis();
  static unsigned long saveTime = 0;

  if((tmr - saveTime) > 1000){
    saveTime = tmr;
    stateRun = !stateRun;
  }
}

void blinkRun(int state){
  if(!state) return;

  unsigned long tmr = millis();
  static unsigned long saveTime = 0;

  if((tmr - saveTime) > 1000){
    saveTime = tmr;
    sRun = !sRun;
  }
  digitalWrite(led_pin,sRun);
}
