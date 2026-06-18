#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

// --- CONFIGURACIÓN WIFI ---
const char* ssid     = "NOMBRE DE TU RED WIFI";
const char* password = "CLAVE DE TU RED WIFI";

// --- SENSORES ---
#define DHTPIN  32
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#define SOIL_PIN 34

// --- RELÉS ---
int pines_salida[] = {5, 18, 19, 23, 33, 27, 26, 25};
#define NUM_RELES 8

WebServer server(80);

// ─────────────────────────────────────────
//  CORS: se llama al inicio de cada handler
//  para que el navegador acepte la respuesta
// ─────────────────────────────────────────
void setCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// --- HANDLER: raíz ---
void handleRoot() {
  setCORS();
  server.send(200, "text/html",
    "<h2>TERRAMIND ESP32</h2>"
    "<p><a href='/sensores'>/sensores</a> — leer sensores</p>"
    "<p>/set?pin=5&estado=1 — controlar relé</p>");
}

// --- HANDLER: OPTIONS (preflight del navegador) ---
void handleOptions() {
  setCORS();
  server.send(204);
}

// --- HANDLER: /sensores ---
void handleSensors() {
  float h      = dht.readHumidity();
  float t      = dht.readTemperature();
  int rawSuelo = analogRead(SOIL_PIN);

  // Porcentaje: ajusta 4095 (seco) y 2300 (húmedo) según tu sensor
  int percSuelo = map(rawSuelo, 4095, 2337, 0, 100);
  percSuelo = constrain(percSuelo, 0, 100);

  if (isnan(h) || isnan(t)) { h = 0; t = 0; }

  // --- Leer estado de todos los relés ---
  String relaysJson = "{";
  for (int i = 0; i < NUM_RELES; i++) {
    int pin = pines_salida[i];
    int estado = digitalRead(pin);
    relaysJson += "\"relay_" + String(pin) + "\":" + String(estado);
    if (i < NUM_RELES - 1) relaysJson += ",";
  }
  relaysJson += "}";

  // --- Construir JSON completo ---
  String json = "{";
  json += "\"temp\":"             + String(t, 1)      + ",";
  json += "\"hum\":"              + String(h, 1)      + ",";
  json += "\"suelo_porcentaje\":" + String(percSuelo) + ",";
  json += "\"suelo_raw\":"        + String(rawSuelo)  + ",";
  json += "\"relays\":"           + relaysJson;
  json += "}";

  setCORS();
  server.send(200, "application/json", json);
}

// --- HANDLER: /set?pin=N&estado=0|1 ---
void handleSet() {
  if (server.hasArg("pin") && server.hasArg("estado")) {
    int pin    = server.arg("pin").toInt();
    int estado = server.arg("estado").toInt();

    // Validar que el pin sea uno de los registrados
    bool valido = false;
    for (int i = 0; i < NUM_RELES; i++) {
      if (pines_salida[i] == pin) { valido = true; break; }
    }

    if (valido) {
      digitalWrite(pin, estado ? HIGH : LOW);
      Serial.printf("Relé pin %d → %s\n", pin, estado ? "ON" : "OFF");
      setCORS();
      server.send(200, "application/json",
        "{\"pin\":" + String(pin) + ",\"estado\":" + String(estado) + "}");
    } else {
      setCORS();
      server.send(400, "text/plain", "Pin no permitido");
    }
  } else {
    setCORS();
    server.send(400, "text/plain", "Faltan parametros");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  for (int i = 0; i < NUM_RELES; i++) {
    pinMode(pines_salida[i], OUTPUT);
    digitalWrite(pines_salida[i], LOW);
  }

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/",        HTTP_GET,     handleRoot);
  server.on("/sensores",HTTP_GET,     handleSensors);
  server.on("/set",     HTTP_GET,     handleSet);

  // Preflight CORS para todas las rutas
  server.on("/",         HTTP_OPTIONS, handleOptions);
  server.on("/sensores", HTTP_OPTIONS, handleOptions);
  server.on("/set",      HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("Servidor listo en http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();

  // Reconectar WiFi si se cae
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi caido, reconectando...");
    WiFi.reconnect();
    delay(5000);
  }
}
