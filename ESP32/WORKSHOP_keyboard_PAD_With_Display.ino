#include <ArduinoJson.h>

// ------------------- DISPLAY --------------------
#include <TFT_eSPI.h>   //library for the display
TFT_eSPI tft = TFT_eSPI();  //initialize the display variable
byte refreshCounter = 0;
// ------------------------------------------------
void showIt(const char* tme, float tout, float twsh, float hwsh, int pwsh, int co2w){
  tft.fillScreen(TFT_BLACK);
  //tft.setTextColor(TFT_RED, TFT_BLACK);
  //int rnd = random(20);
  //int x = random(10, 280);
  //int y = random(20, 200);
  //Serial.println(rnd);
  
  
  tft.setTextDatum(MC_DATUM); 
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString(String(tme), 160, 10, 2);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("Outside Temp:", 10, 48, 2);
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.drawString(String(tout, 1), 10, 100, 6);
  tft.drawLine(155, 30, 155, 95, TFT_WHITE);
  if (tout <10 && tout >0){
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("`C", 85, 93, 4);
  }else{
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("`C", 110, 93, 4);
  }
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("Workshop Temp:", 180, 48, 2);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK); tft.drawString(String(twsh, 1), 180, 100, 6);
  
  if (twsh <10 && twsh >0){
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("`C", 255, 93, 4);
  }else{
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("`C", 280, 93, 4);
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("Air Pressure:", 10, 120, 2);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK); tft.drawString(String(pwsh), 10, 150, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("hPa", 70, 145, 2);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("Workshop Humidity:", 155, 120, 2);
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK); tft.drawString(String(hwsh, 1), 155, 150, 4);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("%", 210, 145, 2);

  tft.drawLine(10, 160, 310, 160, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawString("Workshop AIR Quality:", 10, 185, 2);

  
  if (co2w < 600){
    tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString("EXCELLENT", 160, 185, 2);
  }
  else if (co2w>=600 && co2w<800){
    tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK); tft.drawString("VERY GOOD", 160, 185, 2);
  }
  else if (co2w>=800 && co2w<1000){
    tft.setTextColor(TFT_GOLD, TFT_BLACK); tft.drawString("STILL OK", 160, 185, 2);
  }
  else if (co2w>=1000) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK); tft.drawString("WINDOW SHOULD BE OPEN", 155, 185, 2); 
  }
    
  
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_GOLD, TFT_BLACK); tft.drawString(String(co2w), 130, 240, 6);
  if (co2w <1000){
    tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.drawString("ppm", 200, 232, 4);
    
  }else{
    tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.drawString("ppm", 220, 232, 4); 
    
    
  }
}
//-------------------------------------------------
//------------------- LEDs ------------------------
#include "FastLED.h"
#define PIXPIN 14 //adressed leds control pin
#define LED_N 6 //how many addressed LEDs we have in a row
CRGB leds[LED_N];
byte side_color = 0; // : 0-255,  0 is red.

int ledInterval = 15;
unsigned long lastLed = 0;
//-------------------------------------------------
// --------------------- MQTT ---------------------
#include <WiFi.h>
#include <PubSubClient.h>

const char *ssid =  "MyNetwork";   // name of your WiFi network
const char *password =  "verystrongpassword";
const char* mqtt_server = "192.168.0.173";
const char *ID = "keyb";  // Name of our device, must be unique

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long last_check = 0; // stores the last tiem we check the server_status.
bool isconnected = false;

void setup_wifi() {
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    if (client.connect(ID)) {
      Serial.println("connected");  

      tft.setTextDatum(MC_DATUM); 
      tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawString("connected", 160, 55, 2);
  
      // Subscribe or resubscribe to a topic
      client.subscribe("sensors/tblog"); //every 30 second server send here "-" and lampa must update it to 0 or 1. If not, server knows this is offline.
      
      isconnected = true;
      last_check = millis();
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      isconnected = false;
      Serial.println("not connected. reconnect in 5 sec.");
      // Wait 5 seconds before retrying
      delay(5000); 
    }
  }
}

void callback(char* topic, byte* msg, unsigned int length) {
  
  char *message = (char *) msg;
  message[length]='\0';
  if (strcmp(topic, "sensors/tblog")==0){
    Serial.println(message);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, message);

    const char* tme = doc["time"];
    float tout = doc["to"];
    float twsh = doc["tw"];
    float hwsh = doc["hw"];
    int pwsh = doc["pw"];
    int co2w = doc["co2w"];

    showIt(tme, tout, twsh, hwsh, pwsh, co2w);

    isconnected = true;
    last_check = millis();
  }
}
// ------------------------------------------------

// ------------------------------------------------
void ledClear(){
  leds[0] = CHSV(0, 255, 0);
  leds[1] = CHSV(0, 255, 0);
  leds[2] = CHSV(0, 255, 0);
  leds[3] = CHSV(0, 255, 0);
  leds[4] = CHSV(0, 255, 0);
  leds[5] = CHSV(0, 255, 0);
  FastLED.show();
}
void ledOn(byte color){
  leds[0] = CHSV(color, 255, 255);
  leds[1] = CHSV(color, 255, 255);
  leds[2] = CHSV(color, 255, 255);
  leds[3] = CHSV(color, 255, 255);
  leds[4] = CHSV(color, 255, 255);
  leds[5] = CHSV(color, 255, 255);
  FastLED.show();
}
void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  FastLED.addLeds<NEOPIXEL, PIXPIN>(leds, LED_N);
  
  tft.init();
  tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  
  
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  tft.setTextDatum(MC_DATUM); 
  tft.setTextColor(TFT_SKYBLUE, TFT_BLACK); tft.drawString("WAITING FOR DATA...", 160, 30, 2);
  
  ledOn(0);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  
}

void loop() {
  if (!client.connected())  // Reconnect if connection is lost
    reconnect();    
  client.loop();
  
  // put your main code here, to run repeatedly:
  //showIt();
  delay(2000);
  side_color = side_color+1;
  if (side_color >255)
    side_color = 0;

  ledOn(side_color);
}
