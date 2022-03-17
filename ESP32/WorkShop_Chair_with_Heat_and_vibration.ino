#include <WiFi.h>
#include <PubSubClient.h>

#define HTRH 27 //heater Hight
#define HTRL 25 //heater Low
#define VIBR 16 //Masageur

const char *ssid =  "MyNetwork";   // name of your WiFi network
const char *password =  "verystrongpassword";
const char* mqtt_server = "192.168.0.173";
const char *ID = "stol";  // Name of our device, must be unique

WiFiClient espClient;
PubSubClient client(espClient);

bool heat_off = true;
bool vibr_off = true;

unsigned long last_check = 0; // stores the last tiem we check the server_status.

void setup_wifi() {
  Serial.println("connecting to WIFI...");
  WiFi.begin(ssid, password);
  delay(200);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: "); Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(ID)) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      client.subscribe("stol/check"); //every 30 second server send here "-" and lampa must update it to 0 or 1. If not, server knows this is offline.
      client.subscribe("stol/cmd"); //0-off all.Usually used when a "workshop lock-in" command is executed.
      client.subscribe("stol/heat"); //heater control. 0 off. 1 low, 2 high.
      client.subscribe("stol/vibr"); //masage control. 0 off. 1 on.
      
      //sending a check message to the network for "stol is online".
      if (heat_off && vibr_off) 
        client.publish("stol/check", "0");
      else 
        client.publish("stol/check", "1");
      
      last_check = millis();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
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
  if (strcmp(topic, "stol/check")==0 && message == "-"){
    if (heat_off && vibr_off){
      client.publish("stol/check", "0");
      client.publish("stol/heat_state", "0");
      client.publish("stol/vibr_state", "0");
    }else{
      client.publish("stol/check", "1");
      if (heat_off)
        client.publish("stol/heat_state", "0");
      else
        client.publish("stol/heat_state", "1");
      if (vibr_off)
        client.publish("stol/vibr_state", "0");
      else
        client.publish("stol/vibr_state", "1");
    }
    last_check = millis(); //i count this, if nobody send ping signal, I turn it off.

  }else if (strcmp(topic, "stol/cmd")==0){
    if (message == "0")
      stolOff();
    last_check = millis();
  
  }else if (strcmp(topic, "stol/heat")==0){
    if (message == "0"){
      digitalWrite(HTRH, LOW);
      heat_off = true;
      Serial.println("ALL heaters OFF.");
    }else if (message == "1"){
      digitalWrite(HTRH, HIGH);
      heat_off = false;
      Serial.println("Turning heater ON");
    }
    if (heat_off && vibr_off)
      client.publish("stol/check", "0");
    else
      client.publish("stol/check", "1");
    last_check = millis();
    
  }else if (strcmp(topic, "stol/vibr")==0){
    if (message == "0"){
      digitalWrite(VIBR, LOW);
      vibr_off = true;
      Serial.println("Turning the massager OFF");
    }else if (message == "1"){
      digitalWrite(VIBR, HIGH);
      vibr_off = false;
      Serial.println("Turning the massager ON");
    }
    if (heat_off && vibr_off)
      client.publish("stol/check", "0");
    else
      client.publish("stol/check", "1");
    last_check = millis();
  }
}

void stolOff(){
  digitalWrite(HTRH, LOW);
  digitalWrite(HTRL, LOW);
  digitalWrite(VIBR, LOW);
  heat_off = true;
  vibr_off = true;
  Serial.println("Stol is now Off.");
  client.publish("stol/check", "0");
}
/*
void heatOn(byte cmd){
  if(cmd == 0){
    digitalWrite(HTRH, LOW);
    digitalWrite(HTRL, LOW);
  }else if (cmd == 1){
    digitalWrite(HTRH, LOW);
    delay(200);
    digitalWrite(HTRL, HIGH);
  }else if (cmd == 2){
    digitalWrite(HTRL, LOW);
    delay(200);
    digitalWrite(HTRH, HIGH);
  }
}*/

void setup() {
  Serial.begin(115200);
  while (!Serial);
  
  pinMode(HTRH, OUTPUT);
  pinMode(HTRL, OUTPUT);
  pinMode(VIBR, OUTPUT);
  digitalWrite(HTRH, LOW);
  digitalWrite(HTRL, LOW);
  digitalWrite(VIBR, LOW);
  delay(1000);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("stol is Ready.");
  heat_off = true;
  vibr_off = true;
}

void loop() {
  if (!client.connected())  // Reconnect if connection is lost
    reconnect();    
  client.loop();

  if(!heat_off && !vibr_off){
    unsigned long cmillis = millis();
    if (cmillis - last_check >= 60000){
      Serial.println("Stol is going off because not connected");
      stolOff();
      last_check = cmillis;
    }
  }
}
