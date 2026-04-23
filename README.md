# eecs1021-wifi-rc-car
A Wi-Fi controlled RC car build for EECS 1021 using ESP32, M5Core2, Raspberry Pi Pico 2W, featuring live video transmission and wireless control.

## Overview

This project implements a wireless RC car system with live camera feedback and remote motor control. The system is split across multiple devices:

- **ESP32-CAM** for capturing and transmitting live video as well as sending motor control commands to Pico 2W
- **M5Core2** for receiving and displaying the video stream while also reading potentiometer inputs
- **Raspberry Pi 2W + TB6612FNG motor driver** for controlling the car’s motors based on the received motor control commands

## Features

- Wi-Fi based communication between M5Core2 and ESP32-CAM
- Live video streaming from the ESP32-CAM
- Real-time potentiometer-based control
- Differential motor control for the RC car
- UDP packet-based communication for low-latency transmission

## System Architecture

### ESP32-CAM
- Captures JPEG images
- Splits image data into UDP packets
- Sends packets wirelessly to the M5Core2
- Receives motor control commands to send to Raspberry Pi 2W

### M5Core2
- Operates as a Wi-Fi access point
- Receives and reconstructs image packets
- Displays live video on screen
- Reads potentiometer input
- Sends motor control values to the ESP32-CAM

### Raspberry Pi Pico 2W
- Receives 2-byte control values over serial
- Maps joystick values to motor speeds
- Drives two DC motors through the TB6612FNG motor driver

## Hardware Used

- ESP32-CAM
- M5Core2
- TB6612 motor driver
- 2 N20 DC 600RPM motors
- 2 half breadboards
- Jumper wires
- RC car chassis
- 2 potentiometers
- 12V Battery pack (8x AA batteries)
- 4.5V Battery pack (3x AA batteries)

## Software / Libraries

- Arduino IDE
- `WiFi.h`
- `WiFiUdp.h`
- `esp_camera.h`
- `M5GFX.h`
- `M5Unified.h`
- `SparkFun_TB6612.h`

## How It Works

1. The M5Core2 creates a Wi-Fi access point.
2. The ESP32-CAM connects to the M5Core2 over Wi-Fi.
3. The ESP32-CAM captures images and sends them as UDP packets.
4. The M5Core2 receives the packets, reconstructs the JPEG frame, and displays it.
5. The M5Core2 reads analog potentiometer values and sends control bytes to the ESP32-CAM.
6. ESP32-CAM forwards the control bytes to Raspberry Pi 2W.
7. Raspberry Pi 2W converts those values into left and right motor speeds to the motor driver to drive the car.

## File Structure

- `RC_ESP32.ino` – camera capture, UDP image transmission, and potentiometer input transmission
- `RC_M5.ino` – video reception, display, and potentiometer input transmission
- `RC_Pico.ino` – potentiometer reception, motor control using the TB6612FNG driver

## Notes

- UDP is used for speed and low latency, so occasional packet loss may occur.
- JPEG image data is split into multiple packets before transmission.
- Potentiometer values are mapped so that the center position corresponds to zero motor speed.

Course project developed for **EECS 1021**.
