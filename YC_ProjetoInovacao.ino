/********* ESP32 + DHT22 + LCD I2C + Web + BUZZER (bipes intermitentes) *********
 * DHT22: VCC->3V3, GND->GND, DATA->GPIO 4 + resistor 10k para 3V3
 * LCD I2C (PCF8574): VCC->5V, GND->GND, SDA->GPIO 21, SCL->GPIO 22 (end 0x27)
 * BUZZER: + -> GPIO 25, - -> GND
 *******************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// ========= CONFIG Wi-Fi =========
const char* ssid     = "Anão";
const char* password = "Ana@140711";

// ========= SENSOR =========
#define DHTPIN   4
#define DHTTYPE  DHT22
DHT dht(DHTPIN, DHTTYPE);

const unsigned long SAMPLE_MS = 2000;   // intervalo de leitura (ms)
const float HUM_ON  = 70.0;             // liga alerta quando >= 70%
const float HUM_OFF = 68.0;             // desliga alerta quando <= 68% (histerese)

// ========= LCD I2C =========
LiquidCrystal_I2C lcd(0x27, 16, 2);     // troque para 0x3F se necessário

// ========= BUZZER =========
#define BUZZER_PIN 25                   // recomendo 25 ou 26
const unsigned long BEEP_ON_MS  = 250;  // tempo tocando
const unsigned long BEEP_OFF_MS = 750;  // tempo em silêncio
bool buzzerOn = false;
unsigned long beepPhaseEnd = 0;

// ========= WEB =========
WebServer server(80);

// ========= Estado =========
unsigned long lastSample = 0;
float lastTemp = NAN, lastHum = NAN;
bool humAlto = false;                   // estado global do alerta (com histerese)

// ---------- HTML (página web com auto-update) ----------
const char HTML_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="pt-br">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" type="image/png" href="https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRL9WoFPfcIxb7q5ZPXBd94RdOiI5KuqHKK7A&s">
<title>YOUNG CREATORS</title>
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

// ---------- Atualiza LCD ----------
void updateLCD(bool estadoAlto) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("T: ");
  if (!isnan(lastTemp)) {
    lcd.print(lastTemp, 1);
    lcd.print((char)223);
    lcd.print("C");
  } else {
    lcd.print("--.-");
  }

  lcd.setCursor(0, 1);
  lcd.print("U: ");
  if (!isnan(lastHum)) {
    lcd.print(lastHum, 1);
    lcd.print("% ");
    lcd.print(estadoAlto ? "ALTA" : "OK  ");
  } else {
    lcd.print("--.-");
  }
}

// ---------- Rotas ----------
void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", HTML_PAGE);
}

void handleData() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;

    // Leitura do sensor
    lastHum  = dht.readHumidity();
    lastTemp = dht.readTemperature();

    // Histerese: define humAlto com estabilidade (evita "chattering")
    if (!isnan(lastHum)) {
      if (!humAlto && lastHum >= HUM_ON)  humAlto = true;    // sobe
      if ( humAlto && lastHum <= HUM_OFF) humAlto = false;   // desce
    }

    // Atualiza LCD
    updateLCD(humAlto);
  }

  bool ok = !(isnan(lastHum) || isnan(lastTemp));

  String json = "{";
  if (ok) {
    json += "\"temp\":" + String(lastTemp, 2) + ",";
    json += "\"hum\":"  + String(lastHum, 2)  + ",";
    json += "\"hum_excesso\":" + String(humAlto ? "true":"false");
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
  Wire.begin(21, 22);     // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Iniciando...");

  // BUZZER
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);     // garante silêncio no boot

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

  // Rotas Web
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  // Máquina de estados do buzzer: bipes intermitentes enquanto humAlto==true
  unsigned long now = millis();
  if (humAlto) {
    if (now >= beepPhaseEnd) {
      if (buzzerOn) {
        noTone(BUZZER_PIN);
        buzzerOn = false;
        beepPhaseEnd = now + BEEP_OFF_MS;   // silêncio
      } else {
        tone(BUZZER_PIN, 1000);             // 1 kHz
        buzzerOn = true;
        beepPhaseEnd = now + BEEP_ON_MS;    // som
      }
    }
  } else {
    if (buzzerOn) { noTone(BUZZER_PIN); buzzerOn = false; }
    beepPhaseEnd = now; // reseta fases
  }
}
