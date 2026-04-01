const express = require('express');
const aedes = require('aedes')();
const net = require('net');
const http = require('http');
const ws = require('websocket-stream');
const mysql = require('mysql2');

const MQTT_PORT = 1883;
const WS_PORT = 8080;
const WEB_PORT = 3000;

// ===== MYSQL =====
const db = mysql.createConnection({
    host: 'localhost',
    user: 'root',
    password: '',
    database: 'iot_mqtt'
});

db.connect(err => {
    if (err) console.log("❌ MySQL lỗi:", err);
    else console.log("✅ MySQL OK");
});

// ===== MQTT =====
net.createServer(aedes.handle).listen(MQTT_PORT, () => {
    console.log("🚀 MQTT TCP chạy cổng 1883");
});

// ===== WebSocket MQTT =====
const httpServer = http.createServer();
ws.createServer({ server: httpServer, path: '/mqtt' }, aedes.handle);

httpServer.listen(WS_PORT, () => {
    console.log("🚀 MQTT WS chạy cổng 8080");
});

// ===== WEB =====
const app = express();
app.use(express.static(__dirname));

app.listen(WEB_PORT, () => {
    console.log("🌐 Web: http://localhost:3000");
});

// ===== HANDLE DATA =====
aedes.on('publish', (packet, client) => {
    if (!client) return;

    const topic = packet.topic;
    const payload = packet.payload.toString();

    if (topic === "esp32/dht") {

        console.log("RAW:", payload);

        let data;
        try {
            data = JSON.parse(payload);
        } catch {
            console.log("❌ JSON lỗi");
            return;
        }

        if (!data.temperature || !data.humidity) {
            console.log("⚠️ thiếu data");
            return;
        }

        const sql = `
            INSERT INTO sensor_data (device_id, temperature, humidity)
            VALUES (?, ?, ?)
        `;

        db.query(sql,
            [data.ID, data.temperature, data.humidity],
            (err) => {
                if (err) console.log("❌ DB lỗi:", err);
                else console.log("💾 Saved:", data.ID);
            }
        );
    }
});