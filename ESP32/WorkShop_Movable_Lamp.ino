#include <WiFi.h>
#include <PubSubClient.h>

#define DIRPIN 15
#define STEPPIN 5
#define ENPIN 17
#define STOPER 27
#define RELAY 13
#define OTG_PIN 19



const char *ssid =  "MyNetwork";   // name of your WiFi network
const char *password =  "verystrongpassword";
const char* mqtt_server = "192.168.0.173";
const char *ID = "lampa";  // Name of our device, must be unique

WiFiClient espClient;
PubSubClient client(espClient);

byte lampa_pwm=255;
byte lampa_high = 0;
byte lampa_direction = 0; //prepared for going up.
unsigned long last_check = 0; // stores the last tiem we check the server_status.
bool lampa_off;
bool isconnected = false;

void setup_wifi() {
  // We start by connecting to a WiFi network
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println("");
  //Serial.print("WiFi connected - ESP IP address: ");
  //Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    if (client.connect(ID)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      client.subscribe("lampa/check"); //every 30 second server send here "-" and lampa must update it to 0 or 1. If not, server knows this is offline.
      client.subscribe("lampa/cmd"); //0-off, 1-on, b0 to b255 change brightess, h0 - h50 change hight in cm.
      client.subscribe("lampa/val"); //check info for current hight / brightess.
      //if lampa val = "b" -> lampa sends curent brightness. if "m" lampa sends current hight.
      
      //sending a check message to the network for "lampa is online".
      if (!lampa_off){
        char pubuff [4];
        sprintf(pubuff, "%i", lampa_pwm); // %d decimal; %i integer; %s string; %u unsigned decimal integer
        client.publish("lampa/check", pubuff);
      }else client.publish("lampa/check", "0");
      isconnected = true;
      last_check = millis();
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      isconnected = false;
      // Wait 5 seconds before retrying
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
  if (strcmp(topic, "lampa/check")==0 && message == "-"){
    //Serial.print("received message -. Sending: "); 
    if (!lampa_off){
      sprintf(pubuff, "%i", lampa_pwm); // %d decimal; %i integer; %s string; %u unsigned decimal integer
      //Serial.println(pubuff);
      client.publish("lampa/check", pubuff);
    }else client.publish("lampa/check", "0");
    isconnected = true;
    last_check = millis(); //i count this, if nobody send ping signal, I turn it off.

  }else if (strcmp(topic, "lampa/val")==0){
    if (message == "b"){ //the lamp is asked for its brightness.
      sprintf(pubuff, "%i", lampa_pwm); // %d decimal; %i integer; %s string; %u unsigned decimal integer
      client.publish("lampa/val", pubuff);
    }else if(message == "h"){ //the lampa is asked for its hight...
      sprintf(pubuff, "%i", lampa_high); // %d decimal; %i integer; %s string; %u unsigned decimal integer
      client.publish("lampa/val", pubuff);
    }
    isconnected = true;
    last_check = millis();
    
  }else if (strcmp(topic, "lampa/cmd")==0){
    if (message == "1"){
      lampaOn(lampa_pwm);
    }else if (message == "0"){
      lampaOff();
    }else if (message.charAt(0) =='b'){
      String bval = message.substring(1); //if "b200", it removes the b from b200 and takes only 200, to set it for brightness
      int pwm = bval.toInt();
      if (pwm > 20){
        if(pwm <=255){
          lampa_pwm = pwm;
          lampaOn(lampa_pwm);
        }
      }else{
        lampaOff();
      }
    }else if (message.charAt(0) =='h'){
      String hval = message.substring(1);
      int high = hval.toInt();
      if (high > 10 && high <= 60){
        lampamove(high);
      }
    }
    isconnected = true;
    last_check = millis();
  }
}

void lampamove(int move_to){
  if (!lampa_off){ //move the lampa only if lama is ON
    //calculate the direction and the lenght:
    //new - old returns a value with "-" if I want to go up and "+" if go down.
    int move_with = move_to - lampa_high;
    if (move_with < 0){
      //move up:
      unsigned long steps = -1 * move_with * 1600;
      int movit = digitalRead(STOPER);
      digitalWrite(ENPIN, LOW); //turn the driver ON
      digitalWrite(DIRPIN, LOW);
      for (unsigned long i = 0; i <= steps; i++){
        digitalWrite(STEPPIN, HIGH);
        delayMicroseconds(150);
        digitalWrite(STEPPIN, LOW);
        delayMicroseconds(150);
        movit = digitalRead(STOPER);
        if (movit == 0) break; //stop if the stopper is pressed.
      }
      digitalWrite(ENPIN, HIGH); //turn the driver OFF
    }else if (move_with > 0 && move_to <= 60){
      //move down:
      unsigned long steps = move_with * 1600;
      digitalWrite(ENPIN, LOW); //turn the driver ON
      digitalWrite(DIRPIN, HIGH);
      for (unsigned long i = 0; i <= steps; i++){
        digitalWrite(STEPPIN, HIGH);
        delayMicroseconds(150);
        digitalWrite(STEPPIN, LOW);
        delayMicroseconds(150);
      }
      digitalWrite(ENPIN, HIGH); //turn the driver OFF
    }
  }
}

