#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include "secrets.h"

// Display Pins
#define TFT_CS    5
#define TFT_RST   33
#define TFT_DC    32
// Hardware SPI uses MOSI = 23, SCLK = 18 natively on ESP32

// Colors for Adafruit_ST7735
#define TFT_BLACK     ST77XX_BLACK
#define TFT_WHITE     ST77XX_WHITE
#define TFT_RED       ST77XX_RED
#define TFT_BLUE      ST77XX_BLUE
#define TFT_DARKGREY  0x7BEF
#define TFT_DARKGREEN 0x03E0

// --- CONFIGURATION FOR BUTTONS AND SCENES ---
// Change NUM_BUTTONS and add to the arrays below to easily add more buttons!
#define NUM_BUTTONS 4

const int buttonPins[NUM_BUTTONS] = {
  15, // Button 1 - PC Camera
  2,  // Button 2 - Smartphone Camera
  0,  // Button 3 - Arduino IDE
  4   // Button 4 - PC Camera + Arduino IDE
};

const char *SCENES[NUM_BUTTONS] = {
  "Câmera do PC", 
  "Câmera do Celular", 
  "Arduino IDE",
  "Arduino IDE + Câmera do PC"
};

// Debounce variables
unsigned long lastDebounceTime[NUM_BUTTONS] = {0};
unsigned long debounceDelay = 50;
int lastButtonState[NUM_BUTTONS];

using namespace websockets;
WebsocketsClient client;

// Initialize Adafruit ST7735 using Hardware SPI
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void connectWiFi() {
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("Connecting Wi-Fi...");
  Serial.print("Connecting to Wi-Fi ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("Wi-Fi OK!");
  tft.println(WiFi.localIP());
}

void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Received from OBS: ");
  Serial.println(message.data());
}

void onEventsCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connected to OBS!");
    tft.fillScreen(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.println("OBS Connected!");
    tft.println("Ready to use.");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Disconnected from OBS!");
    tft.fillScreen(TFT_RED);
    tft.setCursor(0, 0);
    tft.println("OBS Disconnected!");
  }
}

void connectOBS() {
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.println("Searching for OBS...");

  client.onMessage(onMessageCallback);
  client.onEvent(onEventsCallback);

  bool connected = client.connect(OBS_HOST, OBS_PORT, "/");
  if (connected) {
    // Send Authentication (Simple version for WebSocket v5 without password)
    StaticJsonDocument<200> authDoc;
    authDoc["op"] = 1; // Identify
    JsonObject d = authDoc.createNestedObject("d");
    d["rpcVersion"] = 1;

    String authPayload;
    serializeJson(authDoc, authPayload);
    client.send(authPayload);
  } else {
    Serial.println("Failed to connect to OBS.");
    tft.println("Connection error");
  }
}

void drawSceneIcon(const char *sceneName) {
  // Screen is 128px wide and 160px high in portrait mode
  
  tft.setCursor(5, 5);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);

  if (strcmp(sceneName, "Câmera do PC") == 0) {
    tft.println("PC");
    tft.setCursor(5, 25);
    tft.println("Camera");
    
    // Webcam icon
    tft.fillRoundRect(24, 65, 80, 50, 10, TFT_BLACK);
    tft.fillCircle(64, 90, 15, TFT_BLUE);
    tft.fillCircle(64, 90, 5, TFT_WHITE);
    tft.fillRect(54, 115, 20, 10, TFT_DARKGREY);
    tft.fillRect(34, 125, 60, 5, TFT_BLACK);
  } 
  else if (strcmp(sceneName, "Câmera do Celular") == 0) {
    tft.println("Phone");
    tft.setCursor(5, 25);
    tft.println("Camera");

    // Smartphone icon
    tft.fillRoundRect(44, 50, 40, 80, 5, TFT_BLACK);
    tft.fillRect(47, 58, 34, 62, TFT_BLUE);
    tft.fillCircle(64, 54, 2, TFT_WHITE);
    tft.fillCircle(64, 125, 3, TFT_DARKGREY);
  }
  else if (strcmp(sceneName, "Arduino IDE") == 0) {
    tft.println("Arduino");
    tft.setCursor(5, 25);
    tft.println("IDE");

    // Arduino chip icon
    tft.fillRoundRect(34, 60, 60, 60, 5, TFT_DARKGREEN);
    tft.fillCircle(64, 90, 15, TFT_WHITE);
    tft.fillCircle(64, 90, 10, TFT_DARKGREEN);
    for(int i=0; i<3; i++) {
       tft.fillRect(39 + i*20, 50, 5, 10, TFT_DARKGREY);
       tft.fillRect(39 + i*20, 120, 5, 10, TFT_DARKGREY);
    }
  }
  else if (strcmp(sceneName, "Arduino IDE + Câmera do PC") == 0) {
    tft.println("Arduino +");
    tft.setCursor(5, 25);
    tft.println("Camera");

    // Split Screen icon (Arduino + Webcam)
    tft.fillRect(10, 70, 45, 55, TFT_DARKGREEN);
    tft.fillCircle(32, 97, 10, TFT_WHITE);
    
    tft.fillRoundRect(65, 80, 50, 35, 5, TFT_BLACK);
    tft.fillCircle(90, 97, 8, TFT_BLUE);
    tft.fillRect(80, 115, 20, 5, TFT_BLACK);
  }
  else {
    // Generic fallback for any newly added scenes
    tft.println("Scene:");
    tft.setCursor(5, 25);
    // Draw only the first 10 characters to fit the screen
    String shortName = String(sceneName).substring(0, 10);
    tft.println(shortName);

    // Generic Icon
    tft.fillRoundRect(34, 60, 60, 60, 10, TFT_DARKGREY);
    tft.fillCircle(64, 90, 15, TFT_WHITE);
  }
}

void setOBSScene(const char *sceneName) {
  StaticJsonDocument<300> doc;
  doc["op"] = 6; // Request

  JsonObject d = doc.createNestedObject("d");
  d["requestType"] = "SetCurrentProgramScene";
  d["requestId"] = String(millis());

  JsonObject requestData = d.createNestedObject("requestData");
  requestData["sceneName"] = sceneName;

  String payload;
  serializeJson(doc, payload);

  client.send(payload);
  Serial.print("Scene changed to: ");
  Serial.println(sceneName);

  // Update Display
  tft.fillScreen(TFT_WHITE);
  
  // Draw formatted text and icons
  drawSceneIcon(sceneName);
}

void readButtons() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    int reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
    }

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading == LOW) {
        setOBSScene(SCENES[i]);
        delay(300); // Pause to prevent double trigger
      }
    }
    lastButtonState[i] = reading;
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonState[i] = HIGH;
  }

  // Initialize display (INITR_BLACKTAB is common for ST7735S)
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0); // Natural portrait orientation
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Starting...");

  delay(1000);
  connectWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    connectOBS();
  } else if (!client.available()) {
    connectOBS();
    delay(2000);
  }

  if (client.available()) {
    client.poll();
    readButtons();
  }
}
