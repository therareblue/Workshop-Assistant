//------------------- LIBRARIES -----------------------
#include "FastLED.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
//------------------- MAIN stuff ----------------------
byte workshop_status = 0; //means OFF. 1 means ON
byte lampa_last = 255; //this parameter saves lampa brightness. When we turn-on workshop, lampa brightnes is taked from here. 

//------------------- LED param -----------------------
#define PIXPIN 14
#define LED_N 8
CRGB leds[LED_N];

byte red = 0; //color of the pixels. 0 is red.
byte aqua = 128; 
byte green = 96; 
byte blue = 145; 
byte yellow = 64;  

byte ledc0, ledc1 = 0; //led counter 0 -> counter for wifi-led change signal, 1-> for workshop ON state indicator
unsigned long lastled = 0; //timestamp for 0->wifi led;
byte led_cmd; // command for controlling the leds. Turn-on different function.

//bool fade_in = true;
int fade_bri= 55;
int fade_bri1 = 0;
int fade_am = 1; //fade amount
int fade_am1 = 1;

//int ctra = 0;

void ledClear(){
  leds[0] = CHSV(0, 255, 0);
  leds[1] = CHSV(0, 255, 0);
  leds[2] = CHSV(0, 255, 0);
  leds[3] = CHSV(0, 255, 0);
  leds[4] = CHSV(0, 255, 0);
  leds[5] = CHSV(0, 255, 0);
  leds[6] = CHSV(0, 255, 0);
  leds[7] = CHSV(0, 255, 0);
  FastLED.show();
}
void wifi_led(){ 
  switch (ledc0){
    case 0:
      {
        leds[4] = CHSV(yellow, 255, 100); 
        leds[5] = CHSV(yellow, 255, 0); 
        leds[6] = CHSV(yellow, 255, 0);
        leds[7] = CHSV(yellow, 255, 0);
      }
    break;
    case 1:
      {
        leds[4] = CHSV(yellow, 255, 0); 
        leds[5] = CHSV(yellow, 255, 100); 
        leds[6] = CHSV(yellow, 255, 0);
        leds[7] = CHSV(yellow, 255, 0);
      }
    break;
    case 2:
      {
        leds[4] = CHSV(yellow, 255, 0); 
        leds[5] = CHSV(yellow, 255, 0); 
        leds[6] = CHSV(yellow, 255, 100);
        leds[7] = CHSV(yellow, 255, 0);
      }
    break;
    case 3:
      {
        leds[4] = CHSV(yellow, 255, 0); 
        leds[5] = CHSV(yellow, 255, 0); 
        leds[6] = CHSV(yellow, 255, 0);
        leds[7] = CHSV(yellow, 255, 100);
      }
    break;
  } 
  if (ledc0 >= 3)
    ledc0 = 0;
  else ledc0++;
  FastLED.show();
}
void mqtt_led(){
  ledClear();
  for (int i=0; i<=6; i++){
    leds[4] = CHSV(red, 255, 255); 
    leds[5] = CHSV(red, 255, 255); 
    leds[6] = CHSV(red, 255, 255);
    leds[7] = CHSV(red, 255, 255);
    FastLED.show();
    delay(500);
    leds[4] = CHSV(red, 255, 0); 
    leds[5] = CHSV(red, 255, 0); 
    leds[6] = CHSV(red, 255, 0);
    leds[7] = CHSV(red, 255, 0);
    FastLED.show();
    delay(500);
  }
}

void ledOn(byte cmd, unsigned long tstamp){
  switch (cmd){
    case 0:{ //led indicator for online, workshop OFF.
      if (tstamp - lastled >= 20){
        //ctra++;
        //Serial.print("led_change:"); Serial.println(ctra);
        lastled = tstamp;

        fade_bri = fade_bri + fade_am;
        fade_bri1 = fade_bri1 + fade_am1;
        if (fade_bri <= 55 || fade_bri >= 255){
          fade_am = - fade_am;
        }
        if (fade_bri1 <= 0 || fade_bri1 >= 185){
          fade_am1 = - fade_am1;
        }

        //if (fade_bri <0) fade_bri =0;
        //else if(fade_bri >255) fade_bri=255;
        
        leds[0] = CHSV(blue, 255, fade_bri);
        //int fade_bri1 = fade_bri / 2;
        leds[1] = CHSV(red, 255, fade_bri1);
        leds[2] = CHSV(red, 255, fade_bri1);
        leds[3] = CHSV(red, 255, fade_bri1);
        FastLED.show();
      }
    }
    break;
    case 1:{ // led indicator for online, workshop ON
      if (tstamp - lastled >= 500){
        lastled = tstamp;
        leds[0] = CHSV(green, 255, 0); 
        leds[0] = CHSV(green, 255, 255); 
        switch (ledc1){
          case 0:{
            leds[1] = CHSV(aqua, 255, 150); 
            leds[2] = CHSV(aqua, 255, 0); 
            leds[3] = CHSV(aqua, 255, 0);
          }
          break;
          case 1:{
            leds[1] = CHSV(aqua, 255, 0); 
            leds[2] = CHSV(aqua, 255, 150); 
            leds[3] = CHSV(aqua, 255, 0);
          }
          break;
          case 2:{
            leds[1] = CHSV(aqua, 255, 0); 
            leds[2] = CHSV(aqua, 255, 0); 
            leds[3] = CHSV(aqua, 255, 150);
          }
          break;
          case 3:{
            leds[1] = CHSV(aqua, 255, 0); 
            leds[2] = CHSV(aqua, 255, 150); 
            leds[3] = CHSV(aqua, 255, 0); 
          }
          break;
        } 
        if (ledc1 >= 3)
          ledc1 = 0;
        else ledc1++;
        FastLED.show();
      }
    }
    break;
  }  
}
//---------------------------------------------------------------
//-------------------- BTN param ----------------------
#define BTN0 27
int btn_int = 200;  
int btn_counter = 0; // counts numbers for pressed button. when = to 15 this means btn is halt for 200x15 = 3000ms and it activates
unsigned long last_btn = 0;
bool on_flag = false;

