// demo.js - Đã TÁCH đồ thị & cập nhật style Cockpit
let mqttClient;
// TÁCH thành 2 biến đồ thị độc lập
let tempGauge, humGauge, tempChart, humChart;

// Định nghĩa màu sắc neon từ style.css
const COLOR_CYAN_TEMP = "#00ffff";
const COLOR_GREEN_HUM = "#33ff33";
const COLOR_GRID_DARK = "#30363d";
const COLOR_TEXT_MAIN = "#c9d1d9";

// --- 1. Khởi tạo Giao diện khi trang web tải xong ---
window.onload = function () {
    initGaugesCockpitStyle(); // Gọi hàm gauge đã cập nhật style tối
    initSplitCharts(); // Khởi tạo 2 đồ thị riêng biệt
};

// Cập nhật Gauges màu tối kiểu kĩ thuật
function initGaugesCockpitStyle() {
    tempGauge = new RadialGauge({
        renderTo: 'tempGauge', width: 200, height: 200, units: "°C",
        minValue: 0, maxValue: 50, majorTicks: ["0", "10", "20", "30", "40", "50"],
        highlights: [{ "from": 35, "to": 50, "color": "rgba(255, 0, 0, 0.6)" }], // Đỏ cảnh báo tối
        needleType: "arrow", animationRule: "bounce", animationDuration: 1500,
        // Dark theme colors
        colorPlate: "#161b22", colorMajorTicks: COLOR_TEXT_MAIN, colorMinorTicks: "#555",
        colorNumbers: COLOR_TEXT_MAIN, colorUnits: COLOR_CYAN_TEMP, colorTitle: COLOR_CYAN_TEMP,
        colorNeedle: "#ff9900", colorNeedleEnd: "#ff9900", colorNeedleCircleOuter: "#ccc",
        borderShadowWidth: 0, borders: false,
        fontNumbersSize: 22, fontUnitsSize: 25, fontTitleSize: 25,
        colorLabel: COLOR_CYAN_TEMP
    }).draw();

    humGauge = new RadialGauge({
        renderTo: 'humGauge', width: 200, height: 200, units: "%",
        minValue: 0, maxValue: 100, majorTicks: ["0", "20", "40", "60", "80", "100"],
        highlights: [{ "from": 70, "to": 100, "color": "rgba(0, 100, 255, 0.6)" }], // Xanh lam ướt tối
        needleType: "arrow", animationRule: "bounce", animationDuration: 1500,
        // Dark theme colors
        colorPlate: "#161b22", colorMajorTicks: COLOR_TEXT_MAIN, colorMinorTicks: "#555",
        colorNumbers: COLOR_TEXT_MAIN, colorUnits: COLOR_GREEN_HUM, colorTitle: COLOR_GREEN_HUM,
        colorNeedle: "#ff9900", colorNeedleEnd: "#ff9900", colorNeedleCircleOuter: "#ccc",
        borderShadowWidth: 0, borders: false,
        fontNumbersSize: 22, fontUnitsSize: 25, fontTitleSize: 25,
        colorLabel: COLOR_GREEN_HUM
    }).draw();
}

// Cấu hình chung cho Đồ thị kiểu Cockpit
const getChartConfig = (ctx, label, color, yMin, yMax, yStep) => {
    return new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: label,
                data: [],
                borderColor: color,
                backgroundColor: `${color}1A`, // Thêm fill phát sáng mờ
                fill: true,
                tension: 0.4,
                pointRadius: 2,
                pointBackgroundColor: color,
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                y: {
                    beginAtZero: false,
                    min: yMin,
                    max: yMax,
                    ticks: { color: "#888", font: { family: 'Roboto Mono' }, stepSize: yStep },
                    grid: { color: COLOR_GRID_DARK }
                },
                x: {
                    ticks: { color: "#888", font: { family: 'Roboto Mono' }, maxRotation: 0 },
                    grid: { color: COLOR_GRID_DARK }
                }
            },
            plugins: {
                legend: {
                    labels: { color: COLOR_TEXT_MAIN, font: { family: 'Orbitron', weight: 'bold' } }
                }
            },
            animation: { duration: 0 } // Update tức thì để trông giống oscilloscope kĩ thuật
        }
    });
};

