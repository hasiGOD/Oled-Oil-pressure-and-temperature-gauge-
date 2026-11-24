//Oled Oil Pressure and Temperature Gauge with arduino nano
//Hannes Sild


#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <avr/pgmspace.h>
#include <math.h>

// =====================================
// OLED SH1106 128x64
// Väike ekraanipuhver - SRAM saastmine
// =====================================

U8G2_SH1106_128X64_NONAME_1_HW_I2C oled(
  U8G2_R0, U8X8_PIN_NONE
);

// =====================================
// SISENDID
// =====================================

const byte PRESSURE_PIN = A0;
const byte TEMP_PIN = A1;
const byte BATTERY_PIN = A2;

// =====================================
// SEADISTUSED
// =====================================

const float VREF = 5.0;

const float R_TOP = 100000.0;
const float R_BOTTOM = 33000.0;
const float R_PULLUP = 4700.0;

// =====================================
// BOSCH PST-F 1 NTC TABEL
// PROGMEM - ei kasuta SRAM-i
// =====================================

const int TEMP_COUNT = 20;

const float tempC[] PROGMEM = {
  -40, -30, -20, -10, 0,
  10, 20, 30, 40, 50,
  60, 70, 80, 90, 100,
  110, 120, 130, 140, 150
};

const float tempOhm[] PROGMEM = {
  44864, 25524, 15067, 9195, 5784,
  3740, 2480, 1683, 1167, 824,
  594, 434.9, 323.4, 244, 186.6,
  144.5, 113.3, 89.9, 71.9, 58.1
};

// =====================================
// ADC KESKMISTAMINE
// =====================================

float readADC(byte pin) {
  analogRead(pin);

  long sum = 0;

  for (byte i = 0; i < 16; i++) {
    sum += analogRead(pin);
  }

  return sum / 16.0;
}

// =====================================
// OLIROHK 0-10 BAR
// =====================================

float readPressure() {
  float adc = readADC(PRESSURE_PIN);

  float pressure =
    ((adc / 1023.0) - 0.1) * 12.5;

  return constrain(pressure, 0.0, 10.0);
}

// =====================================
// OLITEMPERATUUR
// =====================================

float readTemperature() {
  float adc = readADC(TEMP_PIN);

  if (adc <= 0 || adc >= 1023) {
    return NAN;
  }

  float resistance =
    R_PULLUP * adc / (1023.0 - adc);

  float maxR = pgm_read_float(&tempOhm[0]);
  float minR = pgm_read_float(&tempOhm[TEMP_COUNT - 1]);

  if (resistance > maxR || resistance < minR) {
    return NAN;
  }

  for (int i = 0; i < TEMP_COUNT - 1; i++) {

    float r1 = pgm_read_float(&tempOhm[i]);
    float r2 = pgm_read_float(&tempOhm[i + 1]);

    if (resistance <= r1 && resistance >= r2) {

      float t1 = pgm_read_float(&tempC[i]);
      float t2 = pgm_read_float(&tempC[i + 1]);

      float k = (resistance - r1) / (r2 - r1);

      return t1 + k * (t2 - t1);
    }
  }

  return NAN;
}

// =====================================
// AKU PINGE
// =====================================

float readBattery() {
  float adc = readADC(BATTERY_PIN);

  float voltage = adc * VREF / 1023.0;

  return voltage *
         (R_TOP + R_BOTTOM) / R_BOTTOM;
}

// =====================================
// TERMOMEETRI IKOON
// =====================================

