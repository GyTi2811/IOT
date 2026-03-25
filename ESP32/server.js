const express = require('express');
const aedes = require('aedes')();
const net = require('net');
const http = require('http');
const ws = require('websocket-stream');
const mysql = require('mysql2');

const MQTT_PORT = 1883;
const WS_PORT = 8080;
const WEB_PORT = 3000;

// 1. Kết nối MySQL
const db = mysql.createConnection({
    host: 'localhost', user: 'root', password: '', database: 'iot_mqtt'
});

db.connect(err => {
    if (err) console.error("❌ MySQL lỗi:", err.message);
    else console.log("✅ MySQL Connected");
});

// 2. Broker MQTT (TCP & WS)
net.createServer(aedes.handle).listen(MQTT_PORT, '0.0.0.0');
const httpServer = http.createServer();
ws.createServer({ server: httpServer, path: '/mqtt' }, aedes.handle);
httpServer.listen(WS_PORT, '0.0.0.0');

// 3. Web Dashboard
const app = express();
app.use(express.static(__dirname));
app.listen(WEB_PORT, () => console.log(`🚀 Dashboard: http://localhost:${WEB_PORT}`));

// 4. XỬ LÝ DỮ LIỆU VÀ IN CHUỖI JSON (DẠNG MỘT DÒNG)
aedes.on('publish', (packet, client) => {
    if (client) {
        const topic = packet.topic;
        const payload = packet.payload.toString();

        if (topic === "esp32/dht") {
            try {
                const data = JSON.parse(payload);
                const time = new Date().toLocaleTimeString();

                console.log(`[${time}] Received: ${JSON.stringify(data)}`);

                const device = data.ID || "ESP32";
                const temp = data.temperature;
                const hum = data.humidity;

                // Đảm bảo tên cột trong DB của bạn là device_id nhé
                const sql = "INSERT INTO sensor_data (device_id, temperature, humidity) VALUES (?, ?, ?)";
                
                db.query(sql, [device, temp, hum], (err, result) => {
                    if (err) {
                        console.error(`❌ Lỗi lưu DB: ${err.sqlMessage}`);
                    } else {
                        // In ra báo lưu thành công ID nào
                        console.log(`💾 MySQL: Đã lưu dữ liệu của [${device}]`);
                    }
                });

            } catch (e) {
                console.log(`⚠️ Nhận dữ liệu lỗi (không phải JSON): ${payload}`);
            }
        }
    }
});