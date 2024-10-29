#include <Arduino.h>
#include <Wire.h>

#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <EasyNextionLibrary.h>  // Include Easy Nextion Library

// I2C Address for ESP32 Slave
#define I2C_ADDRESS 8

// ===== PIN DEFINITIONS =====
// rfid rc522
#define SS_PIN 15
#define RST_PIN 2
// sck 18, miso 19, mosi 23

// ultrasonic
#define TRIG_PIN_A 4
#define ECHO_PIN_A 5
#define TRIG_PIN_B 13
#define ECHO_PIN_B 12
#define TRIG_PIN_C 14
#define ECHO_PIN_C 27 
#define TRIG_PIN_D 26 
#define ECHO_PIN_D 25

// nextion
EasyNex myNex(Serial2); // pin 16 dan pin 17

// ======== DEFINE LOADCELL, SERVO, RFID READER ========
//HX711 scaleA, scaleB, scaleC, scaleD;
//Servo servoA, servoB, servoC, servoD;
MFRC522 rfid(SS_PIN, RST_PIN);


const char* ssid = "FREE BASABASI";
const char* password = "COKLATBANANA";

// RFID Card Information
String cardID = "";  
String currentUser = "";
String currentAlamat = "";
int currentSaldo = 0;
bool accessGranted = false;

// ============== WEIGHT STORAGE VALUES =================
float previousWeightA = 0, previousWeightB = 0, previousWeightC = 0, previousWeightD = 0;
float currentWeightA = 0, currentWeightB = 0, currentWeightC = 0, currentWeightD = 0;
float newWeightA = 0, newWeightB = 0, newWeightC = 0, newWeightD = 0;
// Dari ESP32 Slave
float storagePlastic = 0.0;
float storagePaper = 0.0;
float storageMetal = 0.0;
float storageGlass = 0.0;

// Fungsi pengganti halaman
int currentPage = 0;
void changePage(int page){
  currentPage = page;
  myNex.writeStr("page " + String(page));
}

