#include <WiFi.h>
#include <WiFiUdp.h>
#include <M5GFX.h>
#include <M5Unified.h>

// Wi-Fi credentials and UDP port
const char* ssid = "RCAP";
const char* password = "eecs1021";
const int PORT = 8888;

// Static IP configuration
IPAddress esp32_IP(192,168,4,2);
IPAddress local_IP(192,168,4,1); //Static ip address
IPAddress gateway(192,168,4,1); //AP ip address
IPAddress subnet(255, 255, 255, 0); //First 3 numbers are the same in the same network

WiFiUDP udp;

// M5core2 display object
M5GFX display;

// Buffers and state variables for receiving JPEG frames over UDP
static uint8_t* jpgBuf = nullptr;
static uint8_t* packetBuf = nullptr;
static const size_t MAX_SIZE = 6000;
static const size_t BUFFER_SIZE = 501;
int jpgBufferIndex = 0;
int packetSize = 0;

// Pins for two potentiometers
const int PORTB_PIN = 36;
const int PORTA_PIN = 33;

void setup() {
  // Initializes M5Core2 hardware
  M5.begin();

  // USB serial for debugging
  Serial.begin(115200);

  // Configure potentiometer pins as inputs
  pinMode(PORTB_PIN, INPUT);
  pinMode(PORTA_PIN, INPUT);
  
  // Set M5Core2 to Access Point mode
  WiFi.mode(WIFI_AP);

  // Assign static IP settings
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  // Start UDP listener on 8888
  udp.begin(PORT);

  IPAddress IP = WiFi.softAPIP();

  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Reserves space in memory to build image from packets
  jpgBuf = (uint8_t*)ps_malloc(MAX_SIZE); //allocates size in PSRAM

  // Reserves space in memory to receive image packets from ESP32
  packetBuf = (uint8_t*)ps_malloc(BUFFER_SIZE);

  // Starts M5Core2 display
  display.begin();

}

// Displays JPEG image on display
void displayImage(){
  display.drawJpg(jpgBuf, ~0u, 0, 0
              , 320  // Width
              , 240 // Height
              , 0    // X offset
              , 0    // Y offset
              , 0  // X magnification(default = 1.0 , 0 = fitsize , -1 = follow the Y magni)
              , 0  // Y magnification(default = 1.0 , 0 = fitsize , -1 = follow the X magni)
              , datum_t::top_left
              );
}

void loop() {

  // Check if a UDP packet has been received
  packetSize = udp.parsePacket();

  // Assembles incoming packets into JPEG frame
  // Uses a simple count check system that verifies the number of packets received is equal to the amount expected
  if (packetSize){
    int id = udp.peek(); // First byte indicates packet ID

    // If its the first packet of an image, reset the packet counter and add packet data to image buffer
    if (id == 0){
      jpgBufferIndex = 0;
      udp.read(packetBuf, packetSize);
      memcpy(jpgBuf, packetBuf + 1, packetSize);
    } 
    
    // Adds packets to image buffer and increments packet counter
    else if (id == jpgBufferIndex + 1){
      jpgBufferIndex++;
      udp.read(packetBuf, packetSize);
      memcpy(jpgBuf + jpgBufferIndex * (BUFFER_SIZE - 1), packetBuf + 1, packetSize);

    } 
    
    // End of image signal, compiles and displays image
    else if (id == 100){
      udp.read(packetBuf, packetSize);

      if (packetBuf[1] == jpgBufferIndex + 1){
        displayImage();
      }

    }else{ // Case is usually never reached, usually for trash
      udp.flush();// If the stack becomes too large it will crash
    }

  }

  // Reads the analog value of the potentiometers
  int bPtRawValue = analogRead(PORTB_PIN);
  int aPtRawValue = analogRead(PORTA_PIN);

  // Scale raw values from to 0-100
  int bPtValue = (bPtRawValue/ 4095.0) *100;
  int aPtValue = (aPtRawValue/ 4095.0) *100;

  // Send both values to ESP32
  udp.beginPacket(esp32_IP, PORT);
  udp.write(bPtValue);
  udp.write(aPtValue);
  udp.endPacket();

  Serial.println(bPtValue);
  Serial.println(aPtValue);
  

  delay(10); // 10ms delay keeps communication stable

}
