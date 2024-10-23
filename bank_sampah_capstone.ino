#include <Arduino.h>
#include "HX711.h"
#include <ESP32Servo.h>
#include <MFRC522.h>
#include <SPI.h>
#include <HardwareSerial.h>
#include "Nextion.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "ArduinoJson.h"

// Pin definisi
#define SS_PIN 21
#define RST_PIN 22
#define TRIG_PIN_A 12
#define ECHO_PIN_A 13
#define TRIG_PIN_B 23
#define ECHO_PIN_B 19
#define TRIG_PIN_C 18
#define ECHO_PIN_C 5  // Ganti dari pin 17 ke pin 5
#define TRIG_PIN_D 2  // Ganti dari pin 16 ke pin 2
#define ECHO_PIN_D 4
#define SERVO_PIN_A 14
#define SERVO_PIN_B 15
#define SERVO_PIN_C 32
#define SERVO_PIN_D 33
#define LOADCELL_DOUT_PIN_A 34
#define LOADCELL_SCK_PIN_A 35
#define LOADCELL_DOUT_PIN_B 26
#define LOADCELL_SCK_PIN_B 27
#define LOADCELL_DOUT_PIN_C 25
#define LOADCELL_SCK_PIN_C 33
#define LOADCELL_DOUT_PIN_D 32
#define LOADCELL_SCK_PIN_D 23
#define NEXTION_RX_PIN 16  // Tetap menggunakan pin 16 untuk RX Nextion
#define NEXTION_TX_PIN 17  // Tetap menggunakan pin 17 untuk TX Nextion 

HardwareSerial nextion(2);

// Membuat objek untuk masing-masing komponen
HX711 scaleA, scaleB, scaleC, scaleD;
Servo servoA, servoB, servoC, servoD;
MFRC522 rfid(SS_PIN, RST_PIN);
NexPage pageLogin = NexPage(0, 0, "login");
NexPage pageHome = NexPage(1, 0, "home");
NexPage pageInfoSaldo = NexPage(2, 0, "info_saldo");
NexPage pageTarikTunai = NexPage(3, 0, "tarik_tunai");
NexPage pageMutasiRekening = NexPage(4, 0, "mutasi");
NexPage pageSelectType = NexPage(5, 0, "setor"); // Pilihan jenis sampah
NexPage pageSelectSubtype = NexPage(6, 0, "detail_setor"); // Pilihan subjenis sampah
NexPage pageKonfirmSetoran = NexPage(7, 0, "konfirm_setor");
NexPage pageInfoSetoran = NexPage(8, 0, "info_setor");
NexPage pageSuccess = NexPage(9, 0, "setor_berhasil");

// Tombol di Laman Home

NexText txtSaldo = NexText(2, 2, "t1");
NexText txtMutasi = NexText(4, 3, "t2");
NexButton btnKirimTarik = NexButton(3, 3, "b4");

NexText txtInputTarik = NexText(3, 7, "n0");

//Tombol Setoran
NexButton btnSelesaiSetor = NexButton(7, 5, "b4");

// Label untuk informasi setoran
NexText txtItem = NexText(8, 6, "t3");
NexText txtWeight = NexText(8, 7, "t4");
NexText txtTotal = NexText(8, 8, "t5");

// Daftar kartu RFID yang diizinkan dan saldo masing-masing
struct Card {
  String id;
  String ownerName;
  float saldo;
};

Card allowedCards[] = {
  {"1234567890AB", "Alice", 430000},
  {"0987654321CD", "Bob", 200000}
};

String cardID = "";  // Menyimpan ID RFID yang digunakan saat ini
String currentOwner = "";
float currentSaldo = 0;
bool accessGranted = false;

// Variabel penyimpanan data setoran
float previousWeight = 0;
float totalWeight = 0;
float pricePerKg = 0;

// Fungsi Nextion HMI
NexTouch *nex_listen_list[] = {
  
  &btnKirimTarik,
  &btnSelesaiSetor,
  
  NULL
};

