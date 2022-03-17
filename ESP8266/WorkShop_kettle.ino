//developed for esp8266 module esp-07
//used one DS18B20 temperature sensor and a 15Amp relay controled by mosfet transistor.
// When Iddle, Temperature is measured constantly and the LED color is updated. 
// If a temperature increase is detected without a comand, this means that kettle was turned-on manualy. A proper warning is sent over MQTT and the LED starts flashng.


#include <OneWire.h>
#include <DallasTemperature.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define TP 4 //temperature pin
#define RP 14 //rellay pin
#define LP 13 //led pin
#define BP 5 //btn pin - for OTA init

#include <Adafruit_NeoPixel.h>

OneWire oneWire(TP);
DallasTemperature sensors(&oneWire);
int temperature;

const char *ssid =  "MyNetwork";   // name of your WiFi network
const char *password =  "verystrongpassword";
const char* mqtt_server = "192.168.0.173";
const char *ID = "teapot";  // Name of our device, must be unique

// ----- OTA settings -------
//Because I integrated the board into the kettle, I use OTA script to be able to upload firmware updates remotely (without using usb).
const char* ota_host = "esp32";
const char* ota_ssid = "zzzz";
const char* ota_password = "111222333";
int ota_channel = 11;
int ota_ssid_hidden = 0;
int ota_max_connection = 1;
IPAddress apIP(192, 168, 4, 1);
bool isOta = false;
//---------------------------
unsigned long last_ota = 0;
unsigned long last_tmp = 0;

unsigned long last_check = 0;
bool teapot_off = true;
int last_temperature; //this saves the last measured temperature. if next temperature is few degrees higher, it sends message to the server.
bool warning_send = false;
bool heating_up = false;
int req_temperature = 100; 

//-----------LED-------------
#define LED_COUNT 1
Adafruit_NeoPixel strip(LED_COUNT, LP, NEO_GRB + NEO_KHZ800);
unsigned long last_led = 0;
bool led_on = false;
byte led_cmd = 0;

void ledOn(byte cmd){
  switch (cmd){
    case 0:
      {//this turns neopixel OFF.
        uint32_t color = strip.Color(0, 0, 0); //red
        strip.setPixelColor(0, color); //set pixel 0 to color "color".
        //strip.setBrightness(0);
        strip.show();  
        led_on = false;
      }break;
    case 1:
      {//led command for OTA active: (everly 500ms blink in blue);
        if (millis() - last_led >= 500){
          last_led = millis();
          if (led_on){
            //strip.setBrightness(0);
            uint32_t color = strip.Color(0, 0, 0); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = false;
          }else{
            //strip.setBrightness(80);
            uint32_t color = strip.Color(0, 0, 255); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = true;
          }
        }
      }break;
    case 2:
      {//led command for OTA upload: (everly 200ms blink in purple);
        /*
        if (millis() - last_led >= 200){
          last_led = millis();
          if (led_on){
            //strip.setBrightness(0);
            uint32_t color = strip.Color(0, 0, 0); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = false;
          }else{
            //strip.setBrightness(80);
            uint32_t color = strip.Color(100, 0, 150); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = true;
          }
        }*/
        int32_t color = strip.Color(255, 0, 255); //purple
        strip.setPixelColor(0, color); //set pixel 0 to color "color".
        //strip.setBrightness(0);
        strip.show();  
        led_on = true;
      }break;
    case 3:
      {//General error: (everly 300ms blink in red);
        if (millis() - last_led >= 300){
          last_led = millis();
          if (led_on){
            //strip.setBrightness(0);
            uint32_t color = strip.Color(0, 0, 0); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = false;
          }else{
            //strip.setBrightness(80);
            uint32_t color = strip.Color(255, 0, 0); //blue
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = true;
          }
        }
      }break;
    case 4:
      {// led command for Teapot online and ready. permanent green.
        if (millis() - last_led >= 2000){
          last_led = millis();
          uint32_t hsv_clr = temperature_color();
          strip.setPixelColor(0, hsv_clr); //set pixel 0 to color "color".
          //strip.setBrightness(80);
          strip.show();  
          led_on = true;
        }
      }break;
    case 5:
      {// flashing led for heat on. flas with the color of temperature.
        if (millis() - last_led >= 250){
          last_led = millis();
          if (led_on){
            //strip.setBrightness(0);
            uint32_t color = strip.Color(0, 0, 0); //off
            strip.setPixelColor(0, color); //set pixel 0 to color "color".
            strip.show();  
            led_on = false;
          }else{
            //strip.setBrightness(80);
            uint32_t hsv_clr = temperature_color();
          
            strip.setPixelColor(0, hsv_clr); //set pixel 0 to color "color".
            strip.show();  
            led_on = true;
          }
        }
      }break;
    
  }
}
//---------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  Serial.println("connecting to WIFI...");
  WiFi.begin(ssid, password);
  delay(200);
  bool l_on = false;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (l_on){
      uint32_t color = strip.Color(0, 0, 0); //green-blue
      strip.setPixelColor(0, color); //set pixel 0 to color "color".
      strip.show(); 
      l_on=false;
    }else{
      uint32_t color = strip.Color(0, 150, 50); //green-blue
      strip.setPixelColor(0, color); //set pixel 0 to color "color".
      strip.show(); 
      l_on=true;
    }
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: "); Serial.println(WiFi.localIP());
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(ID)) {
      Serial.println("connected.");  
      
      // Subscribe or resubscribe to a topic
      client.subscribe("teapot/check"); 
      client.subscribe("teapot/cmd"); 
      client.subscribe("teapot/tmp"); 
      delay(500);
      client.publish("teapot/check", "0");     
      last_check = millis();
      
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000); 
    }
  }
}

