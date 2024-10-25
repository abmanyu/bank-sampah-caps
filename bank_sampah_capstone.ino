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

/*--------------------------
  DEKLARASI KOMPONEN NEXTION
  --------------------------*/

  NexPage pageLogin = NexPage(0, 0, "login");
  NexPage pageHome = NexPage(1, 0, "home");
  NexPage pageInfoSaldo = NexPage(2, 0, "info_saldo");
  NexPage pageTarikTunai = NexPage(3, 0, "tarik_tunai");
  NexPage pageKonfirmTarik = NexPage(4, 0, "konfirm_tarik");
  NexPage pageMutasiRekening = NexPage(5, 0, "mutasi");
  NexPage pageSelectType = NexPage(6, 0, "setor"); // Pilihan jenis sampah
  NexPage pageSetorPlastik = NexPage(7, 0, "setor_plastik"); // Pilihan jenis sampah
  NexPage pageSetorKertas = NexPage(8, 0, "setor_kertas"); // Pilihan jenis sampah
  NexPage pageSetorLogam = NexPage(9, 0, "setor_logam"); // Pilihan jenis sampah
  NexPage pageSetorKaca = NexPage(10, 0, "setor_kaca"); // Pilihan jenis sampah
  NexPage pageKonfirmSetoran = NexPage(11, 0, "konfirm_setor");
  NexPage pageInfoSetoran = NexPage(12, 0, "info_setor");
  NexPage pageSuccess = NexPage(13, 0, "setor_berhasil");


  //Tombol di Page Home
  NexButton btnInformasiSaldo = NexButton(1, 1, "bo");
  NexButton btnSetorSampah = NexButton(1, 4, "b3");
  NexButton btnMutasiRekening = NexButton(1, 2, "b1");
  NexButton btnTarikTunai = NexButton(1, 3, "b2");
  NexButton btnStop = NexButton(1, 5, "b4");

  //Tombol di Page Info Saldo
  NexText txtInfoSaldo = NexText(2, 2, "t1");
  NexButton btnKembaliInfoSaldo = NexButton(2, 3, "b4");

  //Tombol di Page Tarik Tunai
  NexNumber numInputTarik = NexNumber(3,7, "n0" );
  NexButton btnOkTarik = NexButton(3, 4, "b0");
  NexText txtStatusTarik = NexText(3,2, "t2" );
  NexButton btnKonfirmasiTarik = NexButton(3, 3, "b4");

  //Tombol di Page Konfirm Tarik
  NexButton btnSelesaiTarik = NexButton(4, 3, "b4");

  //Tombol di Page Mutasi
  NexText txtMutasiRekening = NexText(5, 3, "t2" );
  NexButton btnKembaliMutasiRekening = NexButton(5, 2, "b4");

  //Tombol di Page Setor
  NexButton btnSetorPlastik = NexButton(6, 2, "b0");
  NexButton btnSetorKertas = NexButton(6, 5, "b3");
  NexButton btnSetorLogam = NexButton(6, 3, "b1");
  NexButton btnSetorKaca = NexButton(6, 4, "b2");
  NexButton btnKembaliSetor = NexButton(6, 6, "b4");

  //Tombol di Page Setor Plastik
  NexPicture picBotolBening = NexPicture(7, 4, "p0");
  NexPicture picBotolWarna = NexPicture(7, 5, "p1");
  NexPicture picPlastikKemasan = NexPicture(7, 6, "p2");
  NexPicture picTutupBotol = NexPicture(7, 7, "p3");
  NexButton btnKembaliSetorPlastik = NexButton(7, 3, "b4");
  //Text nya
  NexText txtBotolBening = NexText(7, 8, "t2");
  NexText txtBotolWarna = NexText(7, 10, "t4");
  NexText txtPlastikKemasan = NexText(7, 12, "t6");
  NexText txtTutupBotol = NexText(7, 14, "t8");

  //Tombol di Page Setor Kertas
  NexPicture picArsip = NexPicture(8, 4, "p0");
  NexPicture picTettraPack = NexPicture(8, 5, "p1");
  NexPicture picKardus = NexPicture(8, 6, "p2");
  NexPicture picMajalah = NexPicture(8, 7, "p3");
  NexButton btnKembalSetorKertas = NexButton(8, 3, "b4");
  //Text nya
  NexText txtArsip = NexText(8, 8, "t2");
  NexText txtTettraPack = NexText(8, 10, "t4");
  NexText txtKardus = NexText(8, 12, "t6");
  NexText txtMajalah = NexText(8, 14, "t8");

  //Tombol di Page Setor Logam
  NexPicture picSeng = NexPicture(9, 4, "p0");
  NexPicture picBesi = NexPicture(9, 5, "p1");
  NexPicture picAluminium = NexPicture(9, 6, "p2");
  NexPicture picTembaga = NexPicture(9, 7, "p3");
  NexButton btnKembaliSetorLogam = NexButton(9, 3, "b4");
  //Text nya
  NexText txtSeng = NexText(9, 8, "t2");
  NexText txtBesi = NexText(9, 10, "t4");
  NexText txtAluminium = NexText(9, 12, "t6");
  NexText txtTembaga = NexText(9, 14, "t8");

  //Tombol di Page Setor Kaca
  NexPicture picBeling = NexPicture(10, 4, "p0");
  NexPicture picBotolKecap = NexPicture(10, 5, "p1");
  NexPicture picBotolUtuh = NexPicture(10, 6, "p2");
  NexPicture picBotolHijau = NexPicture(10, 7, "p3");
  NexButton btnKembaliSetorKaca = NexButton(10, 3, "b4");
  //Text nya
  NexText txtBeling = NexText(10, 8, "t2");
  NexText txtBotolKecap = NexText(10, 10, "t4");
  NexText txtBotolUtuh = NexText(10, 12, "t6");
  NexText txtBotolHijau = NexText(10, 14, "t8");

  //Tombol di Page Konfirm Setor
  NexButton btnSelesaiSetor = NexButton(11, 5, "b4");

  //Tombol di Page Info Setor
  NexText txtItem = NexText(12, 6, "t3");
  NexText txtWeight = NexText(12, 7, "t4");
  NexText txtTotal = NexText(12, 8, "t5");

  //Tombol di Page Setor Berhasil
  NexText txtSaldo = NexText(13, 9, "t4");
  NexButton btnTidak = NexButton(13, 8, "b0");
  NexButton btnYa = NexButton(13, 7, "b4");




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

