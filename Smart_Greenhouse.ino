#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <BH1750.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

//=================================================
// WIFI & SERVER HOSTING (HTTPS)
//=================================================
const char* ssid        = "Infinix NOTE 40 Pro";
const char* password    = "lakasandie";

const char* serverName  = "https://sukawera-greenhouse.web.id/api/sensor";
const char* settingURL  = "https://sukawera-greenhouse.web.id/api/settings";
const char* deviceCode  = "GH-01";

//=================================================
// SENSOR SENSOR
//=================================================
#define DHTPIN 4
#define DHTTYPE DHT22

#define SOIL_PIN_1 34
#define SOIL_PIN_2 35
//=================================================
// KALIBRASI SENSOR TANAH (INDIVIDU)
//=================================================
// Sensor 1 (SOIL_PIN_1)
const int SOIL1_DRY = 4095;   // Nilai kering total (udara)
const int SOIL1_WET = 1200;   // Nilai tanah sangat basah (bukan air murni)

// Sensor 2 (SOIL_PIN_2)
const int SOIL2_DRY = 4095;   // Nilai kering total (udara)
const int SOIL2_WET = 1300;   // Nilai tanah sangat basah (bukan air murni)

// Filter averaging
const int NUM_SAMPLES = 10;   // Jumlah sampel untuk rata-rata

#define LDR_TIMUR_PIN 33
#define LDR_BARAT_PIN 32

DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;

//=================================================
// SENSOR BATERAI AKI
//=================================================
#define BATERAI_PIN 36

const float R1 = 33000.0;
const float R2 = 10000.0;

float teganganAki = 0.0;
int persenBaterai  = 0;
bool isCharging = false; 

//=================================================
// LCD
//=================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

//=================================================
// SERVO & SOLAR TRACKER
//=================================================
#define SERVO_PIN 13
Servo trackerServo;

int servoPos = 90;                
const int SERVO_MIN = 45;         
const int SERVO_MAX = 135;        
const int SERVO_STEP = 1;         
const int TRACKER_INTERVAL = 300; 

// Nilai Kalibrasi LDR (Hasil Tes Senter & Gelap)
const int TERANG_TIMUR = 0;    // LDR Timur saat terang maksimal
const int GELAP_TIMUR  = 4095; // LDR Timur saat gelap

const int TERANG_BARAT = 230;  // LDR Barat saat terang maksimal
const int GELAP_BARAT  = 4095; // LDR Barat saat gelap

const int LDR_TOLERANCE = 15;  // Toleransi pergerakan (15%)

//=================================================
// RELAY
//=================================================
#define RELAY_KIPAS      25
#define RELAY_POMPA      26
#define RELAY_GROW_LIGHT 27

bool kipasStatus = false;
bool pompaStatus = false;
bool lampuStatus = false;

//=================================================
// THRESHOLD
//=================================================
float BATAS_SUHU_ON  = 33.0;
float BATAS_SUHU_OFF = 31.0;
int BATAS_TANAH_ON   = 25;
int BATAS_TANAH_OFF  = 40;
float BATAS_LUX_ON   = 5000;
float BATAS_LUX_OFF  = 7000;

//=================================================
// DATA SENSOR
//=================================================
float suhu            = 0;
float kelembabanUdara = 0;
float luxCahaya       = 0;
int kelembabanTanah1  = 0;
int kelembabanTanah2  = 0;
bool dhtValid = true;

//=================================================
// TIMER
//=================================================
unsigned long previousMillisPost     = 0;
unsigned long previousMillisLCD      = 0;
unsigned long previousMillisActuator = 0;
unsigned long previousMillisServo    = 0;
unsigned long previousMillisSetting  = 0;

const long intervalPost     = 3000; 
const long intervalLCD      = 3000; 
const long intervalActuator = 2000;
const long intervalSetting  = 30000;

int lcdPage = 0;

//=================================================
// SEGMEN ANGKA TEBAL LCD
//=================================================
byte bar1[8] = {B11100, B11100, B11100, B11100, B11100, B11100, B11100, B11100};
byte bar2[8] = {B00111, B00111, B00111, B00111, B00111, B00111, B00111, B00111};
byte bar3[8] = {B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000};
byte bar4[8] = {B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111};
byte bar5[8] = {B11111, B11111, B00000, B00000, B00000, B00000, B11111, B11111};
byte bar6[8] = {B11100, B11100, B11100, B11100, B11100, B11111, B11111, B11111};
byte bar7[8] = {B11111, B11111, B11111, B00111, B00111, B00111, B00111, B00111};
byte bar8[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};

