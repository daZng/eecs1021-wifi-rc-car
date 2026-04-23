#include <SparkFun_TB6612.h>

// Pins for UART communication with ESP32
#define TX2 0
#define RX2 1

// Motor A control pins
#define AIN1 2
#define AIN2 3
#define PWMA 4

// Motor B control pins
#define BIN1 6
#define BIN2 7
#define PWMB 8

#define STBY 5

// Create motor objects
Motor m1 = Motor(AIN1, AIN2, PWMA, -1, STBY);
Motor m2 = Motor(BIN1, BIN2, PWMB, -1, STBY);

void setup() {

  // Set PWM frequency and range for motor speed control
  analogWriteFreq(20000);
  analogWriteRange(255);

  // Turn on built-in LED to show the board is running
  digitalWrite(LED_BUILTIN, HIGH);

  // Start USB serial for debugging
  Serial.begin(115200);

  // Configure Serial1 RX/TX pins
  Serial1.setRX(RX2);
  Serial1.setTX(TX2);

  // Start Serial1 for receiving motor commands from ESP32
  Serial1.begin(115200);

  Serial.println("Starting");
}

void loop() {
  // Buffer to store 2 incoming bytes:
  // buffer[0] = motor 2 command
  // buffer[1] = motor 1 command
  uint8_t buffer[2];

  // Check whether serial data is available
  if (Serial1.available()){
    // Read 2 bytes from Serial 1 into the buffer
    Serial1.readBytes(buffer, 2);
    
    // Convert incoming bytes into integer speed values
    int speed1 = (int)(buffer[1]);
    int speed2 = (int)(buffer[0]);

    double convertedSpeed1;
    double convertedSpeed2;

    // Convert 0-100 input range into -255 to 255 motor range
    convertedSpeed1 = -((speed1 - 50)/50.0) *255;
    convertedSpeed2 = -((speed2 - 50)/50.0) *255;

    // Drive motors using converted speed values
    m2.drive((int)convertedSpeed2);
    m1.drive((int)convertedSpeed1);

  }

  // Small delay for loop stability
  delay(10);
}