#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// ========= CONFIG Wi-Fi =========
const char* ssid     = "Igor Carneiro";
const char* password = "igorcarneiro";

// ========= SENSOR =========
#define DHTPIN   4
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);

const unsigned long SAMPLE_MS   = 2000;  // intervalo de leitura (ms)
const float HUM_THRESHOLD       = 70.0;  // alerta de umidade (%)

// ========= LCD I2C =========
// Endereço mais comum: 0x27 (alguns módulos usam 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========= WEB =========
WebServer server(80);

// ========= Estado =========
unsigned long lastSample = 0;
float lastTemp = NAN, lastHum = NAN;

// ---------- HTML (página com atualização automática) ----------
const char HTML_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="pt-br">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 - DHT22</title>
<style>
  body{font-family:system-ui,Arial;margin:20px}
  .card{border:1px solid #ddd;border-radius:12px;padding:16px;max-width:420px}
  h1{font-size:1.2rem;margin:0 0 12px}
  .val{font-size:2rem;margin:6px 0}
  .ok{color:#2e7d32}
  .warn{color:#c62828;font-weight:600}
  .muted{color:#777}
  .row{display:flex;gap:16px}
</style>
<div class="card">
  <h1>Monitor de Temperatura/Umidade</h1>
  <div class="row">
    <div>
      <div class="muted">Temperatura</div>
      <div id="t" class="val">--.- °C</div>
    </div>
    <div>
      <div class="muted">Umidade</div>
      <div id="h" class="val">--.- %</div>
    </div>
  </div>
  <div id="alerta" class="muted">Lendo...</div>
  <div class="muted" style="margin-top:10px" id="upd">—</div>
</div>
<script>
async function tick(){
  try{
    const r = await fetch('/data');
    const j = await r.json();
    document.getElementById('t').textContent = (j.temp===null? "--.-" : j.temp.toFixed(1)) + " °C";
    document.getElementById('h').textContent = (j.hum===null?  "--.-" : j.hum.toFixed(1))  + " %";
    const a = document.getElementById('alerta');
    if(j.hum_excesso){
      a.textContent = "ALERTA: Umidade excessiva!";
      a.className = "warn";
    }else{
      a.textContent = "Umidade dentro do esperado.";
      a.className = "ok";
    }
    document.getElementById('upd').textContent = "Atualizado: " + new Date().toLocaleTimeString();
  }catch(e){
    document.getElementById('alerta').textContent = "Erro de leitura...";
  }
}
setInterval(tick, 2000);
tick();
</script>
</html>
)HTML";

// ---------- Atualiza o LCD com os últimos valores ----------
void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T: ");
  if (!isnan(lastTemp)) {
    lcd.print(lastTemp, 1);
    lcd.print((char)223); // símbolo de grau
    lcd.print("C");
  } else {
    lcd.print("--.-");
  }

  lcd.setCursor(0, 1);
  lcd.print("U: ");
  if (!isnan(lastHum)) {
    lcd.print(lastHum, 1);
    lcd.print("% ");
    if (lastHum >= HUM_THRESHOLD) {
      lcd.print("ALTA"); // alerta no final da linha
    } else {
      lcd.print("OK  ");
    }
  } else {
    lcd.print("--.-");
  }
}

// ---------- Rota raiz ----------
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", HTML_PAGE);
}

// ---------- Rota de dados JSON ----------
void handleData() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    lastHum  = dht.readHumidity();
    lastTemp = dht.readTemperature(); // °C
    updateLCD();
  }

  bool ok = !(isnan(lastHum) || isnan(lastTemp));
  bool humExcesso = ok && (lastHum >= HUM_THRESHOLD);

  String json = "{";
  if (ok) {
    json += "\"temp\":" + String(lastTemp, 2) + ",";
    json += "\"hum\":"  + String(lastHum, 2)  + ",";
    json += "\"hum_excesso\":" + String(humExcesso ? "true":"false");
  } else {
    json += "\"temp\":null,\"hum\":null,\"hum_excesso\":false";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // I2C + LCD
  Wire.begin(21, 22);   // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Iniciando...");
  delay(800);

  // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Wi-Fi conectando");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConectado! IP: " + WiFi.localIP().toString());
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("IP:");
  lcd.setCursor(0,1); lcd.print(WiFi.localIP().toString());

  // Rotas
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
}
