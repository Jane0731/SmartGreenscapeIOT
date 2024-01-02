#include <SimpleDHT.h>
#include <BluetoothSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Time.h>

BluetoothSerial BT;  //宣告藍芽物件，名稱為BT

int pinDHT11 = 15;
int soil_sensor = 36;
SimpleDHT11 dht11;
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int value = 0;
const char *ssid = "HITRON-55F0";       //ssid:網路名稱
const char *password = "M6D8A9RC79SM";  //pasword：網路密碼
const char *mqtt_server = "192.168.213.11";
const char *MQTTUser = "jane";
const char *MQTTPassword = "0731";
const char *MQTTPubTopic1 = "plant/DHT11";
byte mac[6];
char macStr[18] = { 0 };
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");

  WiFi.macAddress(mac);

  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("mac address: ");
  Serial.println(macStr);
}
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), MQTTUser, MQTTPassword)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  BT.begin("Smart_Plant");
  setup_wifi();
  pinMode(soil_sensor, INPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  byte temperature = 0;
  byte humidity = 0;

  int err = SimpleDHTErrSuccess;
  // start working...
  Serial.println("=================================");
  if ((err = dht11.read(pinDHT11, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err=");
    Serial.println(err);
    delay(1000);
    return;
  }
  int val = analogRead(soil_sensor);
  Serial.print("temp value:");
  Serial.println((int)temperature);
  Serial.print("humid value:");
  Serial.println((int)humidity);
  Serial.print("soil value:");
  Serial.println(val);

  DynamicJsonDocument doc(1024);
  doc["temp"] = (int)temperature;
  doc["humid"] = (int)humidity;
  doc["soli"] = val;

  doc["mac"] = macStr;

  String payload;
  serializeJson(doc, payload);

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    client.publish(MQTTPubTopic1, payload.c_str());
  }
  delay(10000);  //每10秒顯示一次
}