const char bigDigits[10][2][4] = {
  {{8, 3, 8, 0}, {8, 4, 8, 0}},
  {{3, 8, 32, 0}, {4, 8, 4, 0}},
  {{5, 5, 8, 0}, {8, 4, 4, 0}},
  {{5, 5, 8, 0}, {4, 4, 8, 0}},
  {{8, 4, 8, 0}, {32, 32, 8, 0}},
  {{8, 5, 5, 0}, {4, 4, 8, 0}},
  {{8, 5, 5, 0}, {8, 4, 8, 0}},
  {{3, 3, 8, 0}, {32, 32, 8, 0}},
  {{8, 5, 8, 0}, {8, 4, 8, 0}},
  {{8, 5, 8, 0}, {4, 4, 8, 0}}
};

void printBigDigit(int digit, int col) {
  if (digit < 0 || digit > 9) return;
  lcd.setCursor(col, 0);
  for (int i = 0; i < 3; i++) {
    char c = bigDigits[digit][0][i];
    lcd.write(c == 32 ? 32 : c - 1);
  }
  lcd.setCursor(col, 1);
  for (int i = 0; i < 3; i++) {
    char c = bigDigits[digit][1][i];
    lcd.write(c == 32 ? 32 : c - 1);
  }
}

void displayBigNumber(int val, int startCol) {
  if (val > 999) val = 999;
  if (val < 0)   val = 0;

  int ratusan = val / 100;
  int puluhan = (val % 100) / 10;
  int satuan  = val % 10;

  if (val >= 100) {
    printBigDigit(ratusan, startCol);
  } else {
    lcd.setCursor(startCol, 0); lcd.print("   ");
    lcd.setCursor(startCol, 1); lcd.print("   ");
  }

  if (val >= 10) {
    printBigDigit(puluhan, startCol + 4);
  } else {
    lcd.setCursor(startCol + 4, 0); lcd.print("   ");
    lcd.setCursor(startCol + 4, 1); lcd.print("   ");
  }

  printBigDigit(satuan, startCol + 8);
}

//=================================================
// WIFI RECONNECT
//=================================================
void reconnectWiFi() {
  static unsigned long lastCheck = 0;
  unsigned long current = millis();

  if (current - lastCheck >= 5000) {
    lastCheck = current;

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }
}
//=================================================
// BACA SENSOR DENGAN FILTER RATA-RATA
//=================================================
int readSoilSensorAverage(int pin, int numSamples) {
  long sum = 0;
  for (int i = 0; i < numSamples; i++) {
    sum += analogRead(pin);
    delay(5); // Jeda kecil antar pembacaan
  }
  return sum / numSamples;
}

int calculateMoisturePercent(int rawValue, int dryValue, int wetValue) {
  // Batasi nilai agar tidak keluar rentang
  rawValue = constrain(rawValue, wetValue, dryValue);
  // Hitung persentase
  return map(rawValue, dryValue, wetValue, 0, 100);
}
//=================================================
// BACA SENSOR
//=================================================
void readSensors() {
  suhu            = dht.readTemperature();
  kelembabanUdara = dht.readHumidity();
  luxCahaya       = lightMeter.readLightLevel();

  if (isnan(suhu) || isnan(kelembabanUdara)) {
    dhtValid        = false;
    suhu            = 0;
    kelembabanUdara = 0;
  } else {
    dhtValid = true;
  }

  // Baca Sensor Tanah 1 dengan filter averaging
  int rawSoil1 = readSoilSensorAverage(SOIL_PIN_1, NUM_SAMPLES);
  kelembabanTanah1 = calculateMoisturePercent(rawSoil1, SOIL1_DRY, SOIL1_WET);
  
  // Baca Sensor Tanah 2 dengan filter averaging
  int rawSoil2 = readSoilSensorAverage(SOIL_PIN_2, NUM_SAMPLES);
  kelembabanTanah2 = calculateMoisturePercent(rawSoil2, SOIL2_DRY, SOIL2_WET);
}

void readBattery() {
  int rawADC       = analogRead(BATERAI_PIN);
  float teganganADC = (rawADC / 4095.0) * 3.3;
  teganganAki      = teganganADC * ((R1 + R2) / R2);

  if (teganganAki >= 13.0) {
    isCharging = true;
  } else {
    isCharging = false;
  }

  if (teganganAki <= 10.5) {
    persenBaterai = 0;
  } else if (teganganAki >= 12.6) {
    persenBaterai = 100;
  } else {
    persenBaterai = (int)((teganganAki - 10.5) / (12.6 - 10.5) * 100.0);
  }
}

