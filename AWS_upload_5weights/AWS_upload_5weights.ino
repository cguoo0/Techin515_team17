#include <Arduino.h>
#include <HX711.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Secrets.h" // Ensure this file contains your WiFi and AWS credentials
#include <time.h>     // Include the time library

// HX711 circuit pin configuration
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 23;

// Pet weight scale pins
const int PET_A_WEIGHT_DOUT_PIN = 26;
const int PET_A_WEIGHT_SCK_PIN = 27;
const int PET_B_WEIGHT_DOUT_PIN = 14;
const int PET_B_WEIGHT_SCK_PIN = 12;

// Food bowl weight scale pins
const int BOWL_A_WEIGHT_DOUT_PIN = 32;
const int BOWL_A_WEIGHT_SCK_PIN = 33;
const int BOWL_B_WEIGHT_DOUT_PIN = 4;
const int BOWL_B_WEIGHT_SCK_PIN = 0;

// Servo pins
const int SERVO_FEED_PIN = 18;    // Servo for feeding
const int SERVO_ANGLE_PIN = 17;   // Servo for adjusting angle
const int SERVO_BOWL_PIN = 16;    // Servo for selecting pet bowl

// Create HX711 objects
HX711 scale;
HX711 petAScale;
HX711 petBScale;
HX711 bowlAScale;
HX711 bowlBScale;

// Calibration factor
#define CALIBRATION_FACTOR 426

// PWM channel configuration
const int PWM_FREQ = 50;          // PWM frequency of 50Hz
const int PWM_RESOLUTION = 16;    // PWM resolution of 16 bits

// User-set target weight
int targetWeight = 0;

// Continuous servo PWM values
const int CONTINUOUS_SERVO_STOP = 4751;  // PWM value for stopping continuous servo, adjust according to your servo model
const int CONTINUOUS_SERVO_RUN = 7864;   // PWM value for running continuous servo, adjust according to your servo model

// Pet users
String petUsers[] = {"Pet A", "Pet B"};
int currentPetIndex = 0;

// Number of readings to average
const int NUM_READINGS = 10;

