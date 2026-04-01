#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <RF24.h>
#include <WebServer.h>
#include <Preferences.h>

// --- Cấu hình chân kết nối ---
#define DHTPIN 15
#define DHTTYPE DHT11
#define PIN_CE    4 
#define PIN_CSN   5
#define LED_PIN   2  // Chân D2 trên ESP32

// --- Khởi tạo đối tượng ---
RF24 radio(PIN_CE, PIN_CSN);
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);
Preferences preferences;

// --- Thông số NRF24L01 ---
const uint8_t RxAddress[] = {0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
#define NRF_CHANNEL     40
#define PAYLOAD_SIZE    32
char RxData[PAYLOAD_SIZE + 1];

// --- Biến hệ thống ---
String sta_ssid, sta_password, mqtt_server_ip;
const int mqtt_port = 1883;
String chipID;
unsigned long lastDHTRead = 0;
unsigned long lastMqttReconnect = 0;

// --- Giao diện Web Cấu hình ---
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 Config</title>";
  html += "<style>body{font-family:monospace;background:#0d1117;color:#c9d1d9;text-align:center;}";
  html += "input{display:block;margin:10px auto;padding:8px;width:80%;}";
  html += "button{padding:10px 20px;background:#00ffff;border:none;cursor:pointer;}</style></head><body>";
  html += "<h2>SYSTEM CONFIG</h2><form action='/save' method='POST'>";
  html += "SSID: <input type='text' name='ssid' value='" + sta_ssid + "'>";
  html += "PASS: <input type='password' name='pass' value='" + sta_password + "'>";
  html += "MQTT IP: <input type='text' name='mqtt' value='" + mqtt_server_ip + "'>";
  html += "<button type='submit'>SAVE & REBOOT</button></form></body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid")) preferences.putString("ssid", server.arg("ssid"));
  if (server.hasArg("pass")) preferences.putString("pass", server.arg("pass"));
  if (server.hasArg("mqtt")) preferences.putString("mqtt", server.arg("mqtt"));
  server.send(200, "text/html", "Data Saved! Rebooting...");
  delay(2000);
  ESP.restart();
}

void setup_wifi() {
  if (sta_ssid == "") return;
  Serial.print("\nConnecting to: " + sta_ssid);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500); Serial.print("."); retries++;
  }
  if (WiFi.status() == WL_CONNECTED) Serial.println("\n[OK] IP: " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(LED_PIN, OUTPUT); // Cấu hình chân D2 làm Output

  preferences.begin("config", false);
  sta_ssid       = preferences.getString("ssid", "");
  sta_password   = preferences.getString("pass", "");
  mqtt_server_ip = preferences.getString("mqtt", "192.168.1.100");

  uint64_t chipid = ESP.getEfuseMac();
  chipID = "ESP32_" + String((uint32_t)(chipid >> 32), HEX);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_GT", "12345678");
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  setup_wifi();
  client.setServer(mqtt_server_ip.c_str(), mqtt_port);

  if (!radio.begin()) {
    Serial.println("[ERROR] NRF24L01 Not Found!");
  } else {
    radio.setChannel(NRF_CHANNEL);
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.setDataRate(RF24_2MBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.setAutoAck(false); // Tắt AutoAck để khớp với STM32
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.openReadingPipe(0, RxAddress);
    radio.startListening();
    Serial.println("[OK] NRF Bridge Ready.");
  }
}

// ====== GIỮ NGUYÊN PHẦN TRÊN ======

void loop() {
  server.handleClient();

  // MQTT reconnect
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    if (millis() - lastMqttReconnect > 5000) {
      lastMqttReconnect = millis();
      if (client.connect(chipID.c_str())) {
        Serial.println("[MQTT] Connected");
      }
    }
  } else {
    client.loop();
  }

  // ====== NHẬN STM32 QUA NRF ======
  if (radio.available()) {
    memset(RxData, 0, sizeof(RxData));
    radio.read(RxData, PAYLOAD_SIZE);

    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    if (client.connected()) {

      char *pT = strstr(RxData, "\"T\":");
      char *pH = strstr(RxData, "\"H\":");

      float tempVal = (pT) ? atof(pT + 4) : 0;
      float humVal  = (pH) ? atof(pH + 4) : 0;

      String payload = "{\"ID\":\"" + chipID + "_STM32\",\"temperature\":"
                       + String(tempVal,1) + ",\"humidity\":"
                       + String(humVal,1) + "}";

      client.publish("esp32/dht", payload.c_str());

      Serial.print("[NRF->MQTT] ");
      Serial.println(payload);
    }
  }
}