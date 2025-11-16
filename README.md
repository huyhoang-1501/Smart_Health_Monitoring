<div align="center">
  <h1>Smart Health Monitoring System</h1>
  <p>
    <img src="https://img.shields.io/badge/Microcontroller-ESP32-blueviolet?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32">
    <img src="https://img.shields.io/badge/Sensor-MAX30100-red?style=for-the-badge&logo=heartbeat&logoColor=white" alt="MAX30100">
    <img src="https://img.shields.io/badge/Sensor-MPU6050-orange?style=for-the-badge&logo=accelerometer&logoColor=white" alt="MPU6050">
    <img src="https://img.shields.io/badge/Display-SSD1306%20OLED-blue?style=for-the-badge&logo=display&logoColor=white" alt="OLED">
    <img src="https://img.shields.io/badge/Keypad-4x4%20Matrix-green?style=for-the-badge&logo=keyboard&logoColor=white" alt="Keypad">
    <img src="https://img.shields.io/badge/GSM-A7682S-yellow?style=for-the-badge&logo=signal&logoColor=white" alt="A7682S">
    <img src="https://img.shields.io/badge/Cloud-Firebase-critical?style=for-the-badge&logo=firebase&logoColor=white" alt="Firebase">
    <img src="https://img.shields.io/badge/Language-C%2B%2B-blue?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  </p>
  ---
  <p>
    <a href="#-overview">Overview</a> •
    <a href="#-key-features">Key Features</a> •
    <a href="#-setup-guide">Setup Guide</a> •
    <a href="#-web-dashboard">Web Dashboard</a> •
    <a href="#-development-team">Development Team</a>
  </p>
  ---
</div>
<br>

## Overview
**`Smart Health Monitoring System`** is an **intelligent health monitoring solution** built on **ESP32**, designed specifically for **elderly care**. It continuously tracks vital signs and **automatically alerts emergencies**.

The system measures:
- **Heart Rate (HR)** and **Blood Oxygen (SpO₂)** via **MAX30100**
- **Step Count** using **MPU6050** accelerometer
- **Real-time data upload** to **Firebase Realtime Database**
- **Auto emergency call** on abnormality detection (via **A7682S GSM module**)
- **Live display** on **128x64 OLED (SPI)**
- **Emergency number input** via **4x4 keypad**
- **Remote monitoring** through a **Web Dashboard** with real-time charts

> **Use Case**: Home health monitoring for seniors — early detection of heart issues & sedentary behavior.

<br>

## Key Features

| Feature | Description |
|--------|-------------|
| **Heart Rate & SpO₂ Monitoring** | Updated every 60 seconds, shown instantly on OLED |
| **Step Counting** | Moving average filter + threshold-based step detection |
| **Health Alert System** | Auto-triggered when: <br> • HR < 50 or > 100 (≥3 times) <br> • Steps ≤ 80 (≥2 times) |
| **Emergency Auto-Call** | Dials saved number after **90 seconds of continuous alert** |
| **4x4 Keypad Control** | Enter/delete number, mute alert, quick call |
| **OLED 128x64 SPI Display** | Clear UI: HR, SpO₂, steps, previous values, countdown timer |
| **Firebase Realtime DB** | Syncs sensor data & emergency contact |
| **Web Dashboard** | Live charts for HR, SpO₂, and steps |
| **Manual Alert Mute** | Press **A** to disable alert |

<br>

## Setup Guide

### Hardware Requirements
- ESP32 DevKit V1
- MAX30100 (Pulse Oximeter + Heart Rate)
- MPU6050 (6-DoF IMU)
- SSD1306 128x64 OLED (SPI interface)
- 4x4 Matrix Keypad
- A7682S GSM/GPRS Module
- Buzzer/LED on GPIO2
- Stable 5V power supply

### Pinout Configuration

| Device | ESP32 Pin |
|--------|----------|
| OLED (SPI) | MOSI: 23, SCK: 18, CS: 5, DC: 15, RST: 4 |
| MAX30100 & MPU6050 | SDA: 21, SCL: 22 (I²C) |
| A7682S (UART1) | RX: 16, TX: 17 |
| 4x4 Keypad | Rows: 27,14,12,13 – Cols: 32,33,25,26 |
| Buzzer/Alert | GPIO2 |

### Software Requirements
- **Arduino IDE** (latest version)
- **Libraries** (install via Library Manager):

- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit MPU6050
- MAX30100lib (by oxullo)
- Keypad
- Firebase-ESP-Client (by mobizt)
- WiFi (built-in with ESP32)

### Firebase Setup

- Create a project at Firebase Console
- Enable Realtime Database
- Copy API_KEY and DATABASE_URL

### Web Dashboard Setup

- Open index.html
- Update firebaseConfig with your project credentials
- Deploy using Firebase Hosting, Vercel, or GitHub Pages

### Web Dashboard

- **Home: Welcome screen**
- **Phone Input: Save/delete emergency number**
- **Data Display: 3 real-time line charts (HR, SpO₂, Steps)**
- **Fully Responsive: Works on mobile & desktop**

<div align="center">
  <p><strong>© 2025 – HCMUTE Senior Project Team</strong></p>
  <p><i>Nguyễn Phạm Huy Hoàng - 22161125</i> | <i>Trần Nguyễn Gia Huy - 22161129</i></p>
  <p><em>Monitor health. Alert fast. Keep loved ones safe.</em></p>
</div>