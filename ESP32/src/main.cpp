#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <RF24.h>
#include <WebServer.h>    // Thư viện tạo Web Server
#include <Preferences.h>  // Thư viện lưu trữ dữ liệu vào bộ nhớ Flash

// --- ĐÃ ĐỔI CHÂN DHT11 SANG 15 ---
#define DHTPIN 14
#define DHTTYPE DHT11

// --- CẤU HÌNH NRF24L01 ---
#define PIN_CE    4 
#define PIN_CSN   5
RF24 radio(PIN_CE, PIN_CSN);
const uint8_t RxAddress[] = {0xEE, 0xDD, 0xCC, 0xBB, 0xAA};
#define NRF_CHANNEL     40
#define PAYLOAD_SIZE    32

WiFiClient espClient;
PubSubClient client(espClient);
char RxData[PAYLOAD_SIZE + 1];
DHT dht(DHTPIN, DHTTYPE);

// --- BIẾN CẤU HÌNH WEB SERVER & WIFI ---
WebServer server(80);
Preferences preferences;
String sta_ssid;
String sta_password;
String mqtt_server_ip;
const int mqtt_port = 1883;

// Các biến hệ thống
const char* topic_led_status = "esp32/led";
String chipID;
unsigned long lastDHTRead = 0;
unsigned long lastMqttReconnect = 0;

// ================== HÀM XỬ LÝ GIAO DIỆN WEB CẤU HÌNH ==================
void handleRoot() {
  // Giao diện Web được thiết kế theo tone màu Dark Electrical
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>ESP32 System Config</title>";
  html += "<style>body{font-family:'Courier New',monospace;background:#0d1117;color:#c9d1d9;padding:20px;text-align:center;}";
  html += "input{display:block;margin:15px auto;padding:10px;width:90%;max-width:300px;border-radius:5px;border:1px solid #33ff33;background:#161b22;color:#fff;}";
  html += "button{padding:12px 25px;background:#00ffff;color:#000;border:none;border-radius:5px;cursor:pointer;font-weight:bold;text-transform:uppercase;}";
  html += "</style></head><body><h2>SYSTEM CONFIGURATION</h2>";
  html += "<form action='/save' method='POST'>";
  html += "<label>WiFi SSID:</label><input type='text' name='ssid' value='" + sta_ssid + "'>";
  html += "<label>WiFi Password:</label><input type='password' name='pass' value='" + sta_password + "'>";
  html += "<label>MQTT Server IP:</label><input type='text' name='mqtt' value='" + mqtt_server_ip + "'>";
  html += "<button type='submit'>SAVE & REBOOT</button></form></body></html>";
  
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid")) preferences.putString("ssid", server.arg("ssid"));
  if (server.hasArg("pass")) preferences.putString("pass", server.arg("pass"));
  if (server.hasArg("mqtt")) preferences.putString("mqtt", server.arg("mqtt"));

  String html = "<!DOCTYPE html><html><body style='background:#0d1117;color:#00ffff;font-family:monospace;text-align:center;margin-top:50px;'><h2>DATA SAVED!</h2><p>System is rebooting... Please connect to your Home WiFi.</p></body></html>";
  server.send(200, "text/html", html);
  
  delay(2000);
  ESP.restart(); // Khởi động lại chip để áp dụng WiFi mới
}
// =======================================================================

void setup_wifi() {
  if (sta_ssid == "") {
    Serial.println("Chưa có cấu hình WiFi. Đang đợi ở chế độ AP...");
    return;
  }

  Serial.println("\nĐang kết nối tới: " + sta_ssid);
  WiFi.begin(sta_ssid.c_str(), sta_password.c_str());
  
  int retries = 0;
  // Thử kết nối trong khoảng 10 giây (20 * 500ms)
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[FAIL] Không thể kết nối WiFi. Hãy vào cấu hình lại!");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(2, OUTPUT);

  // 1. Mở bộ nhớ đọc cấu hình
  preferences.begin("config", false);
  sta_ssid = preferences.getString("ssid", "");
  sta_password = preferences.getString("pass", "");
  mqtt_server_ip = preferences.getString("mqtt", "192.168.1.100"); // IP mặc định nếu chưa cài

  // 2. Tạo ID thiết bị
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  chipID = "ESP32_" + mac.substring(mac.length() - 6);

  // 3. Khởi tạo chế độ AP_STA (Vừa phát WiFi, vừa thu WiFi)
  WiFi.mode(WIFI_AP_STA);
  // --- ĐÃ ĐỔI TÊN MẠNG THÀNH ESP32_GT ---
  WiFi.softAP("ESP32_GT", "12345678"); 
  Serial.println("\n[AP] Đã tạo trạm phát WiFi: ESP32_GT (Pass: 12345678)");
  Serial.println("[AP] Truy cập IP cấu hình: 192.168.4.1");

  // 4. Định tuyến Web Server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  // 5. Thử kết nối WiFi nhà
  setup_wifi();

  // 6. Cấu hình MQTT
  client.setServer(mqtt_server_ip.c_str(), mqtt_port);

  // 7. Khởi tạo NRF24L01
  if (!radio.begin()) {
    Serial.println("[ERROR] NRF24L01 không phản hồi!");
  } else {
    radio.setChannel(NRF_CHANNEL);
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.setDataRate(RF24_2MBPS);
    radio.setPALevel(RF24_PA_MAX);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.openReadingPipe(0, RxAddress);
    radio.startListening();
    Serial.println("[OK] Bridge System Ready.");
  }
} 

void loop() {
  // 1. DUY TRÌ WEB SERVER CHẠY LIÊN TỤC
  server.handleClient();

  // 2. KẾT NỐI MQTT (Non-blocking, thử lại mỗi 5 giây nếu rớt)
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      unsigned long now = millis();
      if (now - lastMqttReconnect > 5000) {
        lastMqttReconnect = now;
        Serial.print("Đang thử kết nối MQTT... ");
        if (client.connect(chipID.c_str())) {
          Serial.println("[OK]");
        } else {
          Serial.println("[FAIL]");
        }
      }
    } else {
      client.loop();
    }
  }

  // 3. ĐỌC DHT VÀ PUBLISH MỖI 3 GIÂY (Gửi chuỗi JSON trên 1 dòng)
  if (millis() - lastDHTRead > 3000) {
    lastDHTRead = millis();
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (!isnan(temp) && !isnan(hum) && client.connected()) {
      // Chuỗi JSON được nối thành 1 dòng duy nhất theo yêu cầu
      String payload = "{\"ID\":\"" + chipID + "\",\"temperature\":" + String(temp) + ",\"humidity\":" + String(hum) + "}";
      client.publish("esp32/dht", payload.c_str());
      Serial.println(payload);
    }
  }

  // 4. KIỂM TRA NRF24L01 TỪ STM32
  uint8_t pipeNum;
  if (radio.available(&pipeNum) && pipeNum == 0) {
    memset(RxData, 0, sizeof(RxData));
    radio.read(RxData, PAYLOAD_SIZE);

    Serial.print("[NRF -> MQTT] Received: ");
    Serial.println(RxData);

    if (client.connected()) {
      if (strstr(RxData, "ON") != NULL) {
        digitalWrite(2, HIGH);
        client.publish(topic_led_status, "ON");
      } 
      else if (strstr(RxData, "OFF") != NULL) {
        digitalWrite(2, LOW);
        client.publish(topic_led_status, "OFF");
      }
    }
  }
}