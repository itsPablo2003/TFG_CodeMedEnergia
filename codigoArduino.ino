#include <EEPROM.h>

// ---------------- Configuración ----------------
const int sensorPin = A0;
const int relayPin = 3;
const float voltageGrid = 230.0;
const float calibrationFactor = 75.0;
const int numSamples = 100;

const bool RELAY_ACTIVE_HIGH = true;

// WiFi
const char* ssid = "WIFI_MI_JDU";
const char* password = "Portatil%40%";

// ThingSpeak
const String writeAPIKey = "FXIREIGQPMWWB6HA";
const String readAPIKey = "YKGWOG0558VZWTBG";
const String channelID = "2974072";

// EEPROM
const int relayStateAddress = 0;

bool relayState = LOW;
unsigned long lastSend = 0;
unsigned long lastRead = 0;
const unsigned long sendInterval = 60000;
const unsigned long readInterval = 10000;

const int windowSize = 30;
float powerWindow[windowSize] = {0};
int powerIndex = 0;
float powerSum = 0;
float lastValidPower = 0;
float lastSentPower = -1;
const float spikeThreshold = 20.0;
const float sendThreshold = 10.0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  pinMode(relayPin, OUTPUT);
  loadRelayState();
  connectToWiFi();
}

void loop() {
  float sumSquared = 0;
  for (int i = 0; i < numSamples; i++) {
    float voltage = analogRead(sensorPin) * (5.0 / 1023.0);
    float current = voltage * calibrationFactor;
    sumSquared += current * current;
    delayMicroseconds(100);
  }

  float rmsCurrent = sqrt(sumSquared / numSamples);
  float power = rmsCurrent * voltageGrid;

  if (power < 10) power = 0;
  if (abs(power - lastValidPower) > spikeThreshold) {
    power = lastValidPower;
  } else {
    lastValidPower = power;
  }

  powerSum -= powerWindow[powerIndex];
  powerWindow[powerIndex] = power;
  powerSum += powerWindow[powerIndex];
  powerIndex = (powerIndex + 1) % windowSize;
  float smoothedPower = powerSum / windowSize;

  // Solo enviamos si el valor cambió significativamente o ha pasado suficiente tiempo
  if (millis() - lastSend >= sendInterval && abs(smoothedPower - lastSentPower) > sendThreshold) {
    sendToThingSpeak(smoothedPower);
    lastSentPower = smoothedPower;
    lastSend = millis();
  }

  if (millis() - lastRead >= readInterval) {
    updateRelayFromThingSpeak();
    lastRead = millis();
  }

  digitalWrite(relayPin, RELAY_ACTIVE_HIGH ? relayState : !relayState);
}

// ----------------- WiFi -----------------
void connectToWiFi() {
  Serial.println("🔄 Conectando a WiFi...");
  sendAT("AT+CWMODE=1", "OK", 5000);
  sendAT("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(password) + "\"", "WIFI GOT IP", 10000);
}

// ----------------- Envío de datos -----------------
void sendToThingSpeak(float power) {
  String url = "/update?api_key=" + writeAPIKey + "&field1=" + String(power, 2);

  sendAT("AT+CIPCLOSE", "CLOSED", 1000);
  if (!sendAT("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", "CONNECT", 5000)) return;

  String req = "GET " + url + " HTTP/1.1\r\n";
  req += "Host: api.thingspeak.com\r\n";
  req += "Connection: close\r\n\r\n";

  sendAT("AT+CIPSEND=" + String(req.length()), ">", 3000);
  Serial1.print(req);
  sendAT("AT+CIPCLOSE", "OK", 1000);
}

// ----------------- Lectura del rele -----------------
void updateRelayFromThingSpeak() {
  String url = "/channels/" + channelID + "/fields/2/last.txt?api_key=" + readAPIKey;

  sendAT("AT+CIPCLOSE", "CLOSED", 1000);
  if (!sendAT("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", "CONNECT", 5000)) return;

  String req = "GET " + url + " HTTP/1.1\r\n";
  req += "Host: api.thingspeak.com\r\n";
  req += "Connection: close\r\n\r\n";

  sendAT("AT+CIPSEND=" + String(req.length()), ">", 3000);
  Serial1.print(req);

  String resp = "";
  unsigned long start = millis();
  while (millis() - start < 3000) {
    while (Serial1.available()) {
      char c = Serial1.read();
      resp += c;
      if (resp.endsWith("CLOSED")) break;
    }
    if (resp.endsWith("CLOSED")) break;
  }

  Serial.println("📥 Respuesta THINGSPEAK:");
  Serial.println(resp);

  int idx = resp.lastIndexOf("CLOSED");
  if (idx > 0) {
    char lastChar = resp.charAt(idx - 1);
    if (isDigit(lastChar)) {
      int estado = lastChar - '0';
      relayState = (estado == 1) ? HIGH : LOW;
      Serial.print("🌐 Rele desde ThingSpeak: ");
      Serial.println(estado);
      saveRelayState();
      return;
    }
  }

  Serial.println("⚠️ No se pudo leer el valor del relé");
}

// ----------------- AT Commands -----------------
bool sendAT(String cmd, String expect, int timeout) {
  Serial1.println(cmd);
  Serial.print("📡 Enviado: ");
  Serial.println(cmd);
  long t = millis();
  String r = "";

  while (millis() - t < timeout) {
    while (Serial1.available()) {
      char c = Serial1.read();
      r += c;
      Serial.write(c);
    }
    if (r.indexOf(expect) != -1) return true;
  }
  Serial.println("⚠️ No se recibió respuesta esperada.");
  return false;
}

// ----------------- EEPROM -----------------
void saveRelayState() {
  EEPROM.write(relayStateAddress, relayState ? 1 : 0);
  delay(10);
}

void loadRelayState() {
  relayState = EEPROM.read(relayStateAddress) == 1;
  digitalWrite(relayPin, RELAY_ACTIVE_HIGH ? relayState : !relayState);
}