// ======== SETUP =========
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA (21), SCL (22) for I2C
  Serial2.begin(9600); 
  myNex.begin();

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();

  /* Initialize Servos and Load Cells
  servoA.attach(SERVO_PIN_A);
  servoB.attach(SERVO_PIN_B);
  servoC.attach(SERVO_PIN_C);
  servoD.attach(SERVO_PIN_D);

  servoA.write(90);  // Closed position
  servoB.write(90);
  servoC.write(90);
  servoD.write(90);

  scaleA.begin(LOADCELL_DOUT_PIN_A, LOADCELL_SCK_PIN_A);
  scaleB.begin(LOADCELL_DOUT_PIN_B, LOADCELL_SCK_PIN_B);
  scaleC.begin(LOADCELL_DOUT_PIN_C, LOADCELL_SCK_PIN_C);
  scaleD.begin(LOADCELL_DOUT_PIN_D, LOADCELL_SCK_PIN_D);

  scaleA.set_scale(2280.f); 
  scaleB.set_scale(2280.f);
  scaleC.set_scale(2280.f);
  scaleD.set_scale(2280.f);

  scaleA.tare();
  scaleB.tare();
  scaleC.tare();
  scaleD.tare(); */

  // Ultrasonic Sensor Setup
  pinMode(TRIG_PIN_A, OUTPUT);
  pinMode(ECHO_PIN_A, INPUT);
  pinMode(TRIG_PIN_B, OUTPUT);
  pinMode(ECHO_PIN_B, INPUT);
  pinMode(TRIG_PIN_C, OUTPUT);
  pinMode(ECHO_PIN_C, INPUT);
  pinMode(TRIG_PIN_D, OUTPUT);
  pinMode(ECHO_PIN_D, INPUT);

  // Setup EasyNex
  changePage(0);  // Display the login page at startup
}
String currentSubType = "";
String currentType = "";
// ====================== LOOP ======================
void loop() {
  myNex.NextionListen();  // Listen for Nextion events

  // Check RFID for login access
  if (!accessGranted) {
    if (checkRFID()) {
        if(loginRFID(cardID)){
            accessGranted = true;

            Serial.print("Nama: ");
            Serial.println(currentUser);
            Serial.print("Alamat: ");
            Serial.println(currentAlamat);
            Serial.print("Saldo: ");
            Serial.println(currentSaldo);

            myNex.writeStr("t1.txt", currentUser);
            myNex.writeStr("t2.txt", currentAlamat);
            delay(1000);
            changePage(1);   
        }
        else {
            Serial.println("Kartu tidak terdaftar.");
            myNex.writeStr("t1.txt", cardID);
            myNex.writeStr("t2.txt", "Kartu tidak terdaftar");
            delay(2000);
        }
    }
  }

  // Update storage values based on load cell data ================================= NEW
  updateStorageValues();

  int newPage = myNex.readNumber("dp");
  Serial.print(newPage);
  Serial.print(" ");
  delay(50);
  if(true){
    switch(newPage){
      case 0:
        Logout();
        break;
      case 2:
        infoSaldo();
        break;
      case 3:{
        int amount = myNex.readNumber("tarik_tunai.n0.val");
        int buttonPressed = myNex.readNumber("tarik_tunai.b4.val");
        Serial.print("\n " + String(buttonPressed) + " ");
        if(buttonPressed == 1){
          tarikTunai(amount);
          myNex.writeNum("tarik_tunai.b4.val", 0);
        }
        break;
      }
      case 5:{
        mutasiRekening();
        break;
      }
      case 7:{
        currentType = "plastik";
        if (myNex.readNumber("setor_plastik.p0.val") == 1) {  // Botol Bening
            Serial.println("Gambar Botol Bening Ditekan");
            String buffer = myNex.readStr("setor_plastik.t2.txt");
            currentSubType = buffer;
            Wire.write(1);
            myNex.writeNum("setor_plastik.p0.val", 0);
        }

        if (myNex.readNumber("setor_plastik.p1.val") == 1) {  // Botol Warna
            Serial.println("Gambar Botol Warna Ditekan");
            String buffer = myNex.readStr("setor_plastik.t4.txt");
            currentSubType = buffer;
            Wire.write(1);
            myNex.writeNum("setor_plastik.p1.val", 0);
        }

        if (myNex.readNumber("setor_plastik.p2.val") == 1) {  // Plastik Kemasan
            Serial.println("Gambar Plastik Kemasan Ditekan");
            String buffer = myNex.readStr("setor_plastik.t6.txt");
            currentSubType = buffer;
            Wire.write(1);
            myNex.writeNum("setor_plastik.p2.val", 0);
        }

        if (myNex.readNumber("setor_plastik.p3.val") == 1) {  // Tutup Botol
            Serial.println("Gambar Tutup Botol Ditekan");
            String buffer = myNex.readStr("setor_plastik.t8.txt");
            currentSubType = buffer;
            Wire.write(1);
            myNex.writeNum("setor_plastik.p3.val", 0);
        }
        break;
      }
      case 8:{
        currentType = "kertas";
        if (myNex.readNumber("setor_kertas.p0.val") == 1) {  // Arsip
            Serial.println("Gambar Arsip Ditekan");
            String buffer = myNex.readStr("setor_kertas.t2.txt");
            currentSubType = buffer;
            Wire.write(2);
            myNex.writeNum("setor_kertas.p0.val", 0);
        }

        if (myNex.readNumber("setor_kertas.p1.val") == 1) {  // Tetra Pack
            Serial.println("Gambar Tetra Pack Ditekan");
            String buffer = myNex.readStr("setor_kertas.t4.txt");
            currentSubType = buffer;
            Wire.write(2);
            myNex.writeNum("setor_kertas.p1.val", 0);
        }

        if (myNex.readNumber("setor_kertas.p2.val") == 1) {  // Kardus
            Serial.println("Gambar Kardus Ditekan");
            String buffer = myNex.readStr("setor_kertas.t6.txt");
            currentSubType = buffer;
            Wire.write(2);
            myNex.writeNum("setor_kertas.p2.val", 0);
        }

        if (myNex.readNumber("setor_kertas.p3.val") == 1) {  // Majalah
            Serial.println("Gambar Majalah Ditekan");
            String buffer = myNex.readStr("setor_kertas.t8.txt");
            currentSubType = buffer;
            Wire.write(2);
            myNex.writeNum("setor_kertas.p3.val", 0);
        }
        break;
      }
      case 9:{
        currentType = "logam";
        if (myNex.readNumber("setor_logam.p0.val") == 1) {  // Seng
            Serial.println("Gambar Seng Ditekan");
            String buffer = myNex.readStr("setor_logam.t2.txt");
            currentSubType = buffer;
            Wire.write(3);
            myNex.writeNum("setor_logam.p0.val", 0);
        }

        if (myNex.readNumber("setor_logam.p1.val") == 1) {  // Besi
            Serial.println("Gambar Besi Ditekan");
            String buffer = myNex.readStr("setor_logam.t4.txt");
            currentSubType = buffer;
            Wire.write(3);
            myNex.writeNum("setor_logam.p1.val", 0);
        }

        if (myNex.readNumber("setor_logam.p2.val") == 1) {  // Aluminium
            Serial.println("Gambar Aluminium Ditekan");
            String buffer = myNex.readStr("setor_logam.t6.txt");
            currentSubType = buffer;
            Wire.write(3);
            myNex.writeNum("setor_logam.p2.val", 0);
        }

        if (myNex.readNumber("setor_logam.p3.val") == 1) {  // Tembaga
            Serial.println("Gambar Tembaga Ditekan");
            String buffer = myNex.readStr("setor_logam.t8.txt");
            currentSubType = buffer;
            Wire.write(3);
            myNex.writeNum("setor_logam.p3.val", 0);
        }
        break;
      }
      case 10:{
        currentType = "kaca";
        if (myNex.readNumber("setor_kaca.p0.val") == 1) {  // Beling
            Serial.println("Gambar Beling Ditekan");
            String buffer = myNex.readStr("setor_kaca.t2.txt");
            currentSubType = buffer;
            Wire.write(4);
            myNex.writeNum("setor_kaca.p0.val", 0);
        }

        if (myNex.readNumber("setor_kaca.p1.val") == 1) {  // Botol Kecap
            Serial.println("Gambar Botol Kecap Ditekan");
            String buffer = myNex.readStr("setor_kaca.t4.txt");
            currentSubType = buffer;
            Wire.write(4);
            myNex.writeNum("setor_kaca.p1.val", 0);
        }

        if (myNex.readNumber("setor_kaca.p2.val") == 1) {  // Botol Utuh
            Serial.println("Gambar Botol Utuh Ditekan");
            String buffer = myNex.readStr("setor_kaca.t6.txt");
            currentSubType = buffer;
            Wire.write(4);
            myNex.writeNum("setor_kaca.p2.val", 0);
        }

        if (myNex.readNumber("setor_kaca.p3.val") == 1) {  // Botol Hijau
            Serial.println("Gambar Botol Hijau Ditekan");
            String buffer = myNex.readStr("setor_kaca.t8.txt");
            currentSubType = buffer;
            Wire.write(4);
            myNex.writeNum("setor_kaca.p3.val", 0);
        }
        break;
      case 11:
        //updateStorageValues()
        //if(detectItems()){
          //jumlahBarang += 1;
        //}
        if (myNex.readNumber("konfirm_setor.b4.val") == 1){
          if(currentType == "plastik"){
            processStorageA(currentSubType);
            Wire.write(5);
          }
          else if(currentType == "kertas"){
            processStorageB(currentSubType);
            Wire.write(6);
          }
          else if(currentType == "logam"){
            processStorageC(currentSubType);
            Wire.write(7);
          }
          else if(currentType == "kaca"){
            processStorageD(currentSubType);
            Wire.write(8);
          }
          myNex.writeNum("konfirm_setor.b4.val", 0);
        }
      }

    }
    currentPage = newPage;
  }
}

