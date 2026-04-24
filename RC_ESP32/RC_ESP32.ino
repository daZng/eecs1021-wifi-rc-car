#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

// Wi-Fi credentials and UDP port
const char* ssid = "RCAP";
const char* password = "eecs1021";
const int PORT = 8888;

// Maximum payload bytes sent per UDP packet
const int PACKET_SIZE = 500;

// Delay between UDP packets (0 = send as fast as possible)
const int UDP_DELAY = 0;

// Stores size of latest received UDP packet
int packetSize = 0;

// Static IP configuration
IPAddress local_IP(192,168,4,2); //Static ip address
IPAddress gateway(192,168,4,1); //AP ip address
IPAddress subnet(255, 255, 255, 0);

WiFiUDP udp;

// LED state tracker
int ledState = 1;

// External LED pin that turns on when not connected
#define LED_PIN 33

// RX and TX pins for UART communication with Pico 2W
#define RX_PIN 12
#define TX_PIN 14

// ------------ Camera pin definitions --------------
// WROVER-KIT PIN Map
#define CAM_PIN_PWDN    -1 //power down is not used
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    21
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      19
#define CAM_PIN_D2      18
#define CAM_PIN_D1       5
#define CAM_PIN_D0       4
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// Camera configuration structure
static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QQVGA,//QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 22, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1, //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY //CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

// Initializes the camera hardware
esp_err_t camera_init(){
    //power up the camera if PWDN pin is defined
    if(CAM_PIN_PWDN != -1){
        pinMode(CAM_PIN_PWDN, OUTPUT);
        digitalWrite(CAM_PIN_PWDN, LOW);
    }

    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    // Get sensor handle to change settings: reverse image
    sensor_t * s = esp_camera_sensor_get();
    if (s != NULL){
      s->set_vflip(s, 1);   // 1 = Flip Vertically, 0 = Disabled
    }

    return ESP_OK;
}

// Captures one image fram and sends it over UDP
esp_err_t camera_capture(){

    // Acquire a frame from camera driver
    camera_fb_t * fb = esp_camera_fb_get();

    // If capture failed, return error
    if (!fb) {
        ESP_LOGE(TAG, "Camera Capture Failed");
        return ESP_FAIL;
    }

    // Send the JPEG image buffer over UDP
    send_image(fb->buf, fb->len);
  
    // Return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    return ESP_OK;
}

// Splits image data into multiple UDP packets and sends them
void send_image(uint8_t* bufferPointer, int length){

  // Number of packets needed for whole JPEG
  int numPackets = ceil((double)length/PACKET_SIZE);

  for (int i=0; i<numPackets; i++){
    int packetLength = PACKET_SIZE;

    // Last packet may be smaller than PACKET_SIZE
    if (length - packetLength * i < PACKET_SIZE){
      packetLength = length % PACKET_SIZE;
    }

    // Send one packet:
    // first byte = packet index
    // remaining bytes = image data chunk
    udp.beginPacket(gateway, PORT);
    udp.write((uint8_t)i);
    udp.write(bufferPointer + i*PACKET_SIZE, packetLength);
    udp.endPacket();

    delay(UDP_DELAY);
  }
  
  // Send end of image marker packet
  // The value 100 identifies end of image
  // numPackets tells receiver how many packets belonged to this frame
  udp.beginPacket(gateway, PORT);
  udp.write((uint8_t)100); //identifier for end of image
  udp.write(numPackets);
  udp.endPacket();

  delay(UDP_DELAY);
}

void setup() {
  // Configure status LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  // USB serial for debugging
  Serial.begin(115200);

  // Hardware serial to Pico 2W
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Wait for serial monitor connection
  while (!Serial){
    delay(10);
  }

  // Keep trying to initialize camera until initialized
  while(camera_init()){
    delay(100);
  }

  Serial.println("Configuring WIFI settings.");

  // Assign static IP settings
  WiFi.config(local_IP, gateway, subnet);

  Serial.println("Connecting");

  // Reset any previous Wi-Fi states
  WiFi.disconnect(true);   // drop connection + clear config
  delay(200);

  // Set ESP32 to station mode
  WiFi.mode(WIFI_STA);
  delay(200);

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Assigning PORT.");

  // Start UDP listener on 8888
  udp.begin(PORT);

  Serial.println("Connected.");

  // Turn LED on/off to indicate Wi-Fi status
  ledState = 0;
  digitalWrite(LED_PIN, ledState);

}

void loop() {
  // Capture and transmit one image frame
  camera_capture();
  
  // Check if a UDP packet has been received
  packetSize = udp.parsePacket();

  // Buffer for incoming 2-byte command
  char buffer[2] = {0, 0};

  if (packetSize){
    udp.read(buffer, 2);
  }

  // Forward received command to Pico 2W
  Serial2.write((uint8_t*)buffer, sizeof(buffer));

  // Update external LED depending on Wi-Fi connection state
  if (WiFi.status() != WL_CONNECTED && ledState == 0){
    ledState = 1;
    digitalWrite(LED_PIN, ledState);
  } else if (WiFi.status() == WL_CONNECTED && ledState == 1){
    ledState = 0;
    digitalWrite(LED_PIN, ledState);
  }

  delay(10);
}