// =====================================
// ÕLIKANNU IKOON
// =====================================
void drawOilCanIcon(int x, int y) {

  // Korpus - tehtud õhemaks
  oled.drawFrame(x + 2, y + 5, 12, 6);

  // Kork
  oled.drawBox(x + 4, y + 3, 4, 2);

  // Tila
  oled.drawLine(x + 14, y + 6, x + 18, y + 4);
  oled.drawLine(x + 14, y + 7, x + 18, y + 5);
  oled.drawLine(x + 18, y + 4, x + 19, y + 6);
  oled.drawLine(x + 18, y + 5, x + 19, y + 7);

  // Õlitilk
  oled.drawPixel(x + 21, y + 8);
  oled.drawPixel(x + 20, y + 9);
  oled.drawPixel(x + 21, y + 9);
  oled.drawPixel(x + 22, y + 9);
  oled.drawPixel(x + 21, y + 10);
}
// =====================================
// TERMOMEETRI IKOON
// =====================================
void drawThermometerIcon(int x, int y) {
  oled.drawFrame(x + 3, y, 5, 10);
  oled.drawDisc(x + 5, y + 11, 3);
  oled.drawVLine(x + 5, y + 3, 6);
}

// =====================================
// AKU IKOON
// =====================================
void drawBatteryIcon(int x, int y) {
  // välisraam
  oled.drawFrame(x, y + 2, 16, 8);

  // aku klemm
  oled.drawBox(x + 16, y + 4, 2, 4);

  // miinus
  oled.drawBox(x + 3, y + 5, 3, 1);

  // pluss
  oled.drawBox(x + 10, y + 5, 3, 1);
  oled.drawBox(x + 11, y + 4, 1, 3);
}
// =====================================
// EKRAANI JOONISTAMINE
// =====================================


void drawDisplay(float pressure,
                 float temperature,
                 float battery) {

  char buffer[16];

  oled.setFont(u8g2_font_6x10_tf);

  // ==============================
  // RIDA 1 - ÕLIRÕHK
  // ==============================
  drawOilCanIcon(3, 1);

  dtostrf(pressure, 3, 1, buffer);
  oled.drawStr(4, 22, buffer);
  oled.drawStr(30, 22, "bar");

  // Rõhuriba
  oled.drawFrame(64, 14, 60, 10);

  int fill = constrain((int)(pressure * 5.6), 0, 56);
  if (fill > 0) {
    oled.drawBox(66, 16, fill, 6);
  }

  // eraldusjoon
  oled.drawHLine(2, 26, 122);

  // ==============================
  // RIDA 2 - ÕLITEMPERATUUR
  // ==============================
  drawThermometerIcon(5, 30);

  if (isnan(temperature)) {
    oled.drawStr(104, 42, "ERR");
  } else {
    snprintf(buffer, sizeof(buffer), "%d", (int)round(temperature));

    int x = 108 - oled.getStrWidth(buffer);
    oled.drawStr(x, 42, buffer);

    // kraadimärk + C
    oled.drawCircle(112, 34, 2);
    oled.drawStr(116, 42, "C");
  }

  // eraldusjoon
  oled.drawHLine(2, 47, 122);

  // ==============================
  // RIDA 3 - AKU PINGE
  // ==============================
  drawBatteryIcon(4, 50);

  dtostrf(battery, 4, 1, buffer);
  strcat(buffer, "V");

  int x = 124 - oled.getStrWidth(buffer);
  oled.drawStr(x, 60, buffer);
}
// =====================================
// SETUP
// =====================================

void setup() {
  analogReference(DEFAULT);

  Wire.begin();
  Wire.setClock(100000);

  oled.begin();
  oled.setContrast(200);

  Serial.begin(9600);
}

// =====================================
// LOOP
// =====================================

void loop() {

  float pressure = readPressure();
  float temperature = readTemperature();
  float battery = readBattery();

  // U8g2 lehekulgede kaupa joonistamine
  oled.firstPage();

  do {
    drawDisplay(pressure, temperature, battery);
  } while (oled.nextPage());

  // Serial Monitor
  Serial.print(F("Pressure: "));
  Serial.print(pressure, 2);

  Serial.print(F(" bar | Temperature: "));
  Serial.print(temperature, 1);

  Serial.print(F(" C | Battery: "));
  Serial.println(battery, 2);

  delay(200);
}