// Mengambil data RFID nasabah dari database untuk autentikasi
boolean loginRFID(String rfid){
    HTTPClient http;

    String serverUrl = "https://banksampah-be.vercel.app/user/login/";

    http.begin(serverUrl + rfid); // Initialize HTTP connection
    int httpResponseCode = http.GET(); // Send the request

    if (httpResponseCode == 200) { // Check if request was successful
      String payload = http.getString();
      Serial.println("HTTP Response Code: " + String(httpResponseCode));
      Serial.println("Response payload: " + payload);

      // Parse JSON payload
      StaticJsonDocument<1024> jsonDoc; // Adjust the size based on JSON response size
      DeserializationError error = deserializeJson(jsonDoc, payload);

      if (error) {
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
        return false;
      }

      // Access JSON data (Assuming JSON format is {"key1": "value1", "key2": "value2"})
      currentUser = jsonDoc["nama"].as<String>();;
      currentAlamat = jsonDoc["alamat"].as<String>();;
      currentSaldo = jsonDoc["saldo"].as<int>();;

      return true;
      http.end();
    } 
    else {
      Serial.println("Error on HTTP request: " + String(httpResponseCode));
      return false;
      http.end();
    }
    http.end(); // Close the connection  
}


// Function to check the RFID card
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID += String(rfid.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();

  Serial.print("Card detected with ID: ");
  Serial.println(cardID);

  return true;
}