function initSplitCharts() {
    const ctxTemp = document.getElementById('tempChart').getContext('2d');
    const ctxHum = document.getElementById('humChart').getContext('2d');

    // Khởi tạo 2 đồ thị riêng biệt với màu sắc và trục Y phù hợp
    tempChart = getChartConfig(ctxTemp, 'Nhiệt độ (°C)', COLOR_CYAN_TEMP, 20, 45, 5);
    humChart = getChartConfig(ctxHum, 'Độ ẩm (%)', COLOR_GREEN_HUM, 30, 95, 10);
}

// --- 2. Kết nối MQTT (Giữ nguyên logic của bạn) ---
function connectMQTT() {
    const host = document.getElementById("host").value;
    const port = Number(document.getElementById("port").value);
    const clientId = "WebCockpit_" + Math.random().toString(16).substr(2, 5);

    mqttClient = new Paho.MQTT.Client(host, port, "/mqtt", clientId);

    mqttClient.onMessageArrived = (message) => {
        updateUI(message.payloadString);
    };

    mqttClient.connect({
        onSuccess: () => {
            console.log("✅ Cockpit connected to Broker");
            mqttClient.subscribe("esp32/dht"); // Chỉ subscribe DHT, led topic dùng cho điều khiển
            alert("✅ Hệ thống đã kết nối!");
        },
        onFailure: (err) => { alert("Lỗi kết nối MQTT: " + err.errorMessage); }
    });
}

function disconnectMQTT() {
    if (mqttClient && mqttClient.connected) {
        mqttClient.disconnect();
        alert("⚠️ Đã ngắt kết nối.");
    }
}

// --- 3. Cập nhật dữ liệu lên màn hình (Đã sửa logic tách đồ thị) ---
function updateUI(payload) {
    try {
        const data = JSON.parse(payload);
        const t = parseFloat(data.temperature).toFixed(1);
        const h = parseFloat(data.humidity).toFixed(1);
        const deviceId = data.ID || "ESP32_DEV";
        const now = new Date().toLocaleTimeString();

        // HIỂN THỊ ID LÊN NAVBAR PANEL
        document.getElementById("device_id_display").innerText = deviceId;

        // Cập nhật số và Gauges kiểu cockit
        document.getElementById("temp_display").innerText = t + " °C";
        document.getElementById("hum_display").innerText = h + " %";
        tempGauge.value = t;
        humGauge.value = h;

        // CẬP NHẬT 2 ĐỒ THỊ RIÊNG BIỆT (Giữ tối đa 15 điểm)
        const updateChart = (chart, value) => {
            if (chart.data.labels.length > 15) {
                chart.data.labels.shift();
                chart.data.datasets[0].data.shift();
            }
            chart.data.labels.push(now);
            chart.data.datasets[0].data.push(value);
            chart.update(); // Update tức thì kiểu oscilloscope kĩ thuật
        };

        // Đẩy giá trị t sang tempChart, h sang humChart
        updateChart(tempChart, t);
        updateChart(humChart, h);

        // Log tin nhắn kiểu kĩ thuật (Dán cả ID vào đầu)
        const log = document.getElementById("mqtt_messages");
        log.innerHTML = `<div>[${now}] <span style="color:${COLOR_CYAN_TEMP}">${deviceId}</span>: T=${t}, H=${h}</div>` + log.innerHTML;
        // Giới hạn log box để không bị overload
        if (log.children.length > 50) log.lastChild.remove();

    } catch (e) { console.error("Telemetry data error:", e, payload); }
}

// --- 4. Điều khiển LED (Giữ nguyên của bạn) ---
function ON_LED() { sendCommand("esp32/led", "ON"); }
function OFF_LED() { sendCommand("esp32/led", "OFF"); }

function sendCommand(topic, msg) {
    if (mqttClient && mqttClient.connected) {
        let message = new Paho.MQTT.Message(msg);
        message.destinationName = topic;
        mqttClient.send(message);
    } else {
        alert("⚠️ Hệ thống chưa kết nối MQTT!");
    }
}