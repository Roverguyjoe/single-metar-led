#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>

// Define your Wi-Fi credentials
const char* ssid = "ssid";
const char* password = "pass";

// Define the aviationweather.gov API details
#define SERVER "aviationweather.gov"
#define BASE_URI "/cgi-bin/data/dataserver.php?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=3&mostRecentForEachStation=true&stationString="
const char* airport_code = "KPOE";  // The airport code you want the METAR data for

// LED setup
#define LED_PIN D1  // Define the pin connected to the WS2811 data pin
#define NUM_LEDS 1  // Number of LEDs
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);  // I used a ws2811 with GRB color order you may need to change

void setup() {
  Serial.begin(9600);  // Set baud rate to 9600
  strip.begin();
  strip.show();  // Initialize all pixels to 'off'

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");

  while (WiFi.status() != WL_CONNECTED) {
    flashTeal();  // Flash teal while waiting for Wi-Fi connection
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println("Wi-Fi signal strength (RSSI): " + String(WiFi.RSSI()) + " dBm");

  // Test internet connectivity
  testInternetConnectivity();
}

void loop() {
  flashTeal();  // Flash teal while fetching data
  fetchAndDisplayMETAR();
  delay(600000);  // Update every 10 minutes
}

void flashTeal() {
  setLEDColor(0, 128, 128);  // Corrected Teal color (GRB format)
  delay(500);
  setLEDColor(0, 0, 0);  // Turn off
  delay(500);
}

void fetchAndDisplayMETAR() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Disable SSL verification temporarily
    HTTPClient http;

    // Ensure using https:// to connect securely
    String url = "https://" + String(SERVER) + BASE_URI + airport_code;  // Construct the full URL with HTTPS
    
    Serial.println("Fetching METAR data from URL: " + url);

    http.begin(client, url);  // Use the updated method with WiFiClientSecure
    http.setTimeout(30000);  // Set timeout to 30 seconds

    int httpCode = http.GET();
    Serial.println("HTTP Response Code: " + String(httpCode));

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("METAR Data: " + payload);  // Print the raw METAR data
      parseAndSetColor(payload);
    } else if (httpCode == 307) {
      Serial.println("Redirected to: " + http.getLocation());  // Print redirected URL
      fetchAndDisplayMETAR();  // Retry the request
    } else {
      Serial.println("Failed to fetch METAR data. HTTP Error: " + String(httpCode));
    }
    http.end();
  } else {
    Serial.println("Wi-Fi not connected.");
  }
}

void parseAndSetColor(String payload) {
  if (payload.indexOf("<flight_category>VFR</flight_category>") > -1) {
    setLEDColor(0, 255, 0);  // Green for VFR (GRB format)
  } else if (payload.indexOf("<flight_category>MVFR</flight_category>") > -1) {
    setLEDColor(0, 0, 255);  // Blue for MVFR (GRB format)
  } else if (payload.indexOf("<flight_category>IFR</flight_category>") > -1) {
    setLEDColor(255, 0, 0);  // Red for IFR (GRB format)
  } else if (payload.indexOf("<flight_category>LIFR</flight_category>") > -1) {
    setLEDColor(255, 0, 255);  // Magenta for LIFR (GRB format)
  }
}

void setLEDColor(int red, int green, int blue) {
  strip.setPixelColor(0, strip.Color(green, red, blue));  // Correct color order (GRB)
  strip.show();
}

void testInternetConnectivity() {
  WiFiClientSecure client;
  HTTPClient testHttp;
  testHttp.begin(client, "https://www.google.com");  // Use HTTPS here for testing
  int testHttpCode = testHttp.GET();
  Serial.println("Google Test HTTP Response Code: " + String(testHttpCode));
  testHttp.end();
}