void lampaOff(){
  if (!lampa_off){
    Serial.println("lampa is going OFF");
    ledcWrite(0, 50);
    int movit = digitalRead(STOPER);
    digitalWrite(ENPIN, LOW); //turn the driver ON
    digitalWrite(DIRPIN, LOW);
    while (movit == 1){
      digitalWrite(STEPPIN, HIGH);
      delayMicroseconds(150);
      digitalWrite(STEPPIN, LOW);
      delayMicroseconds(150);
      movit = digitalRead(STOPER);
    }
    digitalWrite(ENPIN, HIGH); //turn the driver OFF
    ledcWrite(0, 0);
    lampa_off = true;
    client.publish("lampa/check", "0");
    //lampa_pwm = 0;
  }
}

void lampaOn(byte pwm){
  //int movit = digitalRead(STOPER);
  if (lampa_off){ //this means stopper is pressed and the lamp is on init position.
    digitalWrite(ENPIN, LOW); //turn the driver ON
    digitalWrite(DIRPIN, HIGH);
    //Serial.print("lampa is going ON: ");Serial.println(pwm);
    ledcWrite(0, 50);
    for (unsigned long i = 0; i <= 60000; i++){
      //total high for 80 000 steps is 50cm
      //1600 per centimeter.
      digitalWrite(STEPPIN, HIGH);
      delayMicroseconds(150);
      digitalWrite(STEPPIN, LOW);
      delayMicroseconds(150);
      
    }
    digitalWrite(ENPIN, HIGH); //turn the driver OFF
    ledcWrite(0, 0); delay(20); ledcWrite(0, 50); delay(50); ledcWrite(0, 0); 
    delay(50);
    ledcWrite(0, pwm);
    lampa_off = false;
  }else{
    //if not on the init position, lamp will only change its brightness.
    ledcWrite(0, pwm);
  }
  char pubuff [4];sprintf(pubuff, "%i", lampa_pwm);
  client.publish("lampa/check", pubuff);
  last_check = millis(); //null the counter for going off if server disapire.  
}

// ----------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  pinMode(STOPER, INPUT_PULLUP);
  pinMode(OTG_PIN, INPUT_PULLUP);
 
  ledcAttachPin(RELAY, 0);
  ledcSetup(0, 5000, 8); //pwm channel 0. 5khz. 8 bit resolution (0-255);
  ledcWrite(0, 0);
  //to activate, write ledcWrite(PWM_Ch, DutyCycle);
  
  pinMode(ENPIN, OUTPUT);
  pinMode(DIRPIN, OUTPUT);
  pinMode(STEPPIN, OUTPUT);
  digitalWrite(ENPIN, HIGH); //turn the driver OFF

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(ENPIN, LOW); //turn the driver ON
  digitalWrite(DIRPIN, LOW);
  int movit = digitalRead(STOPER);
  while (movit == 1){
    digitalWrite(STEPPIN, HIGH);
    delayMicroseconds(150);
    digitalWrite(STEPPIN, LOW);
    delayMicroseconds(150);
    movit = digitalRead(STOPER);
  }
  Serial.println("lampa is now off.");
  lampa_off = true;
  //digitalWrite(DIRPIN, HIGH);
  digitalWrite(ENPIN, HIGH); //turn the driver OFF 

  int emerg = digitalRead(OTG_PIN);
  if (emerg == 0){
    lampaOn(20);
  }
  Serial.println("Program started.");
}

void loop() {
  if (!client.connected())  // Reconnect if connection is lost
    reconnect();    
  client.loop();

  if (!lampa_off){
    unsigned long cmillis = millis();
    if (cmillis - last_check >= 60000){
      Serial.println("lampa is going off because not connected.");
      lampaOff();
      client.publish("lampa/check", "0");
      last_check = cmillis;
    }
  }
}
