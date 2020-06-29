#include <Arduino.h>
#include <IRremote.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// IR
int REC_PIN = 14;
IRrecv irrecv(REC_PIN);
decode_results results;

int SEND_PIN = 5;
IRsend irsend(SEND_PIN);

// Wifi
const char *ssid      = "RED";
const char *password  = "36719940CPTH";

// Mqtt 
WiFiClient espClient;
IPAddress server(192, 168, 1, 8);
PubSubClient client(espClient);
#define SENDTOPIC "IR/key"
#define COMMANDTOPIC "IR/command"
#define SERVICETOPIC "IR/service"
// Note: Need mosquito broker running
void setup_wifi() {
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wifi conectado");
}

void send_code(unsigned long code) {
  Serial.print("Enviando codigo");
  for (int i = 0; i < 3; i++) {
    irsend.sendNEC(code, 32);
    delay(40);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Ha llegado un mensaje topic: [");
  Serial.print(topic);
  Serial.println("]");
  Serial.println();

  StaticJsonDocument<256> data;
  deserializeJson(data, payload, length);
  unsigned long code = data["value"];

  Serial.print("El codigo es ");
  Serial.println(code, HEX);
  
  send_code(code);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tratando de conectarse al servidor MQTT...");
    if (client.connect("ESP32")) {
      Serial.println("Conectado");
      client.publish(SERVICETOPIC, "Esp Conectado");
    } else {
      Serial.println("No se ha podido conectar");
      Serial.println("Volviendo a conectar en 5 segundos");
      delay(5000);
    }
  }
}

void setup_mqtt() {
  client.setServer(server, 1883);
  client.setCallback(callback);
  Serial.println("Conectando al servidor MQTT");
  if (client.connect("Esp32")) {
    Serial.println("Conexion exitosa");
    client.publish(SERVICETOPIC, "Esp32 Conectado");
    client.subscribe(COMMANDTOPIC);
  } else {
    Serial.println("Conexion fallida");
    reconnect();
  }
}

String get_protocol(int codeType) {
  Serial.print("Protocolo recibido: ");
  Serial.println(codeType);
  switch (codeType) {
    case UNKNOWN:
      Serial.println("UNKNOWN");
      return "UNKNOWN";
    case NEC:
      Serial.println("NEC");
      return "NEC";
    case SONY:
      Serial.println("SONY");
      return "SONY";
    case PANASONIC:
      Serial.println("PANASONIC");
      return "PANASONIC";
    case JVC:
      Serial.println("JVC");
      return "JVC";
    case RC5:
      Serial.println("RC5");
      return "RC5";
    case RC6:
      Serial.println("RC6");
      return "RC6";
    default:
      Serial.print("No fue posible reconocer un protocolo. codeType: ");
      Serial.println(codeType, DEC);
      return "UNRECOGNIZABLE";
  }
}

void storeCode(decode_results *results) {
  if (results->value == REPEAT) {
    Serial.println("Ignorando codigo repetido");
    return;
  }
  String protocol = get_protocol(results->decode_type);
  Serial.println("Publishing to service topic");
  if (protocol == "UNRECOGNIZABLE") {
    client.publish(SERVICETOPIC, "UNRECOGNIZABLE");
  } else if (protocol == "UNKNOWN") {
    Serial.println("Codigo desconocido, guardando como raw");
    StaticJsonDocument<200> data;
    data["type"] = "UNKNOWN";
    data["value"] = "";
    data["length"] = "";
    char buffer[512];
    size_t n = serializeJson(data, buffer);
    client.publish(SERVICETOPIC, buffer, n);
  } else {
    StaticJsonDocument<200> data;
    data["type"] = protocol;
    data["value"] = results->value;
    data["length"] = results->bits;
    char buffer[512];
    size_t n = serializeJson(data, buffer);
    client.publish(SERVICETOPIC, buffer, n);
  }
}

void setup() {
  Serial.begin(9600);
  setup_wifi();
  setup_mqtt();
  irrecv.enableIRIn();
}

void loop() {
  if (irrecv.decode(&results)) {
    digitalWrite(BUILTIN_LED, HIGH);
    storeCode(&results);
    Serial.println(results.value, HEX);
    irrecv.resume();
    digitalWrite(BUILTIN_LED, LOW);
  }
  client.loop();
}