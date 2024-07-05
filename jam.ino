#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>

const char* defaultSSID = "DefaultSSID";
const char* defaultPassword = "DefaultPassword";

const char* apSSID = "ESP32Clock";
const char* apPassword = "password";

const int hourPin = 5;    // PWM pin for hour meter (example pin)
const int minutePin = 18; // PWM pin for minute meter (example pin)
const int secondPin = 19; // PWM pin for second meter (example pin)

// Create an instance of the NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Create an instance of the web server
WebServer server(80);

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

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Start WiFi in AP mode if SSID and password are not set
  if (!loadWiFiCredentials()) {
    Serial.println("Starting AP mode...");
    WiFi.softAP(apSSID, apPassword);
    Serial.println("AP IP address:");
    Serial.println(WiFi.softAPIP());
  } else {
    // Connect to WiFi using loaded credentials
    connectToWiFi();
  }

  // Initialize PWM pins
  pinMode(hourPin, OUTPUT);
  pinMode(minutePin, OUTPUT);
  pinMode(secondPin, OUTPUT);

  // Initialize NTP client
  timeClient.begin();

  // Setup web server routes
  server.on("/", HTTP_GET, [](){
    server.send(200, "text/html", htmlContent);
  });

  server.on("/connect", HTTP_POST, [](){
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    saveWiFiCredentials(ssid.c_str(), password.c_str());
    connectToWiFi();
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

  // Map the time to PWM values
  int hourPWM = map(hours, 0, 23, 0, 255);    // 0-23 hours mapped to 0-255 PWM
  int minutePWM = map(minutes, 0, 59, 0, 255); // 0-59 minutes mapped to 0-255 PWM
  int secondPWM = map(seconds, 0, 59, 0, 255); // 0-59 seconds mapped to 0-255 PWM

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

  if (ssid.length() > 0 && password.length() > 0) {
    strcpy(defaultSSID, ssid.c_str());
    strcpy(defaultPassword, password.c_str());
    return true;
  }

  return false;
}

void connectToWiFi() {
  WiFi.begin(defaultSSID, defaultPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}