//---------------------------------------------------------------
//------------------ irf-KEY param ---------------------
#define RST_PIN         17          // Configurable, see typical pin layout above
#define SS_PIN          5         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
int scanInterval = 500;
unsigned long last_scan = 0;

bool autorizeCheck(){
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return false;
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return false;
  
  //Show UID on serial monitor
  //Serial.print("UID tag :");
  String content="";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++){
     //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     //Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  //Serial.println();
  //Serial.print("Message : ");
  content.toUpperCase();
  //Serial.println(content.substring(1));
  if (content.substring(1) == "04 1E 8C 22 5D 67 80" || content.substring(1) == "63 DC D9 1A") //change here the UID of the card/cards that you want to give access
    return true;
  else
    return false; 
}

//---------------------------------------------------------------
//------------------ SOUNDI param ---------------------
#define BUZZER_PIN 13
#define BUZZER_CHANNEL 0
void playme(byte cmd){
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  switch (cmd){
    case 0: //security event
      {
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 5);
        delay(500);
      }
    break;
    case 1: //err
      {
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(100);
      }
    break;
    case 2: //turn off
      {
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_E, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_G, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_B, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_G, 5);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_E, 5);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 5);
        delay(500);
      }
    break;
    case 3: //turn on
      {
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 5);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_E, 5);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_G, 5);
        delay(50);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_B, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_G, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_E, 5);
        delay(100);
        ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
        delay(500);
      }
    break;
    case 4: //security OM timeout
      {
        for (int i=0; i<=5; i++){
          ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
          delay(100);
          ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
          delay(50);
          ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
          delay(100);
          ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
          delay(800);
        }
      }
    break;
    
  } 
  ledcDetachPin(BUZZER_PIN); 
}

//---------------------------------------------------------------
//--------------- WiFi & MQTT param -------------------
const char *ssid =  "MyNetwork";   // name of your WiFi network
const char *password =  "verystrongpassword";
const char* mqtt_server = "192.168.0.173";
const char *ID = "idBox";  // Name of our device, must be unique

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long last_ping = 0;
int ping_interval = 30000; //every 30 seconds idbox send ping signals to all devices
bool table_check, lampa_check, stol_check, emma_check, coffee_check, lab_check, garden_check = false; // index wheither device is connected or not.

