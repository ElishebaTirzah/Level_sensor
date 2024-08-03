#include <WiFi.h>
#include <HTTPClient.h>
#include <Firebase_ESP_Client.h>
#include "kalman_filter.h"
#include "poweroptimisation.h"
#include "averaging.h"

const char* ssid = "...";
const char* password = "...";
#define API_KEY "AIzaSyAFihlJ81Poc4vCX01NCYxvKONB-XcrCxA"
#define DATABASE_URL "https://level-sensor-1663d-default-rtdb.asia-southeast1.firebasedatabase.app/"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

const int trigPin = 5;
const int echoPin = 18;
#define VBAT_PIN 35

unsigned long sendDataPrevMillis = 0;
int distance;

KalmanFilter kfDistance(1, 1, 0.01, 0);
KalmanFilter kfBattery(1, 1, 0.01, 0);

float readBatteryVoltage() {
  int rawValue = analogRead(VBAT_PIN);
  float batteryVoltage = (float(rawValue) / 4095.0) * 3.3;
  float actualVoltage = (batteryVoltage * (27000 + 108000)) / 108000;
  return actualVoltage;
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(VBAT_PIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendToFirebase(const String& path, int value) {
  HTTPClient http;
  String url = String(DATABASE_URL) + path + ".json?auth=" + API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String postData = "{\"value\": " + String(value) + "}";
  int httpResponseCode = http.PUT(postData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response from " + path + ": " + response);
  } else {
    Serial.println("Error sending data to " + path);
  }

  http.end();
}

void sendStatusToFirebase(const String& path, float batteryPercentage) {
  HTTPClient http;
  String url = String(DATABASE_URL) + path + ".json?auth=" + API_KEY;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String postData = "{\"on_status\": \"" + String(batteryPercentage, 2) + "%\"}";
  int httpResponseCode = http.PUT(postData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response from " + path + ": " + response);
  } else {
    Serial.println("Error sending status to " + path);
  }

  http.end();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    long duration;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    float distance = duration * 0.034 / 2;
    distance = kfDistance.update(distance);  // Apply Kalman filter

    float battv = readBatteryVoltage();
    float batteryPercentage = (battv - 3.1) / (4.1 - 3.1) * 100;
    batteryPercentage = constrain(batteryPercentage, 0, 100);
    batteryPercentage = kfBattery.update(batteryPercentage);  // Apply Kalman filter

    sendToFirebase("/sensor_1/readings", distance);
    sendStatusToFirebase("/sensor_1/on_status", batteryPercentage);

    Serial.print("Ultrasonic Reading (Sensor 1): ");
    Serial.print(distance);
    Serial.println(" cm");

    Serial.print("Battery Percentage (Sensor 1): ");
    Serial.print(batteryPercentage);
    Serial.println(" %");

    delay(10000);
  }
}
