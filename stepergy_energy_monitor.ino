/**
 * ============================================================================
 * Proje: Stepergy – Mekanik & Yaya Trafiğinden Enerji Hasat Sistemi (4B-SE Takımı)
 * Ödüller: GMKA Sürdürülebilir Enerji Fikri Maratonu 3.'lük | GSÜ Girişimcilik Finalist
 * Geliştirici: Berkay Bilgin (https://github.com/BERKAY081014)
 * Donanım: ESP32 / Arduino Nano, INA219 I2C Güç Sensörü, Süperkapasitör Bankı
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_INA219.h>

// ---------- Donanım Pinleri ----------
#define PIN_LED_STREET_LIGHT 4   // Akıllı Sokak Aydınlatma Çıkışı (PWM)
#define PIN_RELAY_STORAGE    16  // Enerji Depolama Süperkapasitör Rölesi
#define PIN_LDR_SENSOR       35  // Ortam Işık Sensörü (Analog Giriş)

Adafruit_INA219 ina219;

// Telemetri ve Enerji İstatistiği
float totalJoulesHarvested = 0.0f;
float totalMilliWattHours  = 0.0f;
unsigned long lastMeasureTime = 0;
unsigned long stepEventCount  = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(PIN_LED_STREET_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_STORAGE, OUTPUT);
  digitalWrite(PIN_RELAY_STORAGE, HIGH); // Şarj hattını aç
  analogWrite(PIN_LED_STREET_LIGHT, 0);

  if (!ina219.begin()) {
    Serial.println("[HATA] INA219 Enerji Sensörü bulunamadı!");
    while (1) { delay(100); }
  }

  // 32V, 2A Kalibrasyonu
  ina219.setCalibration_32V_2A();
  Serial.println("[STEPERGY] Akıllı Enerji Hasadı İzleme Ünitesi Devrede.");
}

void loop() {
  unsigned long now = millis();
  float dt = (now - lastMeasureTime) / 1000.0f;
  lastMeasureTime = now;

  // 1. Hasat Edilen Anlık Elektriksel Değerleri Oku
  float shuntVoltage_mV = ina219.getShuntVoltage_mV();
  float busVoltage_V    = ina219.getBusVoltage_V();
  float current_mA      = ina219.getCurrent_mA();
  float power_mW        = ina219.getPower_mW();

  // Yere basma / mekanik titreşim algılama eşiği (Akım pikleri)
  if (current_mA > 15.0f) {
    stepEventCount++;
  }

  // 2. Kümülatif Enerji Hesabı (Joule = Watt * Saniye)
  if (power_mW > 0.0f) {
    float power_W = power_mW / 1000.0f;
    float deltaJoules = power_W * dt;
    totalJoulesHarvested += deltaJoules;
    totalMilliWattHours += (power_mW * dt) / 3600.0f;
  }

  // 3. Akıllı Aydınlatma Otomasyonu (LDR Gece/Gündüz Algılama)
  int ldrValue = analogRead(PIN_LDR_SENSOR);
  bool isNight = (ldrValue < 1200); // Karanlık eşiği

  if (isNight && (busVoltage_V > 3.6f)) {
    // Depolanan enerjiyle sokak aydınlatmasını çevreye göre modüle et
    analogWrite(PIN_LED_STREET_LIGHT, 180);
  } else {
    analogWrite(PIN_LED_STREET_LIGHT, 0);
  }

  // 4. Süperkapasitör Aşırı Şarj Koruması
  if (busVoltage_V >= 5.4f) {
    digitalWrite(PIN_RELAY_STORAGE, LOW); // Aşırı gerilimde şarjı koru
  } else {
    digitalWrite(PIN_RELAY_STORAGE, HIGH);
  }

  // 5. Konsol ve Telemetri Çıktısı (Her 1 saniyede bir)
  static unsigned long lastLog = 0;
  if (now - lastLog >= 1000) {
    lastLog = now;
    Serial.printf("[STEPERGY] Gerilim: %.2fV | Akim: %.1fmA | Güc: %.1fmW | Toplam Enerji: %.2f J (%.4f mWh) | Adim Sayisi: %lu\n",
                  busVoltage_V, current_mA, power_mW, totalJoulesHarvested, totalMilliWattHours, stepEventCount);
  }

  delay(100);
}\n