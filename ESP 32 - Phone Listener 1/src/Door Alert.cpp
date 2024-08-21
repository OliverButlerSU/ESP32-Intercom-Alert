#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>


int BOARD_LED = 8;
int PHONE_GPIO = 2;
clock_t time_since_last_request = clock();

// WIFI
const char* ssid = "Username";
const char* password = "Password";

// MQTT Broker
const char *mqtt_broker = "XXX.XXX.XXX.XXX";
const char *mqtt_username = "Username";
const char *mqtt_password = "Password";
const int mqtt_port = XXX;

// Topics
const char *door_opened_topic = "esp32/Door_Alert";
// const char *open_door_request = "esp32/Open_Door";

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

  client.subscribe(door_opened_topic);
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
  pinMode(PHONE_GPIO, INPUT);

  write_to_board_led();

  scan_wifi();
  connect_to_wifi();
  connect_to_mqtt();
}

void loop() {
  //write_to_board_led();
  delay(200);
  
  int phone_state = analogRead(PHONE_GPIO);
  Serial.println(phone_state);
  
  //If the phone is on/plays a loud alarm sound
  if(phone_state < 2000){
    //Get the time between the last sent MQTT message
    double time_taken =  ((double)(clock() - time_since_last_request))/CLOCKS_PER_SEC;
    //If the time > 60 seconds
    if(time_taken > 60){
      //Publish a new message to esp32/Door_Alert
      time_since_last_request = clock();
      client.publish(door_opened_topic, getTimeAsString());
      Serial.println("Message sent to MQTT server");
    } else{
      Serial.println("Cannot send message, need to wait 1 minute before message is sent");
    }
  }
    
  
  client.loop();
}
