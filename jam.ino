#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>

char defaultSSID[32] = "";
char defaultPassword[32] = "";

const char* apSSID = "erlandClock";

const int hourPin = 15;    // PWM pin for hour meter (D8)
const int minutePin = 13;  // PWM pin for minute meter (D7)
const int secondPin = 4;   // PWM pin for second meter (D2)

// Create an instance of the NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200);

// Create an instance of the web server
ESP8266WebServer server(80);

// HTML web page to handle the WiFi configuration
const char* htmlContent =
  "<html><body><h1>WiFi Configuration</h1>"
  "<form action='/connect' method='post'>"
  "SSID: <input type='text' name='ssid'><br>"
  "Password: <input type='password' name='password'><br>"
  "<input type='submit' value='Connect'></form>"
  "</body></html>";

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Adding a delay to allow the system to stabilize
  delay(500);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  } else {
    Serial.println("SPIFFS mounted successfully");
  }

  // Start WiFi in AP mode if SSID and password are not set
  if (!loadWiFiCredentials()) {
    Serial.println("Starting AP mode...");
    WiFi.softAP(apSSID);
    Serial.println("AP IP address:");
    Serial.println(WiFi.softAPIP());
  } else {
    // Connect to WiFi using loaded credentials
    connectToWiFi(defaultSSID, defaultPassword);
  }

  // Initialize PWM pins
  pinMode(hourPin, OUTPUT);
  pinMode(minutePin, OUTPUT);
  pinMode(secondPin, OUTPUT);

  // Initialize NTP client
  timeClient.begin();

  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlContent);
  });

  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    saveWiFiCredentials(ssid.c_str(), password.c_str());
    connectToWiFi(ssid.c_str(), password.c_str());
    server.send(200, "text/plain", "Connecting to WiFi...");
  });

  server.begin();
}

void loop() {
  // Handle web server requests
  server.handleClient();

  // Update NTP time
  timeClient.update();

  // Get the current time
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  hours = hours % 12;
  if (hours == 0) {
    hours = 12; 
  }

  // Map the time to PWM values
  int hourPWM = map(hours, 0, 12, 0, 190);      // 0-23 hours mapped to 0-255 PWM
  int minutePWM = map(minutes, 0, 59, 0, 190);  // 0-59 minutes mapped to 0-255 PWM
  int secondPWM = map(seconds, 0, 59, 0, 187);  // 0-59 seconds mapped to 0-255 PWM

  // Debug prints
  Serial.print("Map: ");
  Serial.print(hourPWM);
  Serial.print(":");
  Serial.print(minutePWM);
  Serial.print(":");
  Serial.println(secondPWM);

  // Output PWM signals to the meters
  analogWrite(hourPin, hourPWM);
  analogWrite(minutePin, minutePWM);
  analogWrite(secondPin, secondPWM);

  // Update every second
  delay(1000);
}

void saveWiFiCredentials(const char* ssid, const char* password) {
  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  configFile.println(ssid);
  configFile.println(password);
  configFile.close();
}

bool loadWiFiCredentials() {
  File configFile = SPIFFS.open("/config.txt", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  String ssid = configFile.readStringUntil('\n');
  String password = configFile.readStringUntil('\n');
  configFile.close();

  ssid.trim();
  password.trim();

  if (ssid.length() > 0 && password.length() > 0) {
    strncpy(defaultSSID, ssid.c_str(), sizeof(defaultSSID) - 1);
    strncpy(defaultPassword, password.c_str(), sizeof(defaultPassword) - 1);
    return true;
  }

  return false;
}

void connectToWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();

  // Attempt to connect for 10 seconds
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi within 10 seconds. Restarting in AP mode...");

    // Clear stored credentials
    clearWiFiCredentials();

    // Restart in AP mode
    WiFi.softAP(apSSID);
    Serial.println("AP IP address:");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Connected to WiFi");
  }
}

void clearWiFiCredentials() {
  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for clearing");
    return;
  }

  configFile.println("");
  configFile.println("");
  configFile.close();
}