// Log out
void Logout(){
  accessGranted = false;
  currentUser = "";
  currentAlamat = "";
  currentSaldo = 0;
  myNex.writeStr("login.t1.txt", "");
  myNex.writeStr("login.t2.txt", "");
}

// deteksi item di sensor ultrasonik
int detectItems(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);
  int distance = duration * 0.034 / 2;  // Distance in cm

  return (distance < 10) ? 1 : 0;
}

// Functions to handle waste type selection
void processStorageA(const String &subtype) {
  //if (detectItems(TRIG_PIN_A, ECHO_PIN_A) > 0) {
    //servoA.write(180);  // Open cover (180 degrees)
    //Wire.write(1); // Send a command to open(1 for open Plastic)
    delay(500);
    currentWeightA = storagePlastic;
    newWeightA = currentWeightA - previousWeightA;
    previousWeightA = currentWeightA;
    processResultPlastic(newWeightA, subtype);
  //}
}

void processStorageB(const String &subtype) {
  if (detectItems(TRIG_PIN_B, ECHO_PIN_B) > 0) {
    //servoB.write(180);
   // Wire.write(2); // Send a command to open
    delay(500);
    currentWeightB = storagePaper;
    newWeightB = currentWeightB - previousWeightB;
    previousWeightB = currentWeightB;
    processResultPaper(newWeightB, subtype);
  }
}

void processStorageC(const String &subtype) {
  if (detectItems(TRIG_PIN_C, ECHO_PIN_C) > 0) {
    //servoC.write(180);
    //Wire.write(3); // Send a command to open
    delay(500);
    currentWeightC = storageMetal;
    newWeightC = currentWeightC - previousWeightC;
    previousWeightC = currentWeightC;
    processResultMetal(newWeightC, subtype);
  }
}

void processStorageD(const String &subtype) {
  if (detectItems(TRIG_PIN_D, ECHO_PIN_D) > 0) {
    //servoD.write(180);
    //Wire.write(4); // Send a command to open
    delay(500);
    currentWeightD = storageGlass;
    newWeightD = currentWeightD - previousWeightD;
    previousWeightD = currentWeightD;
    processResultGlass(newWeightD, subtype);
  }
}

// Result processing functions for each waste type
void processResultPlastic(float newWeight, const String &subtype) {
  float pricePerKg = 0;

  if (subtype == "PET Botol Bening") pricePerKg = 5500;
  else if (subtype == "PET Botol Warna") pricePerKg = 1500;
  else if (subtype == "Plastic Kemasan") pricePerKg = 400;
  else if (subtype == "Tutup Botol") pricePerKg = 3000;

  float totalPrice = newWeight * pricePerKg;

  myNex.writeStr("info_setor.t3", subtype);
  myNex.writeStr("info_setor.t4", String(newWeight, 2) + " kg");
  myNex.writeStr("info_setor.t5", "Rp " + String(totalPrice, 2));

  changePage(12);   // Display the result page
}

