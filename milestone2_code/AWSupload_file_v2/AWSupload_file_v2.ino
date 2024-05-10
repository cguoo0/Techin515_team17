#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <HX711_ADC.h>
#include "Secrets.h" // Ensure this file contains your WiFi and AWS credentials
#include <time.h>     // Include the time library

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"  //the resource-id of your AWS IoT policy document
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"  //the resource-id of your AWS IoT policy document

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// HX711 Pins
const int HX711_dout = 32; // ESP32 Pin connected to HX711 dout
const int HX711_sck = 33;  // ESP32 Pin connected to HX711 sck
HX711_ADC LoadCell(HX711_dout, HX711_sck);

unsigned long t = 0;  // Time variable for loop checks

void connectAWS(){   
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME)){
      Serial.print(".");
      delay(100);
}
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");

}
 
void configTime() {
  configTime(-7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
}

void publishMessage(float weight)
{
  char timestamp[64];
  time_t now = time(nullptr);
  strftime(timestamp, sizeof(timestamp), "%m/%d/%Y, %H:%M", localtime(&now));

  char msgBuffer[256]; // Increased buffer size for JSON
  snprintf(msgBuffer, sizeof(msgBuffer), "{\"Sensor\":\"cat1\", \"Weight\":\"%.2f g\", \"Timestamp\":\"%s\"}", weight, timestamp);

  client.publish(AWS_IOT_PUBLISH_TOPIC, msgBuffer);
  Serial.println(msgBuffer);
}



void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);
  connectAWS();
  configTime();  // Initialize NTP
  LoadCell.begin();
  LoadCell.start(2000, true);
  while (!LoadCell.update());
  LoadCell.setCalFactor(-1061.35);
  Serial.println("Setup complete");
}

void loop() {
  static boolean newDataReady = false;

  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    float weight = LoadCell.getData();
    publishMessage(weight);
    newDataReady = false;
  }

  client.loop();
  delay(10000);  // Delay for 1 minute
}