void selesaiSetorCallback(void *ptr) {
  processStorageA(); // Call the function to process storage A when the button is pressed
  Serial.println("Selesai Setor button pressed, processing storage A");
}


// Fungsi untuk menginisialisasi komponen
void setup() {
  Serial.begin(115200);
  nextion.begin(115200, SERIAL_8N1, NEXTION_RX_PIN, NEXTION_TX_PIN); // Nextion

  // Inisialisasi WiFi
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Inisialisasi RFID
  SPI.begin();
  rfid.PCD_Init();

  // Inisialisasi Servo dan Load Cell
  servoA.attach(SERVO_PIN_A);
  servoB.attach(SERVO_PIN_B);
  servoC.attach(SERVO_PIN_C);
  servoD.attach(SERVO_PIN_D);

  servoA.write(90); // Mulai dari posisi tertutup
  servoB.write(90); // Mulai dari posisi tertutup
  servoC.write(90); // Mulai dari posisi tertutup
  servoD.write(90); // Mulai dari posisi tertutup

  scaleA.begin(LOADCELL_DOUT_PIN_A, LOADCELL_SCK_PIN_A);
  scaleB.begin(LOADCELL_DOUT_PIN_B, LOADCELL_SCK_PIN_B);
  scaleC.begin(LOADCELL_DOUT_PIN_C, LOADCELL_SCK_PIN_C);
  scaleD.begin(LOADCELL_DOUT_PIN_D, LOADCELL_SCK_PIN_D);

  scaleA.set_scale(2280.f); // Sesuaikan kalibrasi
  scaleB.set_scale(2280.f);
  scaleC.set_scale(2280.f);
  scaleD.set_scale(2280.f);

  scaleA.tare();
  scaleB.tare();
  scaleC.tare();
  scaleD.tare();

  // Inisialisasi Sensor Ultrasonik
  pinMode(TRIG_PIN_A, OUTPUT);
  pinMode(ECHO_PIN_A, INPUT);
  pinMode(TRIG_PIN_B, OUTPUT);
  pinMode(ECHO_PIN_B, INPUT);
  pinMode(TRIG_PIN_C, OUTPUT);
  pinMode(ECHO_PIN_C, INPUT);
  pinMode(TRIG_PIN_D, OUTPUT);
  pinMode(ECHO_PIN_D, INPUT);

  // Register button for event listening
  btnSelesaiSetor.attachPop(selesaiSetorCallback, &btnSelesaiSetor);

  // Inisialisasi Nextion
  nexInit();

  // Tampilkan halaman login awal
  pageLogin.show();
  processResult(50);
}

void loop() {
  nexLoop(nex_listen_list);

  // Cek RFID untuk login
  if (!accessGranted) {
    if (checkRFID()) {
      accessGranted = true;
      pageHome.show();
    }
  }
}


// Fungsi untuk cek kartu RFID
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID += String(rfid.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();

  Serial.print("Kartu ditemukan dengan ID: ");
  Serial.println(cardID);

  for (Card &allowedCard : allowedCards) {
    if (cardID == allowedCard.id) {
      currentOwner = allowedCard.ownerName;
      currentSaldo = allowedCard.saldo;
      Serial.print("Akses diberikan kepada: ");
      Serial.println(currentOwner);
      return true;
    }
  }

  return false;
}


// Fungsi untuk memproses penyetoran sampah di storage A (Plastik)
void processStorageA() {
    servoA.write(180);  // Buka penutup (180 derajat)
    delay(1000);
    float currentWeight = scaleA.get_units(5);
    float newWeight = currentWeight - previousWeight;
    previousWeight = currentWeight;
    processResult(newWeight);
    servoA.write(90);  // Tutup penutup (90 derajat)
  
}