//=================================================
// KONTROL RELAY
//=================================================
void controlActuators() {
  if (dhtValid) {
    if (!kipasStatus && suhu >= BATAS_SUHU_ON) {
      digitalWrite(RELAY_KIPAS, LOW);
      kipasStatus = true;
    } else if (kipasStatus && suhu <= BATAS_SUHU_OFF) {
      digitalWrite(RELAY_KIPAS, HIGH);
      kipasStatus = false;
    }
  }

  if (!pompaStatus && (kelembabanTanah1 < BATAS_TANAH_ON || kelembabanTanah2 < BATAS_TANAH_ON)) {
    digitalWrite(RELAY_POMPA, LOW);
    pompaStatus = true;
  } else if (pompaStatus && kelembabanTanah1 > BATAS_TANAH_OFF && kelembabanTanah2 > BATAS_TANAH_OFF) {
    digitalWrite(RELAY_POMPA, HIGH);
    pompaStatus = false;
  }

  if (!lampuStatus && luxCahaya < BATAS_LUX_ON) {
    digitalWrite(RELAY_GROW_LIGHT, LOW);
    lampuStatus = true;
  } else if (lampuStatus && luxCahaya > BATAS_LUX_OFF) {
    digitalWrite(RELAY_GROW_LIGHT, HIGH);
    lampuStatus = false;
  }
}

//=================================================
// SOLAR TRACKER
//=================================================
int readFilteredADC(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 10;
}

void solarTracker() {
  int rawTimur = readFilteredADC(LDR_TIMUR_PIN);
  int rawBarat = readFilteredADC(LDR_BARAT_PIN);

  // Konversi nilai ADC mentah ke Persentase (0% - 100%)
  int persenTimur = map(rawTimur, GELAP_TIMUR, TERANG_TIMUR, 0, 100);
  int persenBarat = map(rawBarat, GELAP_BARAT, TERANG_BARAT, 0, 100);

  persenTimur = constrain(persenTimur, 0, 100);
  persenBarat = constrain(persenBarat, 0, 100);

  int diff = persenTimur - persenBarat;

  // Logika pergerakan servo berdasarkan selisih persentase
  if (abs(diff) > LDR_TOLERANCE) {
    if (diff > 0) {
      // Timur lebih terang -> Kurangi posisi servo
      if (servoPos > SERVO_MIN) {
        servoPos -= SERVO_STEP;
      }
    } else {
      // Barat lebih terang -> Tambah posisi servo
      if (servoPos < SERVO_MAX) {
        servoPos += SERVO_STEP;
      }
    }
    trackerServo.write(servoPos);
  }
}

//=================================================
// LCD UPDATE
//=================================================
void updateLCD() {
  lcd.clear();

  if (lcdPage == 0) {
    displayBigNumber((int)suhu, 0);
    lcd.setCursor(12, 0); lcd.print("SUHU");
    lcd.setCursor(12, 1); lcd.print(".C");
    lcdPage = 1;
  } else if (lcdPage == 1) {
    displayBigNumber((int)kelembabanUdara, 0);
    lcd.setCursor(12, 0); lcd.print("HUMI");
    lcd.setCursor(12, 1); lcd.print("%RH");
    lcdPage = 2;
  } else if (lcdPage == 2) {
    displayBigNumber(kelembabanTanah1, 0);
    lcd.setCursor(12, 0); lcd.print("S-1");
    lcd.setCursor(12, 1); lcd.print("%");
    lcdPage = 3;
  } else if (lcdPage == 3) {
    displayBigNumber(kelembabanTanah2, 0);
    lcd.setCursor(12, 0); lcd.print("S-2");
    lcd.setCursor(12, 1); lcd.print("%");
    lcdPage = 4;
  } else if (lcdPage == 4) {
    displayBigNumber(persenBaterai, 0);
    lcd.setCursor(12, 0); 
    if (isCharging) {
      lcd.print("CHG "); 
    } else {
      lcd.print("AKI "); 
    }
    lcd.setCursor(12, 1); 
    lcd.print("%   ");
    lcdPage = 5;
  } else {
    int luxInt = (int)luxCahaya;
    if (luxInt > 999) {
      displayBigNumber(luxInt / 100, 0);
      lcd.setCursor(12, 0); lcd.print("LUX");
      lcd.setCursor(11, 1); lcd.print("x100");
    } else {
      displayBigNumber(luxInt, 0);
      lcd.setCursor(12, 0); lcd.print("LUX");
    }
    lcdPage = 0;
  }
}