// Variabel penyimpanan berat untuk masing-masing storage
float previousWeightA = 0, previousWeightB = 0, previousWeightC = 0, previousWeightD = 0;
float currentWeightA = 0, currentWeightB = 0, currentWeightC = 0, currentWeightD = 0;
float newWeightA = 0, newWeightB = 0, newWeightC = 0, newWeightD = 0;

// Fungsi Nextion HMI
NexTouch *nex_listen_list[] = {
  
  // Tombol di Page Home
    &btnInformasiSaldo,
    &btnSetorSampah,
    &btnMutasiRekening,
    &btnTarikTunai,
    &btnStop,

  // Tombol di Page Info Saldo
    &btnKembaliInfoSaldo,

  // Tombol di Page Tarik Tunai
    &btnOkTarik,
    &btnKonfirmasiTarik,

  // Tombol di Page Selesai Tarik
    &btnSelesaiTarik,

  // Tombol di Page Mutasi
    &btnKembaliMutasiRekening,

  // Tombol di Page Setor
    &btnSetorPlastik,
    &btnSetorKertas,
    &btnSetorLogam,
    &btnSetorKaca,
    &btnKembaliSetor,

  // Tombol dan Gambar di Page Setor Plastik
    &picBotolBening,
    &picBotolWarna,
    &picPlastikKemasan,
    &picTutupBotol,
    &btnKembaliSetorPlastik,

  // Tombol dan Gambar di Page Setor Kertas
    &picArsip,
    &picTettraPack,
    &picKardus,
    &picMajalah,
    &btnKembalSetorKertas,

  // Tombol dan Gambar di Page Setor Logam
    &picSeng,
    &picBesi,
    &picAluminium,
    &picTembaga,
    &btnKembaliSetorLogam,

  // Tombol dan Gambar di Page Setor Kaca
    &picBeling,
    &picBotolKecap,
    &picBotolUtuh,
    &picBotolHijau,
    &btnKembaliSetorKaca,

  // Tombol di Page Konfirm Setor
    &btnSelesaiSetor,
  
  // Tombol di Page Setor Berhasil
    &btnTidak,
    &btnYa,
  
  NULL
};


/*-----------
  VOID SETUP
------------*/