// Fungsi untuk memproses penyetoran sampah di storage B (Kertas)
void processStorageB() {
  if (detectItems(TRIG_PIN_B, ECHO_PIN_B) > 0) {
    servoB.write(180);  // Buka penutup (180 derajat)
    delay(1000);
    float currentWeight = scaleB.get_units(5);
    float newWeight = currentWeight - previousWeight;
    previousWeight = currentWeight;
    processResult(newWeight);
    servoB.write(90);  // Tutup penutup (90 derajat)
  }
}

// Fungsi untuk memproses penyetoran sampah di storage C (Besi)
void processStorageC() {
  if (detectItems(TRIG_PIN_C, ECHO_PIN_C) > 0) {
    servoC.write(180);  // Buka penutup (180 derajat)
    delay(1000);
    float currentWeight = scaleC.get_units(5);
    float newWeight = currentWeight - previousWeight;
    previousWeight = currentWeight;
    processResult(newWeight);
    servoC.write(90);  // Tutup penutup (90 derajat)
  }
}

// Fungsi untuk memproses penyetoran sampah di storage D (Kaca)
void processStorageD() {
  if (detectItems(TRIG_PIN_D, ECHO_PIN_D) > 0) {
    servoD.write(180);  // Buka penutup (180 derajat)
    delay(1000);
    float currentWeight = scaleD.get_units(5);
    float newWeight = currentWeight - previousWeight;
    previousWeight = currentWeight;
    processResult(newWeight);
    servoD.write(90);  // Tutup penutup (90 derajat)
  }
}

// Fungsi untuk menghitung total harga dan menampilkan hasil
void processResult(float newWeight) {
  float totalPrice = newWeight * pricePerKg;

  // Tampilkan di halaman info setoran
  char itemBuffer[20], weightBuffer[10], totalBuffer[10];
  sprintf(itemBuffer, "%s","pastik");
  sprintf(weightBuffer, "%.2f kg", newWeight);
  sprintf(totalBuffer, "Rp %.2f", totalPrice);

  txtItem.setText(itemBuffer);
  txtWeight.setText(weightBuffer);
  txtTotal.setText(totalBuffer);

  // Update saldo
  currentSaldo += totalPrice;

  pageInfoSetoran.show();
}

// Fungsi untuk mendeteksi barang dengan sensor ultrasonik
int detectItems(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2; // Jarak dalam cm

  if (distance < 10) {  // Jika ada barang terdeteksi
    return 1;
  }
  return 0;
}

// Fungsi untuk mengambil data saldo dari API/database
void showSaldo() {
  // Pastikan ESP32 terhubung ke WiFi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // URL API untuk mendapatkan data saldo berdasarkan ID RFID pengguna yang sedang login
    String url = "http://your-api.com/getSaldo?rfid=" + cardID;
    http.begin(url); // Memulai HTTP request dengan URL

    // Lakukan GET request
    int httpResponseCode = http.GET();

    // Jika request berhasil (kode respons HTTP 200)
    if (httpResponseCode == 200) {
      String payload = http.getString(); // Mendapatkan respons dari server
      Serial.println("Saldo diterima:");
      Serial.println(payload);

      // Parsing respons JSON untuk mendapatkan saldo
      // Misal format JSON: {"saldo": 430000}
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float saldo = doc["saldo"]; // Mengambil nilai saldo dari JSON

        // Menyusun teks saldo yang akan ditampilkan di HMI
        char saldoBuffer[20];
        sprintf(saldoBuffer, "Rp %.2f", saldo);

        // Tampilkan saldo di halaman info_saldo HMI
        txtSaldo.setText(saldoBuffer);

        Serial.print("Saldo pengguna: Rp ");
        Serial.println(saldo);
      } else {
        Serial.println("Error parsing JSON");
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }

    // Menutup koneksi HTTP
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  // Pindah ke halaman info saldo di HMI
  pageInfoSaldo.show();
}

