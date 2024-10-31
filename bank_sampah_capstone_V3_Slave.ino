#include <Wire.h>
#include <Servo.h>
#include "HX711.h"

// I2C Address for ESP32 Slave
#define I2C_ADDRESS 8

// Servo Pins
#define SERVO_A_PIN 33
#define SERVO_B_PIN 32
#define SERVO_C_PIN 25
#define SERVO_D_PIN 26

// Load Cell Pins (Each connected with an HX711)
#define LOAD_CELL_1_DT 15
#define LOAD_CELL_1_SCK 4
#define LOAD_CELL_2_DT 2
#define LOAD_CELL_2_SCK 12
#define LOAD_CELL_3_DT 14
#define LOAD_CELL_3_SCK 27
#define LOAD_CELL_4_DT 13
#define LOAD_CELL_4_SCK 16

// Servo objects
Servo servoA;
Servo servoB;
Servo servoC;
Servo servoD;

// Load cell objects
HX711 loadCell1;
HX711 loadCell2;
HX711 loadCell3;
HX711 loadCell4;

// Array to store load cell data
int loadCellData[4] = {0, 0, 0, 0};

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C as Slave
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(receiveEvent);   // Function to handle received I2C data
  Wire.onRequest(requestEvent);   // Function to send load cell data to Master when requested

  // Attach servos to their respective pins
  servoA.attach(SERVO_A_PIN);
  servoB.attach(SERVO_B_PIN);
  servoC.attach(SERVO_C_PIN);
  servoD.attach(SERVO_D_PIN);

  // Set servos to initial position (e.g., closed position)
  servoA.write(90);
  servoB.write(90);
  servoC.write(90);
  servoD.write(90);

  // Initialize load cells
  loadCell1.begin(LOAD_CELL_1_DT, LOAD_CELL_1_SCK);
  loadCell2.begin(LOAD_CELL_2_DT, LOAD_CELL_2_SCK);
  loadCell3.begin(LOAD_CELL_3_DT, LOAD_CELL_3_SCK);
  loadCell4.begin(LOAD_CELL_4_DT, LOAD_CELL_4_SCK);

  // Tare each load cell to set the zero point
  loadCell1.tare();
  loadCell2.tare();
  loadCell3.tare();
  loadCell4.tare();
}

void loop() {
  // Update load cell data periodically
  updateLoadCellData();
  delay(1000); // Adjust delay as necessary
}

// Function to update load cell data
void updateLoadCellData() {
  loadCellData[0] = loadCell1.get_units(10); // Averaging 10 readings
  loadCellData[1] = loadCell2.get_units(10);
  loadCellData[2] = loadCell3.get_units(10);
  loadCellData[3] = loadCell4.get_units(10);

  // Print the load cell data for debugging
  for (int i = 0; i < 4; i++) {
    Serial.print("Load Cell ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(loadCellData[i]);
  }
}

// This function is called whenever I2C data is received
void receiveEvent(int bytes) {
  if (Wire.available()) {
    int command = Wire.read(); // Read the command sent by ESP32 Master

    // Process the command and move the corresponding servo
    switch (command) {
      case 1:
        Serial.println("Opening Plastic Storage...");
        openServoA();
        break;
      case 2:
        Serial.println("Opening Paper Storage...");
        openServoB();
        break;
      case 3:
        Serial.println("Opening Metal Storage...");
        openServoC();
        break;
      case 4:
        Serial.println("Opening Glass Storage...");
        openServoD();
        break;
      case 5:
        Serial.println("Closing Plastic Storage...");
        closeServoA();
        break;
      case 6:
        Serial.println("Closing Paper Storage...");
        closeServoB();
        break;
      case 7:
        Serial.println("Closing Metal Storage...");
        closeServoC();
        break;
      case 8:
        Serial.println("Closing Glass Storage...");
        closeServoD();
        break;
      default:
        Serial.println("Unknown command received.");
        break;
    }
  }
}

// This function is called whenever the Master requests data
void requestEvent() {
  // Send load cell data as 2 bytes per value (int16)
  for (int i = 0; i < 4; i++) {
    Wire.write((loadCellData[i] >> 8) & 0xFF); // Send high byte
    Wire.write(loadCellData[i] & 0xFF);        // Send low byte
  }
}

// Functions to move each servo to open and close positions
void openServoA() {
  for (int pos = 90; pos <= 180; pos++) {
    servoA.write(pos);
    delay(20);  // Jeda antara setiap langkah
  }
}

void openServoB() {
  for (int pos = 90; pos <= 180; pos++) {
    servoB.write(pos);
    delay(20);  // Jeda antara setiap langkah
  }
}

void openServoC() {
  for (int pos = 90; pos <= 180; pos++) {
    servoC.write(pos);
    delay(20);  // Jeda antara setiap langkah
  }
}

void openServoD() {
  for (int pos = 90; pos <= 180; pos++) {
    servoD.write(pos);
    delay(20);  // Jeda antara setiap langkah
  }
}

void closeServoA() {
  for (int pos = 180; pos >= 90; pos--) {
    servoA.write(pos);
    delay(20);
  }
}

void closeServoB() {
  for (int pos = 180; pos >= 90; pos--) {
    servoB.write(pos);
    delay(20);
  }
}

void closeServoC() {
  for (int pos = 180; pos >= 90; pos--) {
    servoC.write(pos);
    delay(20);
  }
}

void closeServoD() {
  for (int pos = 180; pos >= 90; pos--) {
    servoD.write(pos);
    delay(20);
  }
}