// Fungsi untuk menginisialisasi komponen
void setup() {
  Serial.begin(115200);
  nextion.begin(9600, SERIAL_8N1, NEXTION_RX_PIN, NEXTION_TX_PIN); // Nextion

  // Inisialisasi WiFi
  WiFi.begin("ysl", "zzzzzzzz");
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

  // Inisialisasi Nextion
  nexInit();

  /*------------------------------------
    PER EVENT LISTENING AN (attachPush) 
  -------------------------------------*/

  // Hubungkan tombol dan gambar dengan callback menggunakan attachPush (Touch Press Event)
    btnInformasiSaldo.attachPush(informasiSaldoCallback, &btnInformasiSaldo);
    btnSetorSampah.attachPush(setorSampahCallback, &btnSetorSampah);
    btnMutasiRekening.attachPush(mutasiRekeningCallback, &btnMutasiRekening);
    btnTarikTunai.attachPush(tarikTunaiCallback, &btnTarikTunai);
    btnStop.attachPush(stopCallback, &btnStop);

    btnKembaliInfoSaldo.attachPush(kembaliInfoSaldoCallback, &btnKembaliInfoSaldo);
    btnOkTarik.attachPush(okTarikCallback, &btnOkTarik);
    btnKonfirmasiTarik.attachPush(konfirmasiTarikCallback, &btnKonfirmasiTarik);
    btnSelesaiTarik.attachPush(selesaiTarikCallback, &btnSelesaiTarik);
    btnKembaliMutasiRekening.attachPush(kembaliMutasiRekeningCallback, &btnKembaliMutasiRekening);

    btnSetorPlastik.attachPush(setorPlastikCallback, &btnSetorPlastik);
    btnSetorKertas.attachPush(setorKertasCallback, &btnSetorKertas);
    btnSetorLogam.attachPush(setorLogamCallback, &btnSetorLogam);
    btnSetorKaca.attachPush(setorKacaCallback, &btnSetorKaca);
    btnKembaliSetor.attachPush(kembaliSetorCallback, &btnKembaliSetor);

    // Page Setor Plastik
    picBotolBening.attachPush(botolBeningCallback, &picBotolBening);
    picBotolWarna.attachPush(botolWarnaCallback, &picBotolWarna);
    picPlastikKemasan.attachPush(plastikKemasanCallback, &picPlastikKemasan);
    picTutupBotol.attachPush(tutupBotolCallback, &picTutupBotol);
    btnKembaliSetorPlastik.attachPush(kembaliSetorPlastikCallback, &btnKembaliSetorPlastik);

    // Page Setor Kertas
    picArsip.attachPush(arsipCallback, &picArsip);  // Menggunakan callback arsipCallback
    picTettraPack.attachPush(tettraPackCallback, &picTettraPack);  // Menggunakan callback tettraPackCallback
    picKardus.attachPush(kardusCallback, &picKardus);  // Menggunakan callback kardusCallback
    picMajalah.attachPush(majalahCallback, &picMajalah);  // Menggunakan callback majalahCallback
    btnKembalSetorKertas.attachPush(kembaliSetorKertasCallback, &btnKembalSetorKertas);

    // Page Setor Logam
    picSeng.attachPush(sengCallback, &picSeng);  // Menggunakan callback sengCallback
    picBesi.attachPush(besiCallback, &picBesi);  // Menggunakan callback besiCallback
    picAluminium.attachPush(aluminiumCallback, &picAluminium);  // Menggunakan callback aluminiumCallback
    picTembaga.attachPush(tembagaCallback, &picTembaga);  // Menggunakan callback tembagaCallback
    btnKembaliSetorLogam.attachPush(kembaliSetorLogamCallback, &btnKembaliSetorLogam);

    // Page Setor Kaca
    picBeling.attachPush(belingCallback, &picBeling);  // Menggunakan callback belingCallback
    picBotolKecap.attachPush(botolKecapCallback, &picBotolKecap);  // Menggunakan callback botolKecapCallback
    picBotolUtuh.attachPush(botolUtuhCallback, &picBotolUtuh);  // Menggunakan callback botolUtuhCallback
    picBotolHijau.attachPush(botolHijauCallback, &picBotolHijau);  // Menggunakan callback botolHijauCallback
    btnKembaliSetorKaca.attachPush(kembaliSetorKacaCallback, &btnKembaliSetorKaca);

    btnSelesaiSetor.attachPush(selesaiSetorCallback, &btnSelesaiSetor);
    btnTidak.attachPush(tidakCallback, &btnTidak);
    btnYa.attachPush(yaCallback, &btnYa);

  // Tampilkan halaman login awal
  pageLogin.show();
}




