#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===================================================
// KONFIGURASI SENSOR & INDIKATOR
// ===================================================
// Sensor DHT22
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Sensor Ultrasonik HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 18

// Indikator LED
#define LED_PENUH_PIN 26      // LED Merah: Peringatan Pipa Penuh
#define LED_FERMENTASI_PIN 27 // LED Hijau: Fermentasi Sempurna (Matang)

// Dimensi Pipa Losida
const float TINGGI_PIPA_KOSONG = 10.0; // Jarak sensor ke dasar pipa kosong (cm)
const float BATAS_MINIMAL_JARAK = 1.0;  // Target jarak penuh (2 cm)

// Pengaturan Waktu
unsigned long lastMeasure = 0;
const long interval = 1000; // Pembacaan setiap 1 detik

void setup() {
  Serial.begin(115200);

  // Inisialisasi Jalur I2C khusus untuk Pin OLED ESP32
  Wire.begin(OLED_SDA, OLED_SCL);

  // Inisialisasi Pin Sensor & LED
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PENUH_PIN, OUTPUT);
  pinMode(LED_FERMENTASI_PIN, OUTPUT);

  dht.begin();

  // Inisialisasi OLED pada Alamat I2C 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Gagal menginisialisasi OLED SSD1306"));
    for (;;);
  }

  // Tampilan Awal Layar OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println(F("SYSTEM MONITORING"));
  display.setCursor(25, 35);
  display.println(F("PIPA LOSIDA"));
  display.display();
  delay(2000);
}

void loop() {
  unsigned long now = millis();

  if (now - lastMeasure >= interval) {
    lastMeasure = now;

    // 1. Pembacaan Sensor DHT22
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();

    // 2. Pembacaan Sensor Ultrasonik
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2.0;

    // Salin jarak untuk kalkulasi persentase kapasitas
    float distanceClamped = distance;
    if (distanceClamped > TINGGI_PIPA_KOSONG) distanceClamped = TINGGI_PIPA_KOSONG;
    if (distanceClamped < BATAS_MINIMAL_JARAK) distanceClamped = BATAS_MINIMAL_JARAK;

    // Hitung Kapasitas Terisi (%)
    float kapasitas = ((TINGGI_PIPA_KOSONG - distanceClamped) / (TINGGI_PIPA_KOSONG - BATAS_MINIMAL_JARAK)) * 100.0;
    if (kapasitas < 0) kapasitas = 0;
    if (kapasitas > 100) kapasitas = 100;

    // 3. Diagnosis Fermentasi & Kontrol LED Fermentasi Sempurna (Hijau)
    String statusFermentasi = "";
    bool isMatang = false;

    if (temp >= 30.0 && temp <= 50.0) {
      statusFermentasi = "Proses Aktif";
    } else if (temp < 30.0 && temp >= 20.0 && hum <= 80.0 && kapasitas > 5.0) {
      statusFermentasi = "Sempurna/Matang";
      isMatang = true;
    } else if (temp < 30.0) {
      statusFermentasi = "Lambat/Awal";
    } else {
      statusFermentasi = "Terlalu Panas";
    }

    if (isMatang) {
      digitalWrite(LED_FERMENTASI_PIN, HIGH);
    } else {
      digitalWrite(LED_FERMENTASI_PIN, LOW);
    }

    // 4. Status Kapasitas & Kontrol LED Merah (HANYA NYALA SAAT JARAK BENDA <= 2 CM)
    String statusKapasitas = "";
    if (distance <= 2.0 && distance > 0) {
      statusKapasitas = "PENUH!";
      digitalWrite(LED_PENUH_PIN, HIGH); // LED Merah baru aktif saat menyentuh jarak <= 2 cm
    } else if (distance <= 10.0) {
      statusKapasitas = "Hampir Penuh";
      digitalWrite(LED_PENUH_PIN, LOW);
    } else {
      statusKapasitas = "Aman";
      digitalWrite(LED_PENUH_PIN, LOW);
    }

    // 5. Menampilkan Rekapitulasi Informasi ke Layar OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println(F("=== STATUS LOSIDA ==="));

    display.setCursor(0, 14);
    display.print(F("Isi  : "));
    display.print((int)kapasitas);
    display.print(F("% ("));
    display.print(statusKapasitas);
    display.println(F(")"));

    display.setCursor(0, 28);
    display.print(F("Suhu : "));
    display.print(temp, 1);
    display.println(F(" C"));

    display.setCursor(0, 40);
    display.print(F("Lembab: "));
    display.print(hum, 1);
    display.println(F(" %"));

    display.setCursor(0, 52);
    display.print(F("Proses: "));
    display.println(statusFermentasi);

    display.display();
  }
}