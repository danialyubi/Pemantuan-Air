#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <AntaresESPHTTP.h>

// Konfigurasi untuk DS18B20
#define ONE_WIRE_BUS 26 // Pin SDA atau pin data DS18B20 pada Wemos D1 R32
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Konfigurasi untuk sensor pH dan TDS
const int pHSense = 34;   // Pin ADC1_6 untuk sensor pH pada Wemos D1 R32
const int tdsSense = 35;  // Pin ADC1_7 untuk sensor TDS pada Wemos D1 R32
int samples = 10;         // Jumlah sampel untuk pembacaan TDS
const float tdsConversionFactor = 0.5; // Faktor konversi TDS

// Konfigurasi Antares dan Wi-Fi
#define ACCESSKEY "dfc006da1c3a0035:976d081f50d1fc94"  // Kunci akses ke akun Antares
#define WIFISSID "dani"                         // Nama SSID Wi-Fi
#define PASSWORD "alyubi123"                        // Kata sandi Wi-Fi
#define projectName "UnisaHidro"                    // Nama proyek di Antares
#define deviceName "sabtu"                    // Nama perangkat yang terhubung di platform Antares

AntaresESPHTTP antares(ACCESSKEY);               // Instalasi Objek Antares dengan kunci akses

// Inisialisasi LCD I2C (alamat LCD biasanya 0x27 atau 0x3F)
LiquidCrystal_I2C lcd(0x3F, 20, 4);  // 20x4 LCD

void setup() {
    // Inisialisasi Serial Monitor
    Serial.begin(115200);
    Serial.println("Memulai pembacaan sensor...");

    // Inisialisasi pin SDA dan SCL untuk I2C
    Wire.begin(14, 27);  // Ganti D3 (GPIO0) sebagai SDA, dan D4 (GPIO2) sebagai SCL

    // Inisialisasi sensor DS18B20
    sensors.begin();

    // Inisialisasi LCD
    lcd.begin(20, 4);  // Menentukan jumlah kolom dan baris
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    delay(2000);

    // Koneksi Wi-Fi
    WiFi.begin(WIFISSID, PASSWORD);  // Menghubungkan ke jaringan Wi-Fi
    Serial.print("Menghubungkan ke Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Terhubung ke Wi-Fi");

    // Inisialisasi koneksi ke Antares
    antares.setDebug(true);  // Mengaktifkan mode debugging untuk melihat informasi tambahan pada serial monitor
    antares.wifiConnection(WIFISSID, PASSWORD);  // Menghubungkan ke Antares melalui Wi-Fi
}

// Fungsi untuk menghitung pH dari tegangan
float calculatePH(float voltage) {
    const float pHNeutral = 7.0;
    const float voltageNeutral = 1.65;  // Tegangan netral untuk pH sensor
    const float voltagePerPHUnit = 0.059; // Sensitivitas tegangan per unit pH
    return ((voltage - voltageNeutral) / voltagePerPHUnit + pHNeutral) / 2;
}

// Fungsi untuk membaca nilai TDS
int readTDS() {
    long tdsSum = 0;
    for (int i = 0; i < samples; i++) {
        tdsSum += analogRead(tdsSense);  // Membaca nilai analog dari sensor TDS
        delay(10);
    }
    int rawTDSValue = tdsSum / samples;  // Menghitung rata-rata nilai TDS
    return rawTDSValue * tdsConversionFactor;  // Menghitung nilai TDS berdasarkan faktor konversi
}

// Fungsi untuk mengirim data ke Antares
void sendToAntares(float pHValue, int tdsValue, float temperatureC) {
    antares.add("ph", String(pHValue, 2));  // Menambahkan data pH
    antares.add("tds", tdsValue); // Menambahkan data TDS
    antares.add("temp", temperatureC); // Menambahkan data suhu
    antares.sendNonSecure(projectName, deviceName);  // Mengirim data tanpa enkripsi (non-secure)
}

void loop() {
    // Membaca suhu dari DS18B20
    sensors.requestTemperatures();
    float temperatureC = sensors.getTempCByIndex(0);

    // Membaca nilai pH
    long measurings = 0;
    for (int i = 0; i < samples; i++) {
        measurings += analogRead(pHSense);
        delay(10);
    }
    float voltage = (3.3 / 4095.0) * measurings / samples; // ADC Wemos D1 R32 menggunakan resolusi 12-bit (0-4095)
    float pHValue = calculatePH(voltage);

    // Membaca nilai TDS
    int tdsValue = readTDS();

    // Menampilkan data di Serial Monitor
    Serial.print("Suhu: ");
    Serial.print(temperatureC);
    Serial.println(" °C");
    Serial.print("pH: ");
    Serial.print(pHValue, 2);
    Serial.print(" | TDS: ");
    Serial.println(tdsValue);
    Serial.println("-----------------------------");

    // Menampilkan data pada LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperatureC);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("pH: ");
    lcd.print(pHValue, 2);

    lcd.setCursor(0, 2);
    lcd.print("TDS: ");
    lcd.print(tdsValue);
    lcd.print(" ppm");

    lcd.setCursor(0, 3);
    lcd.print("SEMOGA LAB BARU");

    // Kirim data ke Antares
    sendToAntares(pHValue, tdsValue, temperatureC);

    delay(6000); // Tunggu 6 detik sebelum pembacaan berikutnya
}
