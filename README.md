# ARQBIO
Projeto ArqBio — desenvolvido pela equipe Young Creators para a FIRST LEGO League (FLL). Focado na preservação de artefatos arqueológicos, o ArqBio aplica princípios de robótica e tecnologia sustentável para monitorar temperatura e umidade, auxiliando pesquisas e conservação histórica.

Equipamentos necessários e conexões:
                +-------------------------------------+
                |                ESP32                |
                |                                     |
3V3 -----------o| 3V3                            GND |o----------- GND comum
                |                                     |
GPIO21 (SDA) --o| 21 (SDA)                     VIN/5V |o----+------ +5V LCD
GPIO22 (SCL) --o| 22 (SCL)                         GND|o----+------ GND LCD / DHT / Buzzer
GPIO4  (DHT) --o|  4 (DATA)                           |
GPIO25 (Buzz) -o| 25 (BUZZER +)                       |
                +-------------------------------------+
                 |                     |                         |
                 |                     |                         |
                 v                     v                         v
      +------------------+    +------------------------------+    +------------------+
      |     DHT22        |    |   LCD 16x2 + Mód. I2C (PCF)  |    |     BUZZER       |
      | VCC ---- 3V3     |    | VCC ----------- +5V*         |    |  + --- GPIO25    |
      | GND ---- GND     |    | GND ----------- GND          |    |  - --- GND       |
      | DATA -- GPIO4    |    | SDA ----------- GPIO21       |    +------------------+
      |  ^               |    | SCL ----------- GPIO22       |
      |  |               |    | Endereço: 0x27 (ou 0x3F)     |
      |  +-- 10kΩ pull-up|    +------------------------------+
      |      p/ 3V3      |
      +------------------+

* Observação sobre o LCD/I2C:
- Se o backpack I2C (PCF8574) tiver resistores de pull-up de SDA/SCL para 5V, evite expor os pinos do ESP32 a 5V.
  Opções seguras:
  a) Alimente o módulo I2C em 3V3 (muitos PCF8574 funcionam bem em 3V3), ou
  b) Use um level shifter I2C, ou
  c) Remova/alterne os pull-ups para 3V3.

Lista de peças:

1× ESP32 (ESP32-32X).
1× DHT22 (sensor de temperatura/umidade).
1× Resistor 10 kΩ (pull-up do pino DATA do DHT22 para 3V3).
1× LCD 16×2 com backpack I2C PCF8574 (endereço típico 0x27; alguns são 0x3F).
1× Buzzer (pode ser passivo ou ativo; o código funciona com tone()).
Jumpers macho-fêmea / macho-macho.
Fonte/USB (5V) para alimentar o ESP32 (powerbank, carregador, etc.).
(Opcional) Level shifter I2C se o backpack do LCD mantiver pull-ups para 5V.

Alimentação:

Conecte o ESP32 via USB (5V).
Se usar a pinagem, você pode alimentar pelo pino VIN/5V; sempre garanta GND comum entre todos os dispositivos.
DHT22 deve ficar em 3V3.
O LCD I2C pode ser 5V ou 3V3 conforme o backpack; não exponha SDA/SCL do ESP32 a 5V.
