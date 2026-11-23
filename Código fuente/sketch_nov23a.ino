#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <math.h>

// =====================================
// 1. CONFIGURACION WIFI
// =====================================

const char* ssid = "Invernadero-ESP32";
const char* password = "invernadero123";

// =====================================
// 2. PINES DEL ESP32
// =====================================

#define DHT_PIN 21
#define DHT_TYPE DHT22

#define SOIL_PIN 34
#define LDR_PIN 35

// =====================================
// 3. OBJETOS
// =====================================

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

// =====================================
// 4. VARIABLES DE MEDICION
// =====================================

float temperature = NAN;
float humidity = NAN;

int LDR = 0;
int soilRaw = 0;

// Temporizadores
unsigned long lastRead = 0;
unsigned long lastIP = 0;

const unsigned long interval = 2000;
const unsigned long intervalIP = 10000;

// =====================================
// 5. PAGINA WEB
// =====================================

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">

<head>
<meta charset="UTF-8">
<meta name="viewport"
      content="width=device-width, initial-scale=1">

<title>Smart Greenhouse</title>

<style>
body {
  font-family: Arial, sans-serif;
  background: #101c18;
  color: white;
  text-align: center;
  margin: 0;
  padding: 20px;
}

h1 {
  color: #62d98b;
}

.subtitle {
  color: #aabbb1;
}

.container {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 15px;
  max-width: 600px;
  margin: 25px auto;
}

.card {
  background: #1d3028;
  border-radius: 15px;
  padding: 22px 10px;
}

.label {
  color: #aabbb1;
  font-size: 14px;
}

.value {
  font-size: 30px;
  font-weight: bold;
  margin: 15px 0;
  color: #62d98b;
}

.unit {
  font-size: 14px;
}

.status {
  color: #62d98b;
  font-size: 13px;
}

.footer {
  color: #aabbb1;
  font-size: 12px;
  margin-top: 25px;
}

@media(max-width: 350px) {
  .container {
    grid-template-columns: 1fr;
  }
}
</style>

</head>

<body>

<h1>🌱 SMART GREENHOUSE</h1>

<p class="subtitle">
Sistema inteligente de monitoreo
</p>

<p class="status" id="status">
Conectando...
</p>

<div class="container">

  <div class="card">
    <div class="label">
      🌡️ Temperatura
    </div>

    <div class="value" id="temp">--</div>
    <div class="unit">°C</div>
  </div>

  <div class="card">
    <div class="label">
      💧 Humedad ambiental
    </div>

    <div class="value" id="humidity">--</div>
    <div class="unit">%</div>
  </div>

  <div class="card">
    <div class="label">
      🌿 Humedad del suelo
    </div>

    <div class="value" id="soil">--</div>
    <div class="unit">
      Lectura ADC (0-4095)
    </div>
  </div>

  <div class="card">
    <div class="label">
      ☀️ Iluminación
    </div>

    <div class="value" id="LDR">--</div>
    <div class="unit">
      Lectura ADC (0-4095)
    </div>
  </div>

</div>

<div class="footer">
Actualizacion automatica cada 2 segundos
<br>
ESP32 - Invernadero inteligente
</div>

<script>

async function updateData() {

  try {

    const response = await fetch('/data');

    if (!response.ok) {
      throw new Error('Error HTTP');
    }

    const data = await response.json();

    document.getElementById('temp').textContent =
      data.temperature === null
        ? '--'
        : data.temperature;

    document.getElementById('humidity').textContent =
      data.humidity === null
        ? '--'
        : data.humidity;

    document.getElementById('LDR').textContent =
      data.LDR === null
        ? '--'
        : data.LDR;

    document.getElementById('soil').textContent =
      data.soil;

    document.getElementById('status').textContent =
      '● Sistema conectado';

  } catch(error) {

    document.getElementById('status').textContent =
      'Sin comunicacion con ESP32';

  }
}

setInterval(updateData, 2000);
updateData();

</script>

</body>
</html>
)rawliteral";

// =====================================
// 6. LEER SENSORES
// =====================================

void readSensors() {

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  LDR = analogRead(LDR_PIN);
  soilRaw = analogRead(SOIL_PIN);

}

// =====================================
// 7. ENVIAR DATOS EN JSON
// =====================================

void handleData() {

  String json = "{";

  // Temperatura
  json += "\"temperature\":";

  if (isnan(temperature)) {
    json += "null";
  } else {
    json += String(temperature, 1);
  }

  // Humedad ambiental
  json += ",\"humidity\":";

  if (isnan(humidity)) {
    json += "null";
  } else {
    json += String(humidity, 1);
  }

  // LDR - CORREGIDO
  json += ",\"LDR\":";
  json += String(LDR);

  // Humedad del suelo
  json += ",\"soil\":";
  json += String(soilRaw);

  json += "}";

  server.send(200, "application/json", json);

}

// =====================================
// 8. MOSTRAR DIRECCION IP
// =====================================

void mostrarIP() {

  Serial.println();
  Serial.println("==============================");
  Serial.println("INFORMACION DEL INVERNADERO");
  Serial.println("==============================");

  Serial.print("Red WiFi: ");
  Serial.println(ssid);

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("Direccion web: http://");
  Serial.println(WiFi.softAPIP());

  Serial.println("==============================");
  Serial.println();

}

// =====================================
// 9. CONFIGURACION INICIAL
// =====================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("INICIANDO SMART GREENHOUSE...");

  // Configurar entradas analogicas
  analogReadResolution(12);

  analogSetPinAttenuation(SOIL_PIN, ADC_11db);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  // Iniciar DHT22
  dht.begin();

  // Crear red WiFi propia
  WiFi.mode(WIFI_AP);

  bool wifiOK = WiFi.softAP(ssid, password);

  if (wifiOK) {

    Serial.println("Red WiFi creada correctamente");

    mostrarIP();

  } else {

    Serial.println("ERROR: No se pudo crear la red WiFi");

  }

  // Configurar rutas del servidor web
  server.on("/", []() {

    server.send_P(
      200,
      "text/html",
      webpage
    );

  });

  server.on("/data", handleData);

  // Iniciar servidor HTTP
  server.begin();

  Serial.println("Servidor web iniciado");

  // Primera lectura
  readSensors();

  lastRead = millis();
  lastIP = millis();

}

// =====================================
// 10. CICLO PRINCIPAL
// =====================================

void loop() {

  // Atender peticiones del navegador
  server.handleClient();

  unsigned long ahora = millis();

  // Leer sensores cada 2 segundos
  if (ahora - lastRead >= interval) {

    lastRead = ahora;

    readSensors();

    Serial.println();
    Serial.println("----- MEDICIONES -----");

    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.println(" C");

    Serial.print("Humedad ambiental: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Luz ambiental ADC: ");
    Serial.println(LDR);

    Serial.print("Humedad suelo ADC: ");
    Serial.println(soilRaw);

    Serial.println("----------------------");

  }

  // Mostrar IP cada 10 segundos
  if (ahora - lastIP >= intervalIP) {

    lastIP = ahora;

    Serial.print("[WEB] Direccion IP: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("[WEB] Abrir: http://");
    Serial.println(WiFi.softAPIP());

  }

}