const express = require("express");
const mqtt = require('mqtt')
const fs = require('fs');
const app = express();
app.use(express.json());
app.use(express.static("public"));
const client  = mqtt.connect('mqtt://192.168.1.8:1883')
client.on('connect', function () {
  client.subscribe('IR/command', function (err) {
    if (err) throw err;
    console.log("Connection stablished");
  })
  client.subscribe('IR/service', function (err) {
    if (err) throw err;
    console.log("Connection stablished");
  })
})
client.on('message', function (topic, message) {
  // message is Buffer
  console.log("New Message: " + message.toString())
})


app.get('/toggle', (req, res) => {
  client.publish('IR/command', '{"type":"3",  "value": "551489775","length":"32"}');
  res.status(200).json({"msg": "Sending code"});
})

app.post('/device/create', (req, res) => {
  
})

app.listen(3000, () => console.log("server running"));