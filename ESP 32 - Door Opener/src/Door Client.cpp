#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>


int BOARD_LED = 8;
int SOLENOID_GPIO = 3;
clock_t time_since_last_request = clock();

// WIFI
const char* ssid = "NETWORK NAME";
const char* password = "NETWORK PASSWORD";

// MQTT Broker
const char *mqtt_broker = "XXX.XXX.XXX.XXX";
const char *mqtt_username = "USERNAME";
const char *mqtt_password = "PASSWORD";
const int mqtt_port = XXX;

// Topics
const char *open_door_topic = "esp32/Door_Open";
const char *door_ack_topic = "esp32/Door_ACK";

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

WiFiClient espClient;
PubSubClient client(espClient);

// Function that gets current epoch time as a string
char* getTimeAsString() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return NULL;
  }
  time(&now);
  return ctime(&now);
}

time_t getTime(){
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return NULL;
  }
  return time(&now);
}


void scan_wifi() {
  Serial.println("Scanning Nearby Networks");

  int n = WiFi.scanNetworks();
  if (n == 0) {
      Serial.println("No networks found");
  } else {
      Serial.print(n);
      Serial.println(" Networks found");
      for (int i = 0; i < n; ++i) {
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          delay(10);
      }
  }
  Serial.println("");

  delay(5000);
}

void connect_to_wifi() {
  Serial.println("Connecting to Wifi");

  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int time = 0;
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    time+=500;
    if(time > 10000){
      Serial.println("Failed to connect");
      exit(-1);
    }
  }

  Serial.println("\nConnected to WiFi network");
  configTime(0, 0, ntpServer);
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");

    //If topic is open door topic
    if(strcmp( topic, open_door_topic ) == 0){
        double time_taken =  ((double)(clock() - time_since_last_request))/CLOCKS_PER_SEC;
        
        //Ensure door cannot be spammed open using a 20 second timer between requests
        if(time_taken > 20){
            time_since_last_request = clock();
            
            //Activate solenoid to press button
            analogWrite(SOLENOID_GPIO, HIGH);
            delay(2000);
            digitalWrite(SOLENOID_GPIO, LOW);

            //Send ack back that the door was opened
            client.publish(door_ack_topic, getTimeAsString());
            Serial.println("Message sent to MQTT server");
        } else{
            Serial.println("Cannot send message, need to wait 1 minute before message is sent");
        }
    }
}

void connect_to_mqtt(){
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("Connecting Client %s to MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        Serial.println("Connected to MQTT broker");
    } else {
        Serial.println("Failed to connect to MQTT broker with state: ");
        Serial.println(client.state());
        delay(2000);
    }
  }

  client.subscribe(open_door_topic);
}

void write_to_board_led() {
  digitalWrite(BOARD_LED,HIGH);
  delay(500);
  //Serial.println("loop: Led set HIGH");

  digitalWrite(BOARD_LED,LOW);
  delay(500);
  //Serial.println("loop: Led set LOW");
}

void setup() {
  Serial.begin(115200);
  delay(5000);

  pinMode(BOARD_LED,OUTPUT);
  pinMode(SOLENOID_GPIO, OUTPUT);

  write_to_board_led();

  scan_wifi();
  connect_to_wifi();
  connect_to_mqtt();
}

void loop() {
  //write_to_board_led();
  delay(200);
  client.loop();
}