//=================================================
// KIRIM DATA KE LARAVEL
//=================================================
void sendDataLaravel() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  WiFiClientSecure client;
  client.setInsecure(); 
  HTTPClient http;
  if (http.begin(client, serverName)) {
    http.setTimeout(3000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "ESP32-Greenhouse-Client");
    char jsonPayload[350];
    snprintf(
      jsonPayload,
      sizeof(jsonPayload),
      "{\"device_code\":\"%s\","
      "\"suhu\":%.1f,"
      "\"kelembaban_udara\":%.1f,"
      "\"kelembaban_tanah_1\":%d,"
      "\"kelembaban_tanah_2\":%d,"
      "\"intensitas_cahaya\":%.1f,"
      "\"tegangan_baterai\":%.2f,"
      "\"persentase_baterai\":%d,"
      "\"is_charging\":%d}",
      deviceCode,
      suhu,
      kelembabanUdara,
      kelembabanTanah1,
      kelembabanTanah2,
      luxCahaya,
      teganganAki,
      persenBaterai,
      isCharging ? 1 : 0
    );

    int httpCode = http.POST(jsonPayload);

    if (httpCode == 201 || httpCode == 200) {
      String response = http.getString();
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error && doc.containsKey("threshold")) {
        BATAS_SUHU_ON   = doc["threshold"]["batas_suhu_on"];
        BATAS_SUHU_OFF  = doc["threshold"]["batas_suhu_off"];
        BATAS_TANAH_ON  = doc["threshold"]["batas_tanah_on"];
        BATAS_TANAH_OFF = doc["threshold"]["batas_tanah_off"];
        BATAS_LUX_ON    = doc["threshold"]["batas_lux_on"];
        BATAS_LUX_OFF   = doc["threshold"]["batas_lux_off"];
      }
    }

    http.end();
  }
}

//=================================================
// GET SETTING DARI LARAVEL
//=================================================
void getSettingLaravel() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String fullSettingURL = String(settingURL) + "?device_code=" + String(deviceCode);

  http.begin(client, fullSettingURL);
  http.setTimeout(3000);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK || httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      BATAS_SUHU_ON   = doc["batas_suhu_on"];
      BATAS_SUHU_OFF  = doc["batas_suhu_off"];
      BATAS_TANAH_ON  = doc["batas_tanah_on"];
      BATAS_TANAH_OFF = doc["batas_tanah_off"];
      BATAS_LUX_ON    = doc["batas_lux_on"];
      BATAS_LUX_OFF   = doc["batas_lux_off"];
    }
  }

  http.end();
}

//=================================================
// SETUP
//=================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, bar1); lcd.createChar(1, bar2);
  lcd.createChar(2, bar3); lcd.createChar(3, bar4);
  lcd.createChar(4, bar5); lcd.createChar(5, bar6);
  lcd.createChar(6, bar7); lcd.createChar(7, bar8);

  lcd.setCursor(0, 0);
  lcd.print("Greenhouse IoT");

  dht.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  trackerServo.setPeriodHertz(50);
  trackerServo.attach(SERVO_PIN, 544, 2400);
  
  servoPos = 90;
  trackerServo.write(servoPos);
  delay(2000);

  for(int i = 0; i < 5; i++) {
    readFilteredADC(LDR_TIMUR_PIN);
    readFilteredADC(LDR_BARAT_PIN);
    delay(50);
  }

  pinMode(RELAY_KIPAS, OUTPUT);
  pinMode(RELAY_POMPA, OUTPUT);
  pinMode(RELAY_GROW_LIGHT, OUTPUT);

  digitalWrite(RELAY_KIPAS, HIGH);
  digitalWrite(RELAY_POMPA, HIGH);
  digitalWrite(RELAY_GROW_LIGHT, HIGH);

  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
  } else {
    lcd.print("WiFi Failed");
  }

  delay(1500);
  lcd.clear();

  getSettingLaravel();
}

//=================================================
// LOOP
//=================================================
void loop() {
  unsigned long currentMillis = millis();

  reconnectWiFi();

  // 1. Pembacaan Sensor & Kontrol Relay (2 Detik Sekali)
  if (currentMillis - previousMillisActuator >= intervalActuator) {
    previousMillisActuator = currentMillis;
    readSensors();
    readBattery();
    controlActuators();
  }

  // 2. Pergerakan Solar Tracker (300 ms Sekali)
  if (currentMillis - previousMillisServo >= TRACKER_INTERVAL) {
    previousMillisServo = currentMillis;
    solarTracker();
  }

  // 3. Pergantian Tampilan LCD (3 Detik Sekali)
  if (currentMillis - previousMillisLCD >= intervalLCD) {
    previousMillisLCD = currentMillis;
    updateLCD();
  }

  // 4. Pengiriman Data ke Laravel Hosting (3 Detik Sekali)
  if (currentMillis - previousMillisPost >= intervalPost) {
    previousMillisPost = currentMillis;
    sendDataLaravel();
  }

  // 5. Cek Setting Threshold via GET (30 Detik Sekali)
  if (currentMillis - previousMillisSetting >= intervalSetting) {
    previousMillisSetting = currentMillis;
    getSettingLaravel();
  }
}