// AWS IoT configuration
WiFiClientSecure net;
PubSubClient client(net);

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED) {
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
 
  if (!client.connected()) {
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

void publishMessage(String sensor, double weight) {
  char timestamp[64];
  time_t now = time(nullptr);
  strftime(timestamp, sizeof(timestamp), "%m/%d/%Y, %H:%M", localtime(&now));

  char msgBuffer[256]; // Increased buffer size for JSON
  snprintf(msgBuffer, sizeof(msgBuffer), "{\"Sensor\":\"%s\", \"Weight\":\"%.2f g\", \"Timestamp\":\"%s\"}", sensor.c_str(), weight, timestamp);

  client.publish(AWS_IOT_PUBLISH_TOPIC, msgBuffer);
  Serial.println(msgBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);

  // AWS IoT Setup
  connectAWS();
  configTime();  // Initialize NTP

  // HX711 Setup
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();

  petAScale.begin(PET_A_WEIGHT_DOUT_PIN, PET_A_WEIGHT_SCK_PIN);
  petAScale.set_scale(CALIBRATION_FACTOR);
  petAScale.tare();

  petBScale.begin(PET_B_WEIGHT_DOUT_PIN, PET_B_WEIGHT_SCK_PIN);
  petBScale.set_scale(CALIBRATION_FACTOR);
  petBScale.tare();

  bowlAScale.begin(BOWL_A_WEIGHT_DOUT_PIN, BOWL_A_WEIGHT_SCK_PIN);
  bowlAScale.set_scale(CALIBRATION_FACTOR);
  bowlAScale.tare();

  bowlBScale.begin(BOWL_B_WEIGHT_DOUT_PIN, BOWL_B_WEIGHT_SCK_PIN);
  bowlBScale.set_scale(CALIBRATION_FACTOR);
  bowlBScale.tare();

  // Set up PWM channels and pins
  ledcSetup(0, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(SERVO_FEED_PIN, 0);
  ledcAttachPin(SERVO_ANGLE_PIN, 1);
  ledcAttachPin(SERVO_BOWL_PIN, 2);

  showMenu();
}

void loop() {
  if (Serial.available() > 0) {
    int userInput = Serial.parseInt();

    switch (userInput) {
      case 1:
        monitorBowlWeight();
        break;
      case 2:
        monitorPetWeight();
        break;
      case 3:
        feedPet();
        break;
      default:
        Serial.println("Invalid input, please choose again.");
        showMenu();
        break;
    }
  }

  // Publish weight data to AWS IoT every minute
  static unsigned long lastPublishTime = 0;
  if (millis() - lastPublishTime >= 60000) {
    publishAllWeights();
    lastPublishTime = millis();
  }

  client.loop();
}

void showMenu() {
  Serial.println("Please select an action:");
  Serial.println("1. View food bowl weight");
  Serial.println("2. View pet weight");
  Serial.println("3. Feed pet");
}

void monitorBowlWeight() {
  Serial.println("Real-time food bowl weight:");
  while (true) {
    double bowlAWeight = averageReading(bowlAScale);
    double bowlBWeight = averageReading(bowlBScale);
    Serial.print("Pet A food bowl weight: ");
    Serial.print(bowlAWeight);
    Serial.print("g, Pet B food bowl weight: ");
    Serial.print(bowlBWeight);
    Serial.println("g");

    if (Serial.available() > 0) {
      int userInput = Serial.parseInt();
      if (userInput == 0) {
        Serial.println("Returning to main menu.");
        showMenu();
        return;
      } else {
        Serial.println("Invalid input, please enter 0 to return to main menu.");
      }
    }
    delay(500);
  }
}

void monitorPetWeight() {
  Serial.println("Real-time pet weight:");
  while (true) {
    double petAWeight = averageReading(petAScale);
    double petBWeight = averageReading(petBScale);
    Serial.print("Pet A weight: ");
    Serial.print(petAWeight);
    Serial.print("g, Pet B weight: ");
    Serial.print(petBWeight);
    Serial.println("g");

    if (Serial.available() > 0) {
      int userInput = Serial.parseInt();
      if (userInput == 0) {
        Serial.println("Returning to main menu.");
        showMenu();
        return;
      } else {
        Serial.println("Invalid input, please enter 0 to return to main menu.");
      }
    }
    delay(500);
  }
}

void feedPet() {
  selectPetUser();
  moveBowlServo();                  // Select pet bowl before starting feeding
  while (targetWeight > 0) {
    double currentWeight = averageReading(scale);
    Serial.print("Current weight: ");
    Serial.print(currentWeight);
    Serial.println("g");

    if (currentWeight < targetWeight) {
      ledcWrite(0, CONTINUOUS_SERVO_RUN);   // Continue rotating feeding servo
    } else {
      ledcWrite(0, CONTINUOUS_SERVO_STOP);  // Stop continuous servo
      Serial.println("Target weight reached, continuous servo stopped.");
      moveAngleServo();                  // Adjust angle servo
      targetWeight = 0;                  // Reset target weight
      unsigned long startTime = millis();
      while (true) {
        if (currentPetIndex == 0) {
          double bowlAWeight = averageReading(bowlAScale);
          Serial.print("Pet A food bowl weight: ");
          Serial.print(bowlAWeight);
          Serial.println("g");
          if (bowlAWeight <= 0) {
            unsigned long endTime = millis();
            unsigned long duration = endTime - startTime;
            Serial.print("Pet A feeding time: ");
            Serial.print(duration / 1000);
            Serial.println("seconds");
            break;
          }
        } else {
          double bowlBWeight = averageReading(bowlBScale);
          Serial.print("Pet B food bowl weight: ");
          Serial.print(bowlBWeight);
          Serial.println("g");
          if (bowlBWeight <= 0) {
            unsigned long endTime = millis();
            unsigned long duration = endTime - startTime;
            Serial.print("Pet B feeding time: ");
            Serial.print(duration / 1000);
            Serial.println("seconds");
            break;
          }
        }
        delay(500);
      }
      Serial.println("Feeding completed.");
      showMenu();
    }
    delay(500);   // Slight delay to reduce reading frequency
  }
}

void selectPetUser() {
  Serial.println("Please select a pet user:");
  for (int i = 0; i < sizeof(petUsers) / sizeof(petUsers[0]); i++) {
    Serial.print(i + 1);
    Serial.print(". ");
    Serial.println(petUsers[i]);
  }

  while (Serial.available() == 0) {
    // Wait for user input
  }

  int userInput = Serial.parseInt();

  if (userInput > 0 && userInput <= sizeof(petUsers) / sizeof(petUsers[0])) {
    currentPetIndex = userInput - 1;
    Serial.print("Pet user selected: ");
    Serial.println(petUsers[currentPetIndex]);
    getUserInput();
  } else if (userInput > sizeof(petUsers) / sizeof(petUsers[0])) {
    Serial.println("Sorry, currently only up to two pet users are supported.");
    selectPetUser();
  } else {
    Serial.println("Invalid input, please enter the pet user number.");
    selectPetUser();
  }
}

void getUserInput() {
  Serial.println("Please enter the target weight (grams) and press Enter:");
  while (Serial.available() == 0) {
    // Wait for user input
  }
  int userInput = Serial.parseInt();   // Read the input target weight
  if (userInput > 0) {
    targetWeight = userInput;
    Serial.print("Target weight set to: ");
    Serial.print(targetWeight);
    Serial.println("g");
  } else {
    Serial.println("Invalid input, please enter a positive integer.");
    getUserInput();  // Recursive call until valid input is received
  }
}

void moveAngleServo() {
  Serial.println("Angle servo starting...");
  slowServoMove(1, 60, 7);              // Rotate angle servo to 60 degrees quickly
  delay(2000);                          // Hold for 2 seconds
  slowServoMove(1, 0, 7);               // Angle servo returns to 0 degrees quickly
  Serial.println("Angle servo completed.");
  delay(2000);                          // Wait for 2 seconds before resetting the bowl servo
  if (currentPetIndex == 0) {
    slowServoMove(2, 0, 3);    // Return to initial position very quickly if Pet A was selected
    Serial.println("Pet bowl selection reset.");
  }
}

void moveBowlServo() {
  Serial.println("Selecting pet bowl...");
  if (currentPetIndex == 0) {
    slowServoMove(2, 45, 3);   // Rotate to 45 degrees (clockwise) to select Pet A's bowl very quickly
    delay(5000);               // Hold for 5 seconds to allow food to fall into the bowl
  }
  Serial.println("Pet bowl selection completed.");
}

void slowServoMove(int channel, int targetDegree, int delayTime) {
  int currentPWM = ledcRead(channel);
  int targetPWM = calculatePWM(targetDegree);
  int step = (currentPWM < targetPWM) ? 1 : -1;

  while (currentPWM != targetPWM) {
    currentPWM += step;
    ledcWrite(channel, currentPWM);
    delay(delayTime);  // Adjust delay for speed control
  }
}

int calculatePWM(int degree) {
  // Convert angle to PWM signal
  return map(degree, 0, 180, 1638, 7864);   // Adjust these values according to your servo specifications
}

double averageReading(HX711& scale) {
  double sum = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    sum += scale.get_units();
    delay(10);  // Delay to allow time for stable readings
  }
  return sum / NUM_READINGS;
}

void publishAllWeights() {
  double loadcellWeight = averageReading(scale);
  double petAWeight = averageReading(petAScale);
  double petBWeight = averageReading(petBScale);
  double bowlAWeight = averageReading(bowlAScale);
  double bowlBWeight = averageReading(bowlBScale);

  publishMessage("Loadcell", loadcellWeight);
  publishMessage("Pet A", petAWeight);
  publishMessage("Pet B", petBWeight);
  publishMessage("Bowl A", bowlAWeight);
  publishMessage("Bowl B", bowlBWeight);
}
