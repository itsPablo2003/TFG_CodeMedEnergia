#include <EEPROM.h>

// Pines del sistema
const int sensorPin = A0;  // Pin del SCT-013-030
const int relayPin = 3;    // Pin para el relé

// Parámetros de medición
const float voltageGrid = 230.0;
const float calibrationFactor = 50.0;
const int numSamples = 200;
const int threshold = 100;
const int numMeasurementsToAverage = 5;

// 🔹 Media móvil
const int windowSize = 2;  // Tamaño de la ventana de la media móvil
float powerWindow[windowSize] = {0};  // Almacena los últimos valores
int powerIndex = 0;  // Índice actual en la ventana
float powerSum = 0;  // Suma total para calcular el promedio
int measurementCount = 0; 

// Variables de control del relé
bool automaticMode = true;
bool relayState = HIGH;

// EEPROM
const int relayStateAddress = 0;
const int automaticModeAddress = 1;

void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);

  pinMode(relayPin, OUTPUT);

  loadRelayState();
  loadAutomaticMode();

  connectToWiFi();
  startWebServer();
}

void loop() {
  if (automaticMode) {
    float sumSquared = 0;

    for (int i = 0; i < numSamples; i++) {
      int sensorValue = analogRead(sensorPin);
      float voltage = sensorValue * (5.0 / 1023.0);
      float current = voltage * calibrationFactor;
      sumSquared += current * current;
      delayMicroseconds(100);
    }

    float rmsCurrent = sqrt(sumSquared / numSamples);
    float power = rmsCurrent * voltageGrid;

    if (rmsCurrent < 0.05) rmsCurrent = 0;
    if (power < 1) power = 0;

    // 🔹 Aplicar Media Móvil
    powerSum -= powerWindow[powerIndex];  
    powerWindow[powerIndex] = power;  
    powerSum += powerWindow[powerIndex];  
    powerIndex = (powerIndex + 1) % windowSize;  

    float smoothedPower = powerSum / windowSize;  

    Serial.print("⚡ Potencia Media Móvil: ");
    Serial.print(smoothedPower);
    Serial.println(" W");

    if (smoothedPower > threshold) {
      relayState = LOW;
      Serial.println("🔴 Relé ACTIVADO (Automático)");
    } else {
      relayState = HIGH;
      Serial.println("🟢 Relé DESACTIVADO (Automático)");
    }
  }

  digitalWrite(relayPin, relayState);

  if (measurementCount >= numMeasurementsToAverage) {
    float avgPower = powerSum / numMeasurementsToAverage;

    Serial.print("Promedio Potencia: ");
    Serial.print(avgPower);
    Serial.println(" W");

    sendDataToServer(avgPower);

    powerSum = 0;
    measurementCount = 0;
  }

  checkForCommands();
  delay(1000);
}

// 📡 Conectar a WiFi
void connectToWiFi() {
  Serial.println("🔄 Conectando a WiFi...");
  Serial1.println("AT+CWMODE=1");
  delay(1000);

  Serial1.println("AT+CWJAP=\"WIFI_MI_JDU\",\"Portatil%40%\"");  
  delay(5000);

  Serial1.println("AT+CIFSR");  
  delay(2000);

  Serial.println("✅ Conectado a WiFi");
}

// 🌍 Iniciar servidor web
void startWebServer() {
  Serial1.println("AT+CIPMUX=1");  
  delay(1000);
  Serial1.println("AT+CIPSERVER=1,80");  
  delay(2000);
  Serial.println("✅ Servidor web iniciado");
}

// 📩 Revisar comandos desde la red
void checkForCommands() {
  if (Serial1.available()) {
    String request = Serial1.readString();
    Serial.println("📩 Solicitud recibida: " + request);

    if (request.indexOf("/relay/on") != -1) {
      relayState = LOW;
      automaticMode = false;
      saveRelayState();
      saveAutomaticMode(false);
      Serial.println("🔴 Relé ACTIVADO (Manual)");
    }
    if (request.indexOf("/relay/off") != -1) {
      relayState = HIGH;
      automaticMode = false;
      saveRelayState();
      saveAutomaticMode(false);
      Serial.println("🟢 Relé DESACTIVADO (Manual)");
    }
    if (request.indexOf("/relay/auto") != -1) {
      automaticMode = true;
      saveAutomaticMode(true);
      Serial.println("🔄 Modo Automático Activado");
    }

    sendWebResponse();
  }
}

// 🖥️ Enviar respuesta web
void sendWebResponse() {
  String relayStatus = (relayState == LOW) ? "Encendido" : "Apagado";
  String automaticStatus = (automaticMode) ? "Automático" : "Manual";

  Serial1.println("AT+CIPSEND=0,200");
  delay(100);
  Serial1.println("HTTP/1.1 200 OK");
  Serial1.println("Content-Type: text/html");
  Serial1.println();
  Serial1.println("<p><a href='/relay/on'>ON</a></p>");
  Serial1.println("<p><a href='/relay/auto'>AUTO</a></p>");
  Serial1.println("<p><a href='/relay/off'>OFF</a></p>");
  Serial1.println("<p>Rele: " + relayStatus + "</p>");
  Serial1.println("<p>Modo: " + automaticStatus + "</p>");
  Serial1.println();

  delay(500);
  Serial1.println("AT+CIPCLOSE=0");
}

// 📤 Enviar datos al servidor
void sendDataToServer(float power) {
  String server = "yourserver.com";  
  String httpRequest = "GET /update?power=" + String(power) + " HTTP/1.1\r\n";
  httpRequest += "Host: " + server + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";

  Serial.println("📤 Enviando datos al servidor...");

  Serial1.println("AT+CIPSTART=\"TCP\",\"" + server + "\",80");
  delay(2000);

  if (Serial1.find("OK")) {
    Serial1.print("AT+CIPSEND=");
    Serial1.println(httpRequest.length());
    delay(1000);

    if (Serial1.find(">")) {
      Serial1.print(httpRequest);
      delay(2000);
    }

    Serial1.println("AT+CIPCLOSE");
    Serial.println("✅ Datos enviados correctamente.");
  } else {
    Serial.println("❌ Error en la conexión TCP.");
  }
}

// 🌟 Guardar el estado del relé en EEPROM
void saveRelayState() {
  EEPROM.write(relayStateAddress, relayState == LOW ? 1 : 0);  
  delay(10);  
}

// 🌟 Cargar el estado del relé desde EEPROM
void loadRelayState() {
  int state = EEPROM.read(relayStateAddress);
  relayState = (state == 1) ? LOW : HIGH;  
  digitalWrite(relayPin, relayState);
  Serial.print("Estado cargado del relé: ");
  Serial.println((relayState == LOW) ? "Encendido" : "Apagado");
}

// 🌟 Guardar el estado del modo automático/manual en EEPROM
void saveAutomaticMode(bool mode) {
  EEPROM.write(automaticModeAddress, mode ? 1 : 0);  
  delay(10);  
}

// 🌟 Cargar el estado del modo automático/manual desde EEPROM
void loadAutomaticMode() {
  int mode = EEPROM.read(automaticModeAddress);
  automaticMode = (mode == 1);  
  Serial.print("Estado cargado del modo automático: ");
  Serial.println(automaticMode ? "Automático" : "Manual");
}