void processResultPaper(float newWeight, const String &subtype) {
  float pricePerKg = 0;

  if (subtype == "Arsip (hvs, buku)") pricePerKg = 1800;
  else if (subtype == "Tetra Pack") pricePerKg = 300;
  else if (subtype == "Kardus") pricePerKg = 1600;
  else if (subtype == "Majalah/koran") pricePerKg = 500;
  
  float totalPrice = newWeight * pricePerKg;

  myNex.writeStr("info_setor.t3", subtype);
  myNex.writeStr("info_setor.t4", String(newWeight, 2) + " kg");
  myNex.writeStr("info_setor.t5", "Rp " + String(totalPrice, 2));

  changePage(12);   // Display the result page
}

void processResultMetal(float newWeight, const String &subtype) {
  float pricePerKg = 0;

  if (subtype == "Seng (kaleng)") pricePerKg = 2000;
  else if (subtype == "Besi (paku, dll)") pricePerKg = 3500;
  else if (subtype == "Aluminium") pricePerKg = 10000;
  else if (subtype == "Tembaga (kawat)") pricePerKg = 40000;

  float totalPrice = newWeight * pricePerKg;

  myNex.writeStr("info_setor.t3", subtype);
  myNex.writeStr("info_setor.t4", String(newWeight, 2) + " kg");
  myNex.writeStr("info_setor.t5", "Rp " + String(totalPrice, 2));

  changePage(12);   // Display the result page
}

void processResultGlass(float newWeight, const String &subtype) {
  float pricePerKg = 0;

  if (subtype == "Beling (pecahan)") pricePerKg = 200;
  else if (subtype == "Botol Kecap") pricePerKg = 300;
  else if (subtype == "Botol Utuh") pricePerKg = 400;
  else if (subtype == "Botol Hijau") pricePerKg = 500;

  float totalPrice = newWeight * pricePerKg;

  myNex.writeStr("info_setor.t3", subtype);
  myNex.writeStr("info_setor.t4", String(newWeight, 2) + " kg");
  myNex.writeStr("info_setor.t5", "Rp " + String(totalPrice, 2));

  changePage(12);  // Display the result page
}

void infoSaldo() {
    myNex.writeStr("info_saldo.t1.txt", String(currentSaldo));
}

void tarikTunai(int amount) {
  Serial.println("\nTarik Tunai");
  if (amount > currentSaldo) {
    Serial.println("Saldo tidak cukup untuk tarik tunai");
    myNex.writeStr("tarik_tunai.t2.txt", "Saldo tidak cukup untuk tarik tunai");
    return;  
  } 
  HTTPClient http;

  String serverUrl = "https://banksampah-be.vercel.app/user/tarik";

  http.begin(serverUrl); // Initialize HTTP connection
  http.addHeader("Content-Type", "application/json"); // Specify JSON content type

    // Create JSON object
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["rfid_user"] = cardID; // Replace with actual RFID data
  jsonDoc["nominal"] = amount;       // Replace with actual saldo

    // Serialize JSON to string
  String requestBody;
  serializeJson(jsonDoc, requestBody);

    // Send HTTP POST request
  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode == 201) { // Check if request was successful
    String payload = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response payload: " + payload);
    changePage(4);
  }
}

// belum diedit============================================================
void mutasiRekening() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://your-api.com/getMutasi?rfid=" + cardID;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        String mutasiText = "Mutasi Transaksi:\n";
        for (JsonObject item : doc["mutasi"].as<JsonArray>()) {
          mutasiText += item["tanggal"].as<String>() + ", " + item["transaksi"].as<String>() + ", Rp " + item["jumlah"].as<String>() + "\n";
        }
        myNex.writeStr("mutasi.t2", mutasiText);
      }
    }
    http.end();
  } else {
    myNex.writeStr("mutasi.t2", "WiFi Error");
  }
  myNex.writeStr("page mutasi");
}


