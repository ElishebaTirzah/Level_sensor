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

const int trigPin1 = 5;
const int echoPin1 = 18;
const int trigPin2 = 27;
const int echoPin2 = 26;
#define VBAT_PIN 34  

KalmanFilter kfDistance1(1, 1, 0.01, 0);
KalmanFilter kfDistance2(1, 1, 0.01, 0);
KalmanFilter kfBattery(1, 1, 0.01, 0);

float readBatteryVoltage() {
    int adc_value = analogRead(VBAT_PIN);
    float adc_voltage = (adc_value * 3.3) / 4095.0;
    float in_voltage = adc_voltage / (7500.0 / (30000.0 + 7500.0));
    return in_voltage;
}

void setup() {
    Serial.begin(115200);
    pinMode(trigPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
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
    unsigned long startMillis = millis();
    const unsigned long measurementDuration = 7 * 60 * 1000; // 7 minutes
    const unsigned long averagingInterval = 10 * 1000; // 10 seconds

    while (millis() - startMillis < measurementDuration) {
        unsigned long currentMillis = millis();
        static unsigned long prevMillis = 0;

        if (currentMillis - prevMillis >= averagingInterval) {
            prevMillis = currentMillis;

            float sumDistance1 = 0;
            float sumDistance2 = 0;
            int count = 0;

            for (int i = 0; i < 10; i++) {
                // Sensor 1 
                digitalWrite(trigPin1, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPin1, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPin1, LOW);
                int duration1 = pulseIn(echoPin1, HIGH);
                float distance1 = duration1 * 0.034 / 2;
                distance1 = kfDistance1.update(distance1);
                sumDistance1 += distance1;

                // Sensor 2 
                digitalWrite(trigPin2, LOW);
                delayMicroseconds(2);
                digitalWrite(trigPin2, HIGH);
                delayMicroseconds(10);
                digitalWrite(trigPin2, LOW);
                int duration2 = pulseIn(echoPin2, HIGH);
                float distance2 = duration2 * 0.034 / 2;
                distance2 = kfDistance2.update(distance2);
                sumDistance2 += distance2;

                count++;
                delay(1000); // 1 second delay between readings
            }

            float avgDistance1 = sumDistance1 / count;
            float avgDistance2 = sumDistance2 / count;

            float distances[] = {avgDistance1, avgDistance2};
            float avgDistance = averageReadings(distances, 2);

            int waterLevel = 20 - avgDistance; // Assuming container height is 20 cm
            if (waterLevel < 0) {
                waterLevel = 0;
            }

            float battv = readBatteryVoltage();
            float batteryPercentage = (battv - 3.1) / (4.1 - 3.1) * 100;
            batteryPercentage = constrain(batteryPercentage, 0, 100);
            batteryPercentage = kfBattery.update(batteryPercentage);

            sendToFirebase("/sensor_2/readings", waterLevel);
            sendStatusToFirebase("/sensor_2/on_status", batteryPercentage);

            Serial.print("Water Level: ");
            Serial.print(waterLevel);
            Serial.println(" cm");

            Serial.print("Battery Percentage: ");
            Serial.print(batteryPercentage);
            Serial.println(" %");
        }
    }

    configureDeepSleep();
    enterDeepSleep();
}