/*----------
  VOID LOOP
-----------*/

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

/*----------------
  PROCESS STORAGE
-----------------*/
// Penggantian proses setoran pada fungsi processStorage sesuai subjenis
void processStorageA(char *subtype) {
    if(detectItems(TRIG_PIN_A, ECHO_PIN_A) > 0){
      servoA.write(180);  // Buka penutup (180 derajat)
      delay(1000);
      currentWeightA = scaleA.get_units(10);
      newWeightA = currentWeightA - previousWeightA;
      previousWeightA = currentWeightA;
      processResultPlastic(newWeightA, subtype);  // Panggil fungsi hasil untuk plastik berdasarkan subjenis
    }
    
}

void processStorageB(char *subtype) {
    if (detectItems(TRIG_PIN_B, ECHO_PIN_B) > 0) {
        servoB.write(180);  // Buka penutup (180 derajat)
        delay(1000);
        currentWeightB = scaleB.get_units(10);
        newWeightB = currentWeightB - previousWeightB;
        previousWeightB = currentWeightB;
        processResultPaper(newWeightB, subtype);  // Panggil fungsi hasil untuk kertas berdasarkan subjenis
    }
}

void processStorageC(char *subtype) {
    if (detectItems(TRIG_PIN_C, ECHO_PIN_C) > 0) {
        servoC.write(180);  // Buka penutup (180 derajat)
        delay(1000);
        currentWeightC = scaleC.get_units(10);
        newWeightC = currentWeightC - previousWeightC;
        previousWeightC = currentWeightC;
        processResultMetal(newWeightC, subtype);  // Panggil fungsi hasil untuk logam berdasarkan subjenis
    }
}

void processStorageD(char *subtype) {
    if (detectItems(TRIG_PIN_D, ECHO_PIN_D) > 0) {
        servoD.write(180);  // Buka penutup (180 derajat)
        delay(1000);
        currentWeightD = scaleD.get_units(10);
        newWeightD = currentWeightD - previousWeightD;
        previousWeightD = currentWeightD;
        processResultGlass(newWeightD, subtype);  // Panggil fungsi hasil untuk kaca berdasarkan subjenis
    }
}

/*---------------
  PROCESS RESULT
---------------*/
// Fungsi untuk memproses hasil setoran berdasarkan subjenis plastik
void processResultPlastic(float newWeight, char *subtype) {
    float pricePerKg = 0;

    if (strcmp(subtype, "PET Botol Bening") == 0) {
        pricePerKg = 5500;
    } else if (strcmp(subtype, "PET Botol Warna") == 0) {
        pricePerKg = 1500;
    } else if (strcmp(subtype, "Plastic Kemasan") == 0) {
        pricePerKg = 400;
    } else if (strcmp(subtype, "Tutup Botol") == 0) {
        pricePerKg = 3000;
    } else {
        Serial.println("Subjenis plastik tidak ditemukan!");
        return;
    }

    // Hitung total harga
    float totalPrice = newWeight * pricePerKg;

    // Tampilkan hasil di Nextion
    char itemBuffer[20], weightBuffer[10], totalBuffer[10];
    sprintf(itemBuffer, "%s", subtype);
    sprintf(weightBuffer, "%.2f kg", newWeight);
    sprintf(totalBuffer, "Rp %.2f", totalPrice);

    txtItem.setText(itemBuffer);
    txtWeight.setText(weightBuffer);
    txtTotal.setText(totalBuffer);

    // Update saldo
    currentSaldo += totalPrice;

    // Pindahkan ke halaman info setoran
    pageInfoSetoran.show();
}


