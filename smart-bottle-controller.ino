#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Adafruit_VL53L0X.h>

const char* ssid = "smart-bottle";
const char* password = "password123";
const char* url = "https://8080-cs-548004209681-default.cs-europe-west1-xedi.cloudshell.dev/data/insert";

String option = "<select id='wifiName' name='wifi_name' required>";

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
VL53L0X_RangingMeasurementData_t measure;
float prevAmount = 0;
bool firstBoot = true;
const int buttonPin = 19;
bool waterAmountChanged = false;

WebServer server(80);

void setup() {
  Serial.begin(115200);

  while (! Serial) {
    delay(1);
  }

  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    option += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }
  option += "</select>";

  WiFi.softAP(ssid, password);

  Serial.println("Access point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", showSetupPage);
  server.on("/showToken", showTokenPage);
  server.on("/setToken", tokenSubmit);
  server.on("/setup", setupSubmit);

  server.begin();

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

  pinMode(buttonPin, INPUT);
}

void loop() {
  server.handleClient();
  int buttonValue = digitalRead(buttonPin);

  if (buttonValue == LOW) {
    if (firstBoot) {
      prevAmount = getWaterAmount();
      firstBoot = false;
      delay(1000);
      Serial.print("First boot amount value: ");Serial.println(prevAmount);
    } else {
      waterAmountChanged = true;
      delay(500);
    }
  }

  if (waterAmountChanged) {
    lox.rangingTest(&measure, false);
    float amount = static_cast<float>(measure.RangeMilliMeter) / 10;

    if (prevAmount - 2 >= amount) {
      prevAmount = amount;
      Serial.print("Water amount increase: ");Serial.println(prevAmount);
      delay(1000);
    } else {
      if (amount <= 29) {
        float newAmount = getWaterAmount();
        Serial.print("New water amount: ");Serial.println(newAmount);
        Serial.print("Prew water amount: ");Serial.println(prevAmount);
        Serial.print("Send data: ");Serial.println(newAmount - prevAmount);
        sendRequest(newAmount - prevAmount);
        prevAmount = newAmount;
      }
    }
    delay(1000);
    waterAmountChanged = false;
  }
}

float getWaterAmount() {
  float amount = 0;
  for (int i = 0; i < 10; i++) {
    lox.rangingTest(&measure, false);
    amount += static_cast<float>(measure.RangeMilliMeter) / 10;
    delay(500);
  }

  return amount / 10;
}

void showTokenPage() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Token Configuration</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body {";
  html += "    font-family: Arial, sans-serif;";
  html += "    margin: 0;";
  html += "    padding: 20px;";
  html += "    background-color: #222;";
  html += "    color: #fff;";
  html += "}";
  html += "h1 {";
  html += "    text-align: center;";
  html += "    color: #fff;";
  html += "}";
  html += "form {";
  html += "    max-width: 300px;";
  html += "    margin: auto;";
  html += "}";
  html += "label {";
  html += "    display: block;";
  html += "    margin-bottom: 10px;";
  html += "    color: #ccc;";
  html += "}";
  html += "input[type=\"text\"],";
  html += "input[type=\"password\"],";
  html += "select {";
  html += "    width: 100%;";
  html += "    padding: 8px;";
  html += "    border: 1px solid #ccc;";
  html += "    border-radius: 4px;";
  html += "    background-color: #333;";
  html += "    color: #fff;";
  html += "}";
  html += "input[type=\"submit\"] {";
  html += "    background-color: #4CAF50;";
  html += "    color: white;";
  html += "    border: none;";
  html += "    padding: 10px 20px;";
  html += "    text-align: center;";
  html += "    text-decoration: none;";
  html += "    display: inline-block;";
  html += "    font-size: 16px;";
  html += "    border-radius: 4px;";
  html += "    cursor: pointer;";
  html += "}";
  html += "input[type=\"submit\"]:hover {";
  html += "    background-color: #45a049;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Wi-Fi Configuration</h1>";
  html += "<form method=\"post\" action=\"/setToken\">";
  html += "    <label for=\"tokenId\">Token</label>";
  html += "    <input type=\"text\" id=\"tokenId\" name=\"token\" required>";
  html += "<br><br>";
  html += "    <input type=\"submit\" value=\"Set token\">";
  html += "</form>";
  html += "<br><br>";
  html += getWaterAmount();
  html += "<br><br>";
  html += "</body>";
  html += "</html>";
  server.send(200, "text/html", html);
}

