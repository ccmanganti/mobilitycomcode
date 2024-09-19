#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "MobilityCom";
const char* pass = "password";

// Flex sensor pins
const int flexThumb = 35;
const int flexIndex = 34;
const int flexMiddle = 32;
const int flexRing = 33;
const int flexPinky = 39;

const int actionRegister = 22; // Button pin

// ESP32 serial number (for demo purposes, replace with actual serial number retrieval method)
String serialNumber = "123456789ABC";

// Status flags
bool isWiFiConnected = false;
bool isSerialNumberValid = false;

const int ledPin = 2;

// Timing variables for non-blocking LED blinking
unsigned long lastBlinkTime = 0;
bool ledState = LOW;

// Timing intervals for different states
unsigned long blinkOnTime = 100;  // Default on time
unsigned long blinkOffTime = 100; // Default off time

// Non-blocking WiFi connection
unsigned long lastWiFiCheckTime = 0;
unsigned long wifiCheckInterval = 5000;  // Time between WiFi connection attempts (5 seconds)
bool wifiConnecting = false;

void setup() {
  Serial.begin(9600);

  // Initialize button pin
  pinMode(actionRegister, INPUT_PULLUP);

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Turn off LED initially

  // Start WiFi connection attempt
  WiFi.begin(ssid, pass);
  wifiConnecting = true;  // Indicate we are in WiFi connection process
}

void loop() {
  // Run the non-blocking LED blink function based on current state
  blinkLed(blinkOnTime, blinkOffTime);

  // Handle WiFi connection in a non-blocking way
  handleWiFiConnection();

  if (isWiFiConnected && isSerialNumberValid) {
    digitalWrite(ledPin, HIGH);

    // Check if button is pressed
    if (digitalRead(actionRegister) == LOW) { // Button pressed
      digitalWrite(ledPin, LOW);
      sendActionData();
      delay(2000);  // Wait for 2 seconds before continuing
    } else {
      // Send flex sensor data to API
      sendFlexData();
    }
  } else if (isWiFiConnected && !isSerialNumberValid) {
    // Blink to indicate waiting for valid serial number
    blinkOnTime = 300;
    blinkOffTime = 300;

    // Retry serial number validation
    isSerialNumberValid = checkSerialNumber();
  } else {
    // Blink quickly to indicate WiFi not connected
    blinkOnTime = 100;
    blinkOffTime = 100;
  }
}

// Non-blocking LED blink function
void blinkLed(unsigned long onTime, unsigned long offTime) {
  unsigned long currentMillis = millis();

  // Check if it's time to turn the LED on or off based on the on/off durations
  if ((ledState == LOW) && (currentMillis - lastBlinkTime >= offTime)) {
    ledState = HIGH;
    lastBlinkTime = currentMillis;
    digitalWrite(ledPin, ledState);  // Turn LED ON
  } else if ((ledState == HIGH) && (currentMillis - lastBlinkTime >= onTime)) {
    ledState = LOW;
    lastBlinkTime = currentMillis;
    digitalWrite(ledPin, ledState);  // Turn LED OFF
  }
}

// Non-blocking WiFi connection function
void handleWiFiConnection() {
  if (wifiConnecting) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi Connected!");
      Serial.println("IP Address: " + WiFi.localIP().toString());
      isWiFiConnected = true;
      wifiConnecting = false;  // Stop trying to reconnect
    } else {
      unsigned long currentMillis = millis();
      if (currentMillis - lastWiFiCheckTime >= wifiCheckInterval) {
        Serial.println("Still trying to connect to WiFi...");
        lastWiFiCheckTime = currentMillis;  // Update the time for next WiFi check
      }
    }
  }
}

bool checkSerialNumber() {
  HTTPClient http;
  String apiEndpoint = "http://mobilitycom.satemp.site/api/gloves/check-serial";
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Connection", "keep-alive");

  // Prepare the data to be sent in the request body
  String httpRequestData = "serial_number=" + serialNumber;

  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response: " + response);

    // Parse JSON response
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);
    bool isSerialValid = doc["isValid"].as<bool>();

    http.end();
    return isSerialValid;

  } else {
    Serial.println("Error in HTTP request: " + String(httpResponseCode));
    http.end();
    return false;
  }
}

void sendFlexData() {
  HTTPClient http;
  http.begin("http://mobilitycom.satemp.site/api/readings");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "keep-alive");

  // Read flex sensor values
  int flexTValue = analogRead(flexThumb);
  int flexIValue = analogRead(flexIndex);
  int flexMValue = analogRead(flexMiddle);
  int flexRValue = analogRead(flexRing);
  int flexPValue = analogRead(flexPinky);

  // Create JSON object
  DynamicJsonDocument doc(1024);
  doc["serial_number"] = serialNumber;
  doc["finger_1"] = flexTValue;
  doc["finger_2"] = flexIValue;
  doc["finger_3"] = flexMValue;
  doc["finger_4"] = flexRValue;
  doc["finger_5"] = flexPValue;

  String requestBody;
  serializeJson(doc, requestBody);

  // Send POST request
  int httpResponseCode = http.POST(requestBody);

  // Check the response
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error in HTTP request: " + String(httpResponseCode));
  }

  http.end();
}

void sendActionData() {
  HTTPClient http;
  http.begin("http://mobilitycom.satemp.site/api/actions");
  http.addHeader("Content-Type", "application/json");

  // Read flex sensor values
  int flexTValue = analogRead(flexThumb);
  int flexIValue = analogRead(flexIndex);
  int flexMValue = analogRead(flexMiddle);
  int flexRValue = analogRead(flexRing);
  int flexPValue = analogRead(flexPinky);

  // Create JSON object
  DynamicJsonDocument doc(1024);
  doc["serial_number"] = serialNumber;
  doc["finger_1"] = flexTValue;
  doc["finger_2"] = flexIValue;
  doc["finger_3"] = flexMValue;
  doc["finger_4"] = flexRValue;
  doc["finger_5"] = flexPValue;

  String requestBody;
  serializeJson(doc, requestBody);

  // Send POST request
  int httpResponseCode = http.POST(requestBody);

  // Check the response
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    Serial.println("Error in HTTP request: " + String(httpResponseCode));
  }

  http.end();
}