// Fungsi untuk memproses tarik tunai dan mengupdate saldo di database
void processTarikTunai() {
  char tarikBuffer[10];
  txtInputTarik.getText(tarikBuffer, sizeof(tarikBuffer));
  float amount = atof(tarikBuffer);  // Mengambil jumlah tarik tunai dari input pengguna

  if (amount > currentSaldo) {
    Serial.println("Saldo tidak cukup untuk tarik tunai");
    // Tambahkan kode untuk menampilkan pesan error di HMI jika saldo tidak cukup
  } else {
    // Kurangi saldo dari sistem lokal
    currentSaldo -= amount;

    // Kirim permintaan untuk memperbarui saldo di database
    if (updateSaldoInDatabase(cardID, amount)) {
      // Kirim email notifikasi setelah saldo berhasil diperbarui
      Serial.println("Saldo berhasil diperbarui.");
    } else {
      Serial.println("Gagal memperbarui saldo di database.");
    }
  }
}

// Fungsi untuk memperbarui saldo di database (melalui HTTP POST)
bool updateSaldoInDatabase(String rfid, float amount) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://your-api.com/updateSaldo";  // Endpoint API untuk memperbarui saldo
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    // Data yang akan dikirim ke server: ID RFID dan jumlah tarik tunai
    String postData = "rfid=" + rfid + "&amount=" + String(amount, 2);

    // Lakukan POST request
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode == 200) {
      Serial.println("Saldo berhasil diperbarui di database.");
      http.end();
      return true;
    } else {
      Serial.print("Error updating saldo: ");
      Serial.println(httpResponseCode);
      http.end();
      return false;
    }
  } else {
    Serial.println("WiFi not connected");
    return false;
  }
}


// Fungsi untuk mengambil data mutasi rekening dari API/database
void showMutasiRekening() {
  // Koneksi ke WiFi harus sudah tersambung
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Mengakses API untuk mengambil data mutasi berdasarkan ID RFID
    String url = "http://your-api.com/getMutasi?rfid=" + cardID;
    http.begin(url); // URL API untuk mendapatkan data

    // Lakukan GET request
    int httpResponseCode = http.GET();

    // Jika request berhasil (kode respons HTTP 200)
    if (httpResponseCode == 200) {
      String payload = http.getString(); // Mendapatkan respons dari server
      Serial.println("Mutasi rekening diterima:");
      Serial.println(payload);

      // Parsing respons JSON untuk menampilkan data
      // Misal format JSON: {"mutasi": [{"tanggal": "20-10-2024", "transaksi": "Setor", "jumlah": 200000}, {...}]}
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String mutasiText = "Mutasi Transaksi:\n";
        for (JsonObject item : doc["mutasi"].as<JsonArray>()) {
          String tanggal = item["tanggal"];
          String transaksi = item["transaksi"];
          int jumlah = item["jumlah"];

          // Menyusun teks untuk ditampilkan di HMI
          mutasiText += tanggal + ", " + transaksi + ", Rp " + String(jumlah) + "\n";
        }

        // Tampilkan mutasi di halaman mutasi_rekening HMI
        txtMutasi.setText(mutasiText.c_str());
      } else {
        Serial.println("Error parsing JSON");
      }
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }

    // Menutup koneksi HTTP
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }

  // Pindah ke halaman mutasi rekening di HMI
  pageMutasiRekening.show();
}

// Fungsi yang dipanggil ketika tombol "Info Saldo" ditekan
void infoSaldo(void *ptr) {
  showSaldo();
}

// Fungsi yang dipanggil ketika tombol "Tarik Tunai" ditekan
void tarikTunai(void *ptr) {
  pageTarikTunai.show();
}

// Fungsi yang dipanggil ketika tombol "Mutasi Rekening" ditekan
void mutasiRekening(void *ptr) {
  showMutasiRekening();
}

// Fungsi yang dipanggil ketika tombol "Kirim Tarik Tunai" ditekan
void kirimTarik(void *ptr) {
  processTarikTunai();
}

