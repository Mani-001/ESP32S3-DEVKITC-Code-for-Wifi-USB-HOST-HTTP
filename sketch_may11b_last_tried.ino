#include <WiFi.h>
#include <HTTPClient.h>
#include "EspUsbHostKeybord.h"

// Wi-Fi credentials
const char* ssid = "GWN-Guest";
const char* password = "F@st!234";

// Your API endpoint
const char* apiEndpoint = "http://192.168.41.159:3000/api/scan";

// Custom USB Keyboard class
class MyEspUsbHostKeybord : public EspUsbHostKeybord {
public:
  String barcode = "";

  char keycodeToChar(uint8_t keycode) {
    if (keycode >= 0x04 && keycode <= 0x1d) return 'a' + (keycode - 0x04);   // a-z
    if (keycode >= 0x1e && keycode <= 0x26) return '1' + (keycode - 0x1e);   // 1-9
    if (keycode == 0x27) return '0';                                         // 0
    if (keycode == 0x28) return '\n';                                        // Enter
    return 0;  // unsupported
  }

  void sendBarcodeToAPI(const String& barcodeData) {
    // 1) Make sure we’re still on Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[ERROR] WiFi not connected. Cannot send data.");
      return;
    }

    // 2) Create a fresh TCP client & HTTP client
    HTTPClient http;

    Serial.println("[INFO] Connecting to API...");
    // Pass our WiFiClient explicitly:
    if (!http.begin(apiEndpoint)) {
      Serial.println("[ERROR] HTTP begin() failed");
      return;
    }

    // 3) Set headers + timeouts
    http.setReuse(false);         // don’t reuse old connections
    http.setTimeout(10000000);       // 10s timeout
    http.addHeader("Content-Type", "application/json");

    // 4) Build & send payload
    String payload = "{\"barcode\":\"testdata123\"}";
    Serial.println("[INFO] Sending payload: " + payload);
    int httpResponseCode = http.POST(payload);

    // 5) Handle the response
    if (httpResponseCode > 0) {
      Serial.print("[INFO] HTTP Response Code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println("[INFO] Server Response: " + response);
    } else {
      Serial.print("[ERROR] POST failed: ");
      Serial.println(http.errorToString(httpResponseCode));
    }

    // 6) Clean up
    http.end();
  }

  void onKey(usb_transfer_t *transfer) override {
    uint8_t* p = transfer->data_buffer;
    for (int i = 2; i < 8; i++) {
      char c = keycodeToChar(p[i]);
      if (c != 0) {
        if (c == '\n') {
          Serial.println("Scanned Barcode: " + barcode);
          sendBarcodeToAPI(barcode);
          barcode = "";
        } else {
          barcode += c;
        }
      }
    }
  }
};

MyEspUsbHostKeybord usbHost;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Wi-Fi connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[INFO] WiFi connected!");
  Serial.print("[INFO] IP Address: ");
  Serial.println(WiFi.localIP());

  // Add a small delay before USB starts to avoid clashes
  delay(2000);  // Give Wi-Fi some time to stabilize

  usbHost.begin();
  Serial.println("[INFO] USB Host Initialized");
  Serial.println("[INFO] Waiting for barcode scanner input...");
}

void loop() {
  usbHost.task();  // This must run continuously

  // Check if the Wi-Fi is still connected and handle reconnection if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi disconnected, attempting to reconnect...");
    WiFi.begin(ssid, password);  // Reconnect
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n[INFO] Reconnected to Wi-Fi");
  }
}
