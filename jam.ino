#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <RemoteDebug.h>

#include "utils.h"
#include "html.h"

char defaultSSID[32] = "";
char defaultPassword[32] = "";
const char* apSSID = "erlandClock";
const int hourPin = 15;    // PWM pin for hour meter (D8)
const int minutePin = 13;  // PWM pin for minute meter (D7)
const int secondPin = 4;   // PWM pin for second meter (D2)
int hourMaxPWM = 190;
int minuteMaxPWM = 190;
int secondMaxPWM = 187;
bool testMode = false;
unsigned long testModeStart = 0;
int testHour = 12;
int testMinute = 59;
int testSecond = 59;

// Create an instance of the NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200);

// Create an instance of the web server
ESP8266WebServer server(80);

// Initialize RemoteDebug object
RemoteDebug Debug;

void setupWiFiAP() {
  Debug.println("Starting AP mode...");
  WiFi.softAP(apSSID);
  Debug.println("AP IP address:");
  Debug.println(WiFi.softAPIP());
}

void connectToWiFi(const char* ssid, const char* password) {
  Debug.print("Connecting to WiFi ");
  Debug.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Debug.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Debug.println("\nWiFi connected");
    Debug.print("IP address: ");
    Debug.println(WiFi.localIP());
  } else {
    Debug.println("Failed to connect to WiFi");
  }
}

void setup() {
  // Adding a delay to allow the system to stabilize
  delay(500);

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Debug.println("Failed to mount file system");
    return;
  } else {
    Debug.println("SPIFFS mounted successfully");
  }

  // Start WiFi in AP mode if SSID and password are not set
  if (!loadWiFiCredentials()) {
    setupWiFiAP();
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
    server.send(200, "text/html", getHtmlContent(defaultSSID, defaultPassword));
  });

  server.on("/connect", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    saveWiFiCredentials(ssid.c_str(), password.c_str());
    connectToWiFi(ssid.c_str(), password.c_str());
    server.send(200, "text/plain", "WiFi configuration saved!");
  });

  server.on("/settime", HTTP_POST, []() {
    int hour = server.arg("hour").toInt();
    int minute = server.arg("minute").toInt();
    int second = server.arg("second").toInt();

    // Set test mode variables for 10 seconds
    testHour = hour;
    testMinute = minute;
    testSecond = second;
    testMode = true;
    testModeStart = millis();

    server.send(200, "text/plain", "Setting time for 10 seconds.");
  });

  server.on("/time", HTTP_GET, []() {
    String currentTime = timeClient.getFormattedTime();
    String json = "{\"time\": \"" + currentTime + "\"}";
    server.send(200, "application/json", json);
  });

  // Initialize ArduinoOTA
  ArduinoOTA.setPassword("imroot");  // Set OTA password here
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    Debug.printf("%s\n", type);
  });
  ArduinoOTA.onEnd([]() {
    Debug.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Debug.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Debug.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Debug.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Debug.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Debug.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Debug.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Debug.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  server.begin();

  Debug.begin("remotedebug");      // Initialize the WiFi server
  Debug.setResetCmdEnabled(true);  // Enable the reset command
  Debug.showProfiler(true);        // Profiler (Good to measure times, to optimize codes)
  Debug.showColors(true);          // Colors

  Debug.println("Ready");
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // Handle web server requests
  server.handleClient();

  // Handle RemoteDebug client connections
  Debug.handle();

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

  // Handle test mode
  if (testMode) {
    hours = testHour;
    minutes = testMinute;
    seconds = testSecond;

    unsigned long currentTime = millis();
    if (currentTime - testModeStart > 10000) {
      testMode = false;  // Exit test mode after 10 seconds
    }
  }

  // Map the time to PWM values
  int hoursPWM = mapHourToPWM(hours);
  int minutePWM = mapMinuteToPWM(minutes);
  int secondPWM = mapSecondToPWM(seconds);

  // Output PWM signals to the meters
  analogWrite(hourPin, hoursPWM);
  analogWrite(minutePin, minutePWM);
  analogWrite(secondPin, secondPWM);

  // Update every second
  delay(1000);
}



void saveWiFiCredentials(const char* ssid, const char* password) {
  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile) {
    Debug.println("Failed to open config file for writing");
    return;
  }

  configFile.println(ssid);
  configFile.println(password);
  configFile.close();

  strncpy(defaultSSID, ssid, sizeof(defaultSSID) - 1);
  strncpy(defaultPassword, password, sizeof(defaultPassword) - 1);
}

bool loadWiFiCredentials() {
  File configFile = SPIFFS.open("/config.txt", "r");
  if (!configFile) {
    Debug.println("Failed to open config file");
    return false;
  }

  String ssid = configFile.readStringUntil('\n');
  String password = configFile.readStringUntil('\n');
  configFile.close();

  ssid.trim();
  password.trim();

  if (ssid.length() > 0 && password.length() > 0) {
    ssid.toCharArray(defaultSSID, sizeof(defaultSSID) - 1);
    password.toCharArray(defaultPassword, sizeof(defaultPassword) - 1);
    return true;
  }

  return false;
}

void saveCalibrationValues() {
  File configFile = SPIFFS.open("/calibration.txt", "w");
  if (!configFile) {
    Debug.println("Failed to open calibration file for writing");
    return;
  }

  configFile.println(hourMaxPWM);
  configFile.println(minuteMaxPWM);
  configFile.println(secondMaxPWM);
  configFile.close();
}

void loadCalibrationValues() {
  File configFile = SPIFFS.open("/calibration.txt", "r");
  if (!configFile) {
    Debug.println("Failed to open calibration file");
    return;
  }

  hourMaxPWM = configFile.readStringUntil('\n').toInt();
  minuteMaxPWM = configFile.readStringUntil('\n').toInt();
  secondMaxPWM = configFile.readStringUntil('\n').toInt();
  configFile.close();
}