// Fungsi untuk memproses hasil setoran berdasarkan subjenis kertas
void processResultPaper(float newWeight, char *subtype) {
    float pricePerKg = 0;

    if (strcmp(subtype, "Arsip (hvs, buku)") == 0) {
        pricePerKg = 1800;
    } else if (strcmp(subtype, "Tetra Pack") == 0) {
        pricePerKg = 300;
    } else if (strcmp(subtype, "Kardus") == 0) {
        pricePerKg = 1600;
    } else if (strcmp(subtype, "Majalah/koran") == 0) {
        pricePerKg = 500;
    } else {
        Serial.println("Subjenis kertas tidak ditemukan!");
        return;
    }

    float totalPrice = newWeight * pricePerKg;
    char itemBuffer[20], weightBuffer[10], totalBuffer[10];
    sprintf(itemBuffer, "%s", subtype);
    sprintf(weightBuffer, "%.2f kg", newWeight);
    sprintf(totalBuffer, "Rp %.2f", totalPrice);

    txtItem.setText(itemBuffer);
    txtWeight.setText(weightBuffer);
    txtTotal.setText(totalBuffer);

    currentSaldo += totalPrice;
    pageInfoSetoran.show();
}


// Fungsi untuk memproses hasil setoran berdasarkan subjenis logam
void processResultMetal(float newWeight, char *subtype) {
    float pricePerKg = 0;

    if (strcmp(subtype, "Seng (kaleng)") == 0) {
        pricePerKg = 2000;
    } else if (strcmp(subtype, "Besi (paku, dll)") == 0) {
        pricePerKg = 3500;
    } else if (strcmp(subtype, "Aluminium") == 0) {
        pricePerKg = 10000;
    } else if (strcmp(subtype, "Tembaga (kawat)") == 0) {
        pricePerKg = 40000;
    } else {
        Serial.println("Subjenis logam tidak ditemukan!");
        return;
    }

    // Hitung total harga
    float totalPrice = newWeight * pricePerKg;

    // Tampilkan hasil di Nextion
    char itemBuffer[20], weightBuffer[10], totalBuffer[10];
    sprintf(itemBuffer, "%s", subtype);
    sprintf(weightBuffer, "%.2f kg", newWeight);
    sprintf(totalBuffer, "Rp %.2f", totalPrice);

    txtItem.setText(itemBuffer);
    txtWeight.setText(weightBuffer);
    txtTotal.setText(totalBuffer);

    // Update saldo
    currentSaldo += totalPrice;

    // Pindahkan ke halaman info setoran
    pageInfoSetoran.show();
}


// Fungsi untuk memproses hasil setoran berdasarkan subjenis kaca
void processResultGlass(float newWeight, char *subtype) {
    float pricePerKg = 0;

    if (strcmp(subtype, "Beling(pecahan)") == 0) {
        pricePerKg = 200;
    } else if (strcmp(subtype, "Botol Kecap") == 0) {
        pricePerKg = 300;
    } else if (strcmp(subtype, "Botol Utuh") == 0) {
        pricePerKg = 400;
    } else if (strcmp(subtype, "Botol Hijau") == 0) {
        pricePerKg = 500;
    } else {
        Serial.println("Subjenis kaca tidak ditemukan!");
        return;
    }

    // Hitung total harga
    float totalPrice = newWeight * pricePerKg;

    // Tampilkan hasil di Nextion
    char itemBuffer[20], weightBuffer[10], totalBuffer[10];
    sprintf(itemBuffer, "%s", subtype);
    sprintf(weightBuffer, "%.2f kg", newWeight);
    sprintf(totalBuffer, "Rp %.2f", totalPrice);

    txtItem.setText(itemBuffer);
    txtWeight.setText(weightBuffer);
    txtTotal.setText(totalBuffer);

    // Update saldo
    currentSaldo += totalPrice;

    // Pindahkan ke halaman info setoran
    pageInfoSetoran.show();
}


// Fungsi untuk mendeteksi barang dengan sensor ultrasonik
int detectItems(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH,20000);
  int distance = duration * 0.034 / 2; // Jarak dalam cm

  if (distance < 10) {  // Jika ada barang terdeteksi
    return 1;
  }
  return 0;
}


/* --------------------
  PER CALL BACK AN
----------------------*/

// Callback untuk tombol plastik
void setorPlastikCallback(void *ptr) {
    Serial.println("Tombol Setor Plastik Ditekan");
}

// Callback untuk tombol kertas
void setorKertasCallback(void *ptr) {
    Serial.println("Tombol Setor Kertas Ditekan");
}

// Callback untuk tombol logam
void setorLogamCallback(void *ptr) {
    Serial.println("Tombol Setor Logam Ditekan");
}

// Callback untuk tombol kaca
void setorKacaCallback(void *ptr) {
    Serial.println("Tombol Setor Kaca Ditekan");
}

