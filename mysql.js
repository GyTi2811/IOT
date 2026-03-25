const mqtt = require('mqtt');
const mysql = require('mysql2');

const db = mysql.createConnection({
host:'localhost',
user:'root',
password:'',
database:'iot_mqtt'
});

db.connect(err=>{
if(err) throw err;
console.log("MySQL connected");
});

const client = mqtt.connect({
host:'192.168.31.120',
port:1883,
username:'GiaThanh',
password:'123'
});

client.on('connect',()=>{

console.log("MQTT connected");

client.subscribe('esp32/dht');

});

client.on('message',(topic,message)=>{

let payload = message.toString();

console.log(topic,payload);

if(topic==="esp32/dht"){

let data = JSON.parse(payload);

let temp = data.temperature;
let hum  = data.humidity;

let sql = "INSERT INTO sensor_data (temperature,humidity) VALUES (?,?)";

db.query(sql,[temp,hum],(err,result)=>{

if(err) throw err;

console.log("Saved:",temp,hum);

});

}

});