void tokenSubmit() {
  String token = server.arg("token");

  accessToken = "Bearer " + token;

  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Response</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body {";
  html += "    font-family: Arial, sans-serif;";
  html += "    margin: 0;";
  html += "    padding: 20px;";
  html += "    background-color: #222;";
  html += "    color: #fff;";
  html += "}";
  html += "h1 {";
  html += "    text-align: center;";
  html += "    color: #fff;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<a href=\"/\"><h1>" + accessToken + "</a></h1>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

void showSetupPage() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Wi-Fi Configuration</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body {";
  html += "    font-family: Arial, sans-serif;";
  html += "    margin: 0;";
  html += "    padding: 20px;";
  html += "    background-color: #222;";
  html += "    color: #fff;";
  html += "}";
  html += "h1 {";
  html += "    text-align: center;";
  html += "    color: #fff;";
  html += "}";
  html += "form {";
  html += "    max-width: 300px;";
  html += "    margin: auto;";
  html += "}";
  html += "label {";
  html += "    display: block;";
  html += "    margin-bottom: 10px;";
  html += "    color: #ccc;";
  html += "}";
  html += "input[type=\"text\"],";
  html += "input[type=\"password\"],";
  html += "select {";
  html += "    width: 100%;";
  html += "    padding: 8px;";
  html += "    border: 1px solid #ccc;";
  html += "    border-radius: 4px;";
  html += "    background-color: #333;";
  html += "    color: #fff;";
  html += "}";
  html += "input[type=\"submit\"] {";
  html += "    background-color: #4CAF50;";
  html += "    color: white;";
  html += "    border: none;";
  html += "    padding: 10px 20px;";
  html += "    text-align: center;";
  html += "    text-decoration: none;";
  html += "    display: inline-block;";
  html += "    font-size: 16px;";
  html += "    border-radius: 4px;";
  html += "    cursor: pointer;";
  html += "}";
  html += "input[type=\"submit\"]:hover {";
  html += "    background-color: #45a049;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Wi-Fi Configuration</h1>";
  html += "<form method=\"post\" action=\"/setup\">";
  html += "    <label for=\"wifiName\">Wi-Fi Name:</label>";
  html += option;
  html += "<br><br>";
  html += "    <label for=\"wifiPassword\">Wi-Fi Password:</label>";
  html += "    <input type=\"password\" id=\"wifiPassword\" name=\"password\" required>";
  html += "<br><br>";
  html += "    <input type=\"submit\" value=\"Save Wi-Fi Configuration\">";
  html += "</form>";
  html += "</body>";
  html += "</html>";
  server.send(200, "text/html", html);
}

void setupSubmit() {
  String wifiName = server.arg("wifi_name");
  String password = server.arg("password");

  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Response</title>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body {";
  html += "    font-family: Arial, sans-serif;";
  html += "    margin: 0;";
  html += "    padding: 20px;";
  html += "    background-color: #222;";
  html += "    color: #fff;";
  html += "}";
  html += "h1 {";
  html += "    text-align: center;";
  html += "    color: #fff;";
  html += "}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<a href=\"/\"><h1>" + connectToWifi(wifiName, password) + "</a></h1>";
  html += "</body>";
  html += "</html>";

  server.send(200, "text/html", html);
}

String connectToWifi(String ssidWIFI, String passwordWIFI) {
  WiFi.begin(ssidWIFI.c_str(), passwordWIFI.c_str());\

  unsigned long timeout = millis() + 10000;

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > timeout) {
      return "Incorrect password. Failed to connect " + ssidWIFI + ".";
    }
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to " + ssidWIFI + ".");
  return "Connected to " + ssidWIFI + ".";
}

void sendRequest(float data) {
  HTTPClient http;
  http.begin(url);

  http.addHeader("Content-Length", "0");
  http.addHeader("Authorization", accessToken);
  http.addHeader("Content-Type", "text/plain");

  int httpResponseCode = http.POST(String(data));
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.println("Error on HTTP request");
  }

  http.end();
}