// Callback Selesai Setor untuk Tombol Selesai
void selesaiSetorCallback(void *ptr) {
    // Semua servo kembali ke posisi 90 derajat setelah setoran selesai
    Serial.println("Tombol Selesai Setor Ditekan");
    servoA.write(90);
    servoB.write(90);
    servoC.write(90);
    servoD.write(90);
    Serial.println("Selesai Setor button pressed, all servos returned to 90 degrees");
}

// Fungsi yang dipanggil ketika tombol "Info Saldo" ditekan
void informasiSaldoCallback(void *ptr) {
    Serial.println("Tombol Informasi Saldo Ditekan");
    showSaldo();
    // Aksi untuk informasi saldo
}

// Fungsi yang dipanggil ketika tombol "Setor sampah" ditekan
void setorSampahCallback(void *ptr) {
    Serial.println("Tombol Setor Sampah Ditekan");
    // Aksi untuk setor sampah
}

// Fungsi yang dipanggil ketika tombol "Tarik Tunai" ditekan
void tarikTunaiCallback(void *ptr) {
    Serial.println("Tombol Tarik Tunai Ditekan");
    pageTarikTunai.show();
    // Aksi untuk tarik tunai
}

// Fungsi yang dipanggil ketika tombol "Mutasi Rekening" ditekan
void mutasiRekeningCallback(void *ptr) {
    Serial.println("Tombol Mutasi Rekening Ditekan");
    showMutasiRekening();
    // Aksi untuk mutasi rekening
}

// Fungsi yang dipanggil ketika tombol "Kirim Tarik Tunai" ditekan (BARU INI YANG GW GANTI)
void konfirmasiTarikCallback(void *ptr) {
    Serial.println("Tombol Konfirmasi Tarik Ditekan");
    processTarikTunai();
    // Aksi untuk konfirmasi tarik tunai
}

// Tombol Stop di Halaman HOME
void stopCallback(void *ptr) {
    Serial.println("Tombol Stop Ditekan");
    // Aksi untuk stop
}

// Tombol OK di Tarik Tunai
void okTarikCallback(void *ptr) {
    Serial.println("Tombol OK Tarik Ditekan");
    // Aksi untuk konfirmasi tarik
}

// Tombol Selesai di Tarik Tunai
void selesaiTarikCallback(void *ptr) {
    Serial.println("Tombol Selesai Tarik Ditekan");
    // Aksi untuk selesai tarik tunai
}

/*--------------
 Tombol KEMBALI
---------------*/
void kembaliInfoSaldoCallback(void *ptr) {
    Serial.println("Tombol Kembali Info Saldo Ditekan");
    // Aksi untuk kembali dari info saldo
}

void kembaliMutasiRekeningCallback(void *ptr) {
    Serial.println("Tombol Kembali Mutasi Rekening Ditekan");
    // Aksi untuk kembali dari mutasi rekening
}

void kembaliSetorCallback(void *ptr) {
    Serial.println("Tombol Kembali Setor Ditekan");
    // Aksi untuk kembali dari setoran sampah
}

// Tombol Kembali di Page Setor Sub Jenis
void kembaliSetorPlastikCallback(void *ptr) {
    Serial.println("Tombol Kembali Setor Plastik Ditekan");
    // Aksi untuk kembali dari setor plastik
}

void kembaliSetorKertasCallback(void *ptr) {
    Serial.println("Tombol Kembali Setor Kertas Ditekan");
    // Aksi untuk kembali dari setor kertas
}

void kembaliSetorLogamCallback(void *ptr) {
    Serial.println("Tombol Kembali Setor Logam Ditekan");
    // Aksi untuk kembali dari setor logam
}

void kembaliSetorKacaCallback(void *ptr) {
    Serial.println("Tombol Kembali Setor Kaca Ditekan");
    // Aksi untuk kembali dari setor kaca
}

/*-----------------------
 Tombol Sub Jenis PLASTIK
------------------------*/
void botolBeningCallback(void *ptr) {
    Serial.println("Gambar Botol Bening Ditekan");
    char buffer[20];
    txtBotolBening.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t2 di Nextion
    processStorageA(buffer);  // Panggil fungsi untuk memproses hasil dengan teks yang diambil
}

void botolWarnaCallback(void *ptr) {
    char buffer[20];
    txtBotolWarna.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t4 di Nextion
    processStorageA(buffer);
}

