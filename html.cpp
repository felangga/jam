#include <ESP8266WiFi.h>
#include "html.h"

// HTML web page to handle the WiFi configuration
String getHtmlContent(String ssid, String password) {
  String html = "<html><head><title>erlandClock Configuration</title><style>";
  html += "body { font-family: Arial, sans-serif; }";
  html += "form { margin-bottom: 20px; }";
  html += "label { display: inline-block; width: 150px; text-align: right; margin-right: 10px; }";
  html += "input[type='text'], input[type='password'], input[type='number'] { width: 200px; padding: 5px; }";
  html += "input[type='submit'] { margin-left: 160px; padding: 5px 10px; }";
  html += "</style></head><body><h1>WiFi and Calibration Configuration</h1>";

  html += "<form action='/connect' method='post'>";
  html += "<label for='ssid'>SSID:</label> <input type='text' name='ssid' value='" + ssid + "'><br>";
  html += "<label for='password'>Password:</label> <input type='password' name='password' value='" + password + "'><br>";
  html += "<input type='submit' value='Connect'></form>";

  html += "<h2>Current Time</h2>";
  html += "<div id='currentTime'>Loading...</div>";

  html += "<h2>[DEBUG] Test Time</h2>";
  html += "<form action='/settime' method='post'>";
  html += "<label for='hour'>Hour:</label> <input type='number' name='hour' min='1' max='12' value='1'><br>";
  html += "<label for='minute'>Minute:</label> <input type='number' name='minute' min='0' max='59' value='0'><br>";
  html += "<label for='second'>Second:</label> <input type='number' name='second' min='0' max='59' value='0'><br>";
  html += "<input type='submit' value='Set Time'></form>";

  html += "<script>";
  html += "function updateTime() {";
  html += "  fetch('/time').then(response => response.json()).then(data => {";
  html += "    document.getElementById('currentTime').innerText = data.time;";
  html += "  });";
  html += "}";
  html += "setInterval(updateTime, 1000);";
  html += "updateTime();";  // Initial call to update the time immediately
  html += "</script>";

  html += "</body></html>";
  return html;
}