void setup_wifi() {
  // We start by connecting to a WiFi network
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    wifi_led();
    //Serial.print(".");
  }
  ledClear();
  //Serial.println("");
  //Serial.print("WiFi connected - ESP IP address: ");
  //Serial.println(WiFi.localIP());
}
void reconnect() {
  // Loop until we're reconnected to MQTT server:
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    if (client.connect(ID)) {
      //Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      client.subscribe("table/check");
      client.subscribe("lampa/check");
      client.subscribe("stol/check");
      client.subscribe("emma/check");
      //client.subscribe("coffee/check");
      //client.subscribe("lab/check");
      //client.subscribe("garden/check");
      client.subscribe("id/cmd");
      client.subscribe("id/check"); //ping from emma to id to check if all locked
      
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(5000); 
      mqtt_led(); //this function will delay 5sec with blinkin leds.
    }
  }
  pingIt();
}
void callback(char* topic, byte* msg, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    //Serial.print((char)msg[i]);
    message += (char)msg[i];
  }
  //Serial.print("topic: "); Serial.print(topic); Serial.print(" -> message: "); Serial.println(message);
  if (strcmp(topic, "table/check")==0 && message != "-"){
    if (message == "0"){
      leds[4] = CHSV(red, 255, 255); //table is off, but online.
      /*// check if main state is on or off, and change the device acordingly:
      if (workshop_status == 1){
        //if table says is off, but must be on, idbox send command to turn on.
        client.publish("table", "1");
      }*/
    }else if (message == "1"){
      leds[4] = CHSV(green, 255, 255); //table is on, but online.
      /*if (workshop_status == 0){
        //if table says is on, but must be off, idbox send command to turn off.
        client.publish("table", "0");
      }*/
    }
    table_check = true;
  } else 
  
  /*if (strcmp(topic, "lampa")==0  && message != "0"){
    byte lampa_pwm = message.toInt(); 
    //Serial.print("received lampa_pwm ");Serial.println(lampa_pwm);
    if (lampa_pwm != 0)
      lampa_last = lampa_pwm; // save a stamp from the current lampa brightness.
  } else */
  
  if (strcmp(topic, "lampa/check")==0  && message != "-"){
    byte lampa_pwm = message.toInt(); 
    byte clr = lampa_pwm / 2;
    leds[5] = CHSV(clr, 255, 255); // led color depends on lampa current brightness pwm.
    /*if (lampa_pwm != 0)
      lampa_last = lampa_pwm; // save a stamp from the current lampa brightness.
     
    if (message == "0" && workshop_status == 1){
      char pubuff [4];
      sprintf(pubuff, "%i", lampa_last); // %d decimal; %i integer; %s string; %u unsigned decimal integer
      client.publish("lampa", pubuff);
    }else if (message =="1" && workshop_status == 0){
      client.publish("lampa", "0");
    }*/
    lampa_check = true;
  } else

  if (strcmp(topic, "stol/check")==0 && message != "-"){
    /*if (message == "0" && workshop_status == 1){
      client.publish("stol", "1");
    }else if (message =="1" && workshop_status == 0){
      client.publish("stol", "0");
    }*/
    if (message == "0") 
      leds[6] = CHSV(red, 255, 255); //stol is off, but online.
    else if (message == "1")
      leds[6] = CHSV(green, 255, 255); //stoll is online and on.
    stol_check = true;
  } else
  if (strcmp(topic, "emma/check")==0 && message != "-"){
    if (message == "0") 
      leds[7] = CHSV(red, 255, 255); //emma is off, but online.
    else if (message == "1")
      leds[7] = CHSV(green, 255, 255); //emma is online and on.
    emma_check = true;
  } else
  if (strcmp(topic, "id/cmd")==0){
    if (message == "0"){
      // LOCK IN everything using emma
      if (workshop_status != 0){
        lockin();
      }
    }
    else if (message == "1"){
      //UNLOCK the workshop... using emma
      if (workshop_status == 0){
        unlock();
      }
    }  
  }else 
  if (strcmp(topic, "id/check")==0 && message == "-"){
    if (workshop_status == 0)
      client.publish("id/check", "0");
    else if (workshop_status == 1)
      client.publish("id/check", "1");
  }
}

void pingIt(){
  last_ping = millis();
    //Serial.println("PING for status...");
    // if we do not receive anser and table_check remain false, we turn LED off.
    if (!table_check)
      leds[4] = CHSV(0, 255, 0); //table is offline.
    if (!lampa_check)
      leds[5] = CHSV(0, 255, 0); //lampa is offline.
    if (!stol_check)
      leds[6] = CHSV(0, 255, 0); //stol is offline.
    if (!emma_check)
      leds[7] = CHSV(0, 255, 0); //EMMA is offline.
    //there is no led for other devices.
   
    //ping again.
    table_check = false;
    client.publish("table/check", "-");
    lampa_check = false;
    client.publish("lampa/check", "-");
    stol_check = false;
    client.publish("stol/check", "-");
    emma_check = false;
    client.publish("emma/check", "-");
    //coffee_check = false;  //only ping the machine. Emma also listen and update its status. So emma do not need to ping itself.
    client.publish("coffee/check", "-");
    client.publish("teapot/check", "-");
    //lab_check = false;  //only ping the machine. Emma also listen and update its status. So emma do not need to ping itself.
    client.publish("lab/check", "-");   
    //garden_check = false;  //only ping the machine. Emma also listen and update its status. So emma do not need to ping itself.
    client.publish("garder/check", "-");
}
//---------------------------------------------------------------
//--------------- SECURITY Vl53l0x --------------------
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#define I2C_SDA 33
#define I2C_SCL 32
TwoWire I2CL0X = TwoWire(0);

unsigned long last_measure = 0;

int laserMeasure(){
  int tof_mm = 0;
  //Serial.println("Scan Overlay...");
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  delay(15);
  if (measure.RangeStatus != 4){
    tof_mm = measure.RangeMilliMeter;
    Serial.print("-->"); Serial.println(tof_mm);
  }
  else{
    tof_mm = 2500;
  }
  return tof_mm;
}