void plastikKemasanCallback(void *ptr) {
    Serial.println("Gambar Plastik Kemasan Ditekan");
    char buffer[20];
    txtPlastikKemasan.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t6 di Nextion
    processStorageA(buffer);
}

void tutupBotolCallback(void *ptr) {
    Serial.println("Gambar Tutup Botol Ditekan");
    char buffer[20];
    txtTutupBotol.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t8 di Nextion
    processStorageA(buffer);
}

/*-----------------------
 Tombol Sub Jenis KERTAS
------------------------*/
void arsipCallback(void *ptr) {
    Serial.println("Arsip Ditekan");
    char buffer[20];
    txtArsip.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t2 di Nextion
    processStorageB(buffer);  // Panggil fungsi penyetoran dengan subjenis yang didapatkan dari Nextion
}

void tettraPackCallback(void *ptr) {
    Serial.println("Tettra Pack Ditekan");
    char buffer[20];
    txtTettraPack.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t4 di Nextion
    processStorageB(buffer);
}

void kardusCallback(void *ptr) {
    Serial.println("Kardus Ditekan");
    char buffer[20];
    txtKardus.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t6 di Nextion
    processStorageB(buffer);
}

void majalahCallback(void *ptr) {
    Serial.println("Majalah Ditekan");
    char buffer[20];
    txtMajalah.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t8 di Nextion
    processStorageB(buffer);
}


/*-----------------------
 Tombol Sub Jenis LOGAM
------------------------*/
void sengCallback(void *ptr) {
    Serial.println("Seng Ditekan");
    char buffer[20];
    txtSeng.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t2 di Nextion
    processStorageC(buffer);  // Panggil fungsi penyetoran dengan subjenis yang didapatkan dari Nextion
}

void besiCallback(void *ptr) {
    Serial.println("Besi Ditekan");
    char buffer[20];
    txtBesi.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t4 di Nextion
    processStorageC(buffer);
}

void aluminiumCallback(void *ptr) {
    Serial.println("Aluminium Ditekan");
    char buffer[20];
    txtAluminium.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t6 di Nextion
    processStorageC(buffer);
}

void tembagaCallback(void *ptr) {
    Serial.println("Tembaga Ditekan");
    char buffer[20];
    txtTembaga.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t8 di Nextion
    processStorageC(buffer);
}


/*-----------------------
 Tombol Sub Jenis KACA
------------------------*/
void belingCallback(void *ptr) {
    Serial.println("Beling Ditekan");
    char buffer[20];
    txtBeling.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t2 di Nextion
    processStorageD(buffer);  // Panggil fungsi penyetoran dengan subjenis yang didapatkan dari Nextion
}

void botolKecapCallback(void *ptr) {
    Serial.println("Botol Kecap Ditekan");
    char buffer[20];
    txtBotolKecap.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t4 di Nextion
    processStorageD(buffer);
}

void botolUtuhCallback(void *ptr) {
    Serial.println("Botol Utuh Ditekan");
    char buffer[20];
    txtBotolUtuh.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t6 di Nextion
    processStorageD(buffer);
}

void botolHijauCallback(void *ptr) {
    Serial.println("Botol Hijau Ditekan");
    char buffer[20];
    txtBotolHijau.getText(buffer, sizeof(buffer));  // Ambil teks dari komponen t8 di Nextion
    processStorageD(buffer);
}




void tidakCallback(void *ptr) {
    Serial.println("Tombol Tidak Ditekan");
    // Aksi untuk tidak
}

void yaCallback(void *ptr) {
    Serial.println("Tombol Ya Ditekan");
    // Aksi untuk ya
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
    txtSaldo.setText("WiFi Error");
  }

  // Pindah ke halaman info saldo di HMI
  pageInfoSaldo.show();
}

// Fungsi untuk memproses tarik tunai dan mengupdate saldo di database
void processTarikTunai() {
  uint32_t numberValue;
  numInputTarik.getValue(&numberValue);  // Mendapatkan nilai angka dari komponen Nextion "numInputTarik"
  float amount = (float)numberValue;  // Konversi nilai menjadi tipe float untuk perhitungan

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
        txtMutasiRekening.setText(mutasiText.c_str());
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
    txtMutasiRekening.setText("WiFi Error");
  }

  // Pindah ke halaman mutasi rekening di HMI
  pageMutasiRekening.show();
}

