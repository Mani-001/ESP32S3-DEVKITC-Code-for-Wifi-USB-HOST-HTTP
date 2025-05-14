#include <WiFi.h>
#include <WebSocketsClient.h>
#include "EspUsbHostKeybord.h"

// Wi-Fi credentials
const char* ssid = "GWN-Guest";
const char* password = "F@st!234";

// WebSocket Server
const char* websocketServer = "172.16.84.83";
const uint16_t websocketPort = 3000;
const char* websocketPath = "/";

// State Management
enum State {
  INITIALIZE_WIFI,
  INITIALIZE_WEBSOCKET,
  START_USB_SCAN
};

State currentState = INITIALIZE_WIFI;

// Flag to check if barcode is scanned
bool barcodeScanned = false;

// WebSocket client
WebSocketsClient webSocket;

// Custom USB Keyboard class
class MyEspUsbHostKeybord : public EspUsbHostKeybord {
public:
  String barcode = "";

  char keycodeToChar(uint8_t keycode) {
    if (keycode >= 0x04 && keycode <= 0x1d) return 'a' + (keycode - 0x04);
    if (keycode >= 0x1e && keycode <= 0x26) return '1' + (keycode - 0x1e);
    if (keycode == 0x27) return '0';
    if (keycode == 0x28) return '\n';
    return 0;
  }

  void onKey(usb_transfer_t *transfer) override {
    uint8_t* p = transfer->data_buffer;
    for (int i = 2; i < 8; i++) {
      char c = keycodeToChar(p[i]);
      if (c != 0) {
        if (c == '\n') {
          Serial.println("Scanned Barcode: " + barcode);
          barcodeScanned = true;
        } else {
          barcode += c;
        }
      }
    }
  }
};

MyEspUsbHostKeybord usbHost;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_DISCONNECTED) {
    Serial.println("[INFO] WebSocket Disconnected");
  } else if (type == WStype_CONNECTED) {
    Serial.println("[INFO] WebSocket Connected");
  } else if (type == WStype_TEXT) {
    Serial.printf("[INFO] WebSocket Message: %s\n", payload);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void loop() {
  switch (currentState) {
    case INITIALIZE_WIFI:
      Serial.println("[INFO] Initializing WiFi...");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("\n[INFO] WiFi connected!");
      Serial.print("[INFO] IP: ");
      Serial.println(WiFi.localIP());
      currentState = INITIALIZE_WEBSOCKET;
      break;

    case INITIALIZE_WEBSOCKET:
      Serial.println("[INFO] Connecting to WebSocket Server...");
      webSocket.begin(websocketServer, websocketPort, websocketPath);
      webSocket.onEvent(webSocketEvent);
      webSocket.setReconnectInterval(5000);
      currentState = START_USB_SCAN;
      break;

    case START_USB_SCAN:
      Serial.println("[INFO] Starting USB Host...");
      delay(2000);  // Added delay to stabilize power
      bool usbInit = usbHost.begin();
      if (!usbInit) {
        Serial.println("[ERROR] USB Host Initialization Failed.");
      } else {
        Serial.println("[INFO] USB Host Initialized Successfully.");
      }
      break;
  }

  webSocket.loop();
  
  if (barcodeScanned) {
    Serial.println("[INFO] Sending barcode to WebSocket...");
    webSocket.sendTXT("{\"barcode\":\"" + usbHost.barcode + "\"}");
    usbHost.barcode = "";
    barcodeScanned = false;
  }
}