void callback(char* topic, byte* msg, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    //Serial.print((char)msg[i]);
    message += (char)msg[i];
  }
  Serial.print("topic: "); Serial.print(topic); Serial.print(" -> message: "); Serial.println(message);
  char pubuff [4];

  if (strcmp(topic, "teapot/check")==0 && message == "-"){
    if (teapot_off)
      client.publish("teapot/check", "0");
    else
      client.publish("teapot/check", "1"); 
      
    char pubuff [4];
    sprintf(pubuff, "%i", temperature);
    client.publish("teapot/tmp", pubuff);
  
  }

  else if (strcmp(topic, "teapot/cmd")==0){
    if (message == "1"){
      req_temperature = 100;
      digitalWrite(RP, HIGH);
      teapot_off = false;
      client.publish("teapot/check", "1");
      
    }else if (message == "0"){
      teapotOff();
    }
    else{
      //check if number is sent and change the req_temperature value, so the teapor will stop there.
      
      req_temperature = message.toInt();
      digitalWrite(RP, HIGH);
      teapot_off = false;
      client.publish("teapot/check", "1");        
    }
  }

  else if (strcmp(topic, "teapot/tmp")==0 && message == "-"){
    char pubuff [4];
    sprintf(pubuff, "%i", temperature);
    client.publish("teapot/tmp", pubuff);
  }
  
}

void teapotOff(){
  pinMode(RP, OUTPUT);
  digitalWrite(RP, LOW);
  teapot_off = true;
  last_temperature = temperature;
  warning_send = true;
  req_temperature = 100; 
  client.publish("teapot/check", "0");
  led_cmd = 4;
}

void update_tmp(){
  //this funcrion updates the global temperature value.
  //it also detects if there is a rapid increase of t and sends to the mqtt server a message.
  sensors.requestTemperatures();
  float tC = sensors.getTempCByIndex(0);
  temperature = round(tC + tC/10);
  
  if (temperature > 20){
    if((temperature-last_temperature) >= 3){
      heating_up = true;
      if (teapot_off){
        if (!warning_send){
          client.publish("teapot/check", "2"); //"2" means WARNING, the teapot is heating up, but is off..
          warning_send = true;
        }
      }
      //led_cmd = 5;
    }else{
      heating_up = false;
      warning_send = false;
      //led_cmd = 4;
    }
  }else{
    warning_send = false;
    //led_cmd = 4;
  } 
  last_temperature = temperature;
  if (temperature >= req_temperature){
    teapotOff();
  }
  //Serial.print(tC);
  //Serial.println("ºC");
}

uint32_t temperature_color(){
  uint16_t tempClr = 25000 - temperature * 260; //if temp=100, clr=0 red/purple; if 0: 30000 cyan
  //from 0 to 65535; 0 is red
  //10992 is yellow ; 21845 is green; 32768 is cyan; 43691 is blue;54613 is violet.
  if (tempClr < 0) tempClr = tempClr + 65535;
  else if (tempClr > 65535) tempClr - 65535;
  uint32_t clr = strip.ColorHSV(tempClr, 255, 255);
  return clr;
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  strip.begin();           
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255);
  
  pinMode(BP, INPUT_PULLUP);
  if (digitalRead(BP) == 0){
    //run OTA:
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ota_ssid, ota_password, ota_channel, ota_ssid_hidden, ota_max_connection);
    IPAddress IP = WiFi.softAPIP();
    if (!MDNS.begin(ota_host)) {
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("MDNS responder started.");
    
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      led_cmd = 2;  
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Serial.print("Upploadubg the " + type + "...");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      led_cmd = 2;
      //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      led_cmd = 3;
    });
    ArduinoOTA.begin();
    Serial.println("OTA ready!");
    isOta = true;
    led_cmd = 1;
    //--------
  }
  else{
    isOta = false;
    
    pinMode(RP, OUTPUT);
    digitalWrite(RP, LOW);
    
    sensors.begin();
    delay(500);
  
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    led_cmd = 4;

    //measure the temperature and update the last temperature:
    sensors.requestTemperatures();
    float tC = sensors.getTempCByIndex(0);
    delay(100);
    last_temperature = round(tC);
    last_tmp = millis();
    warning_send = true;
    delay(1000);
    
  }
}

void loop() {
  ledOn(led_cmd);
  
  if (isOta){
    if (millis() - last_ota >= 5000){
      last_ota = millis();
      ArduinoOTA.handle();
    }
    //delay(5000);
  }else{
    if (!client.connected())  // Reconnect if connection is lost
      reconnect();    
    client.loop();

    //update temperature
    if (millis() - last_tmp >= 5000){
      last_tmp = millis();
      update_tmp();
      //Serial.println(temperature);
      if (!teapot_off || heating_up){
        char pubuff [4];
        sprintf(pubuff, "%i", temperature);
        client.publish("teapot/tmp", pubuff);
        led_cmd = 5;
      }else{
        led_cmd = 4; 
      }
    }
    
    
  }
  
  
}