void alarm_it(){
  client.publish("security/event", "1");
  
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledClear();
  for (int i=0; i<9; i++){
    leds[0] = CHSV(0, 255, 255); 
    leds[1] = CHSV(0, 255, 0);
    leds[2] = CHSV(0, 255, 255); 
    leds[3] = CHSV(0, 255, 0);
    FastLED.show();
    ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 6);
    delay(100);
    ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
    delay(50);
    leds[0] = CHSV(0, 255, 0); 
    leds[1] = CHSV(0, 255, 255);
    leds[2] = CHSV(0, 255, 0); 
    leds[3] = CHSV(0, 255, 255);
    FastLED.show();
    ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 5);
    delay(200);
    leds[0] = CHSV(0, 255, 255); 
    leds[1] = CHSV(0, 255, 0);
    leds[2] = CHSV(0, 255, 255); 
    leds[3] = CHSV(0, 255, 0);
    FastLED.show();
    delay(200);
    leds[0] = CHSV(0, 255, 0); 
    leds[1] = CHSV(0, 255, 255);
    leds[2] = CHSV(0, 255, 0); 
    leds[3] = CHSV(0, 255, 255);
    FastLED.show();
    ledcWriteNote(BUZZER_CHANNEL, NOTE_C, 0);
    delay(100);
  }
  ledcDetachPin(BUZZER_PIN); 
  
  client.publish("security/event", "0");
  pingIt();
}

void unlock(){
  //Serial.println("Access Granted!");
  playme(3);
  workshop_status = 1;
  /*char pubuff [4];
  //sprintf(pubuff, "%i", lampa_last); // %d decimal; %i integer; %s string; %u unsigned decimal integer
  client.publish("lampa", pubuff);*/
  client.publish("lampa/cmd", "1");
  delay(10);
  client.publish("table/cmd", "1");
  delay(10);
  client.publish("id/check", "1");
  delay(10);
}
void lockin(){
  //Serial.println("btn is pressed. the workshop will now turn off.");  
  playme(2);
  client.publish("table/cmd", "0");
  delay(10);
  client.publish("lampa/cmd", "0");
  delay(10);
  client.publish("stol/cmd", "0");
  delay(10);
  client.publish("id/check", "0"); //id sends to the listeners that all is locked.
  delay(10);
  client.publish("coffee/cmd", "0");
  delay(10);
  //delay 5 seconds and play sound, until turn security ON:
  delay(1000);
  playme(4);
  workshop_status = 0;
}

//---------------------------------------------------------------
//==================== VOID SETUP =====================
void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  FastLED.addLeds<NEOPIXEL, PIXPIN>(leds, LED_N);
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(BTN0, INPUT_PULLUP);

  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  I2CL0X.begin(I2C_SDA, I2C_SCL, 100000);
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(500);
  if (!lox.begin(0x29, &I2CL0X)) {
    Serial.println("TOF sensor not found!");
    //while(1);
  }delay(500);
}

//==================== VOID LOOP ======================
void loop() {
  if (!client.connected())  // Reconnect if connection is lost
    reconnect();    
  client.loop();

  unsigned long cmillis = millis();
  ledOn(workshop_status, cmillis);
  
  if (workshop_status == 0){ //if workshop is OFF, security system is active.
    if (cmillis - last_measure >= 500){
      last_measure = cmillis;
      int distance = laserMeasure();
      if (distance > 100 && distance < 1200){
        delay(50);
        distance = laserMeasure();
        if (distance > 100 && distance < 1200){
          delay(50);
          distance = laserMeasure();
          if (distance > 100 && distance < 1200){
            alarm_it();
          }
        }
      }
    }
  }
  
  //1. ping on every 30 seconds to check what device is online.
  // if the device is online, it sends back to the "check" topic its state.
  if (cmillis - last_ping >= ping_interval){
    pingIt();          
  }

  //2. check if btn is pressed and hold:
  if (workshop_status != 0){
    if (cmillis - last_btn >= btn_int){ //check if btn is pressed every 200ms
      last_btn = cmillis;
      int btn0 = digitalRead(BTN0);
      if (btn0 == LOW){
        if (btn_counter == 15){
          if (workshop_status != 0){
            lockin();
            btn_counter = 0;
          }
        }else{
          btn_counter++;
        }
      }else{//btn is not pressed:
        btn_counter = 0;
      }
    }
  }
  

  //3. Check if IRF key is presented:
  if (workshop_status == 0){
    if (cmillis - last_scan >= scanInterval){
      last_scan = cmillis;
      if (autorizeCheck()){
          unlock();
          
      }
    }
  }
  
  
}
