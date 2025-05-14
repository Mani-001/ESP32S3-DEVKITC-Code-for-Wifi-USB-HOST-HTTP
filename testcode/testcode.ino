#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "dlink-6598";
const char* password = "MINICLIP0012";
const char* apiEndpoint = "http://192.168.0.175:3000/api/scan";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"barcode\":\"TEST123\"}";
  Serial.println("[INFO] Sending Test Payload: " + payload);

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode > 0) {
    Serial.print("[INFO] HTTP Response Code: ");
    Serial.println(httpResponseCode);
    Serial.println("[INFO] Response:");
    Serial.println(http.getString());
  } else {
    Serial.print("[ERROR] Failed to send POST request: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  http.end();
}

void loop() {}
