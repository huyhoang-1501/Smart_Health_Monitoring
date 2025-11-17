<div align="center">
  <h1>Smart Health Monitoring System</h1>
  <p>
    <img src="https://img.shields.io/badge/Microcontroller-ESP32-blueviolet?style=for-the-badge&logo=espressif&logoColor=white" alt="ESP32">
    <img src="https://img.shields.io/badge/Sensor-MAX30100-red?style=for-the-badge&logo=heartbeat&logoColor=white" alt="MAX30100">
    <img src="https://img.shields.io/badge/Sensor-MPU6050-orange?style=for-the-badge&logo=accelerometer&logoColor=white" alt="MPU6050">
    <img src="https://img.shields.io/badge/Display-SH110X%20128x64%20OLED-blue?style=for-the-badge&logo=display&logoColor=white" alt="SH110X OLED">
    <img src="https://img.shields.io/badge/Keypad-4x4%20Matrix-green?style=for-the-badge&logo=keyboard&logoColor=white" alt="Keypad">
    <img src="https://img.shields.io/badge/GSM-A7682S-yellow?style=for-the-badge&logo=signal&logoColor=white" alt="A7682S">
    <img src="https://img.shields.io/badge/Cloud-Firebase-critical?style=for-the-badge&logo=firebase&logoColor=white" alt="Firebase">
    <img src="https://img.shields.io/badge/AI-ONNX%20Inference-success?style=for-the-badge&logo=tensorflow&logoColor=white" alt="AI ONNX">
    <img src="https://img.shields.io/badge/Language-C%2B%2B-blue?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++">
  </p>
  ---
  <p>
    <a href="#-overview">Overview</a> •
    <a href="#-key-features">Key Features</a> •
    <a href="#-ai-health-prediction">AI Health Prediction</a> •
    <a href="#-setup-guide">Setup Guide</a> •
    <a href="#-web-dashboard">Web Dashboard</a> •
    <a href="#-development-team">Team</a>
  </p>
  ---
</div>
<br>

## Overview
**Smart Health Monitoring System** is an intelligent IoT-based solution designed specifically for **elderly care**. Powered by the **ESP32**, it continuously monitors vital signs, uses **on-device AI** to predict health status, and automatically triggers emergency calls when critical conditions are detected.

**Real-time monitoring includes:**
- Heart Rate (HR) & Blood Oxygen Saturation (SpO₂) → MAX30100
- Step counting → MPU6050 (6-axis IMU)
- Live data streaming to **Firebase Realtime Database**
- **AI-powered health classification** (8 classes) using ONNX Runtime in browser
- Automatic emergency call via **A7682S GSM module**
- Clear local display on **SH110X 128×64 OLED**
- Emergency contact input via **4×4 matrix keypad**

> **Target Application**: Home-based health monitoring for seniors — early detection of cardiac issues, hypoxia, and sedentary behavior.

<br>

## Key Features

| Feature                        | Description                                                                                         |
|--------------------------------|-----------------------------------------------------------------------------------------------------|
| **Heart Rate & SpO₂ Monitoring** | Updated every 60 seconds, instantly displayed on OLED                                              |
| **Step Counting**              | Moving-average filter + adaptive threshold algorithm                                               |
| **Health Alert System**        | Auto-triggered when:<br>• HR < 50 or > 100 (≥3 consecutive times)<br>• Steps ≤ 80 (≥2 times)       |
| **Emergency Auto-Call**        | Automatically dials saved number after **90 seconds of continuous alert**                          |
| **4×4 Keypad Control**         | Enter/delete emergency number, mute alarm, manual call                                             |
| **SH110X 128×64 OLED Display** | High-contrast UI showing HR, SpO₂, steps, previous values, and countdown timer                     |
| **Firebase Realtime Database** | Synchronizes sensor data and emergency contact in real time                                        |
| **Web Dashboard**              | Live charts + **AI health prediction** tab                                                          |
| **Manual Alert Mute**          | Press key **A** to temporarily silence the alarm                                                   |

<br>

## AI Health Prediction (ONNX Runtime Web)

**The most powerful feature of the project — 100% client-side inference, no backend required!**

| Specification                  | Details                                                                                             |
|--------------------------------|-----------------------------------------------------------------------------------------------------|
| **Model Architecture**         | Fully-connected Neural Network (64→32→16→8), accuracy **> 98.5%** on test set                       |
| **Input**                      | 2 features: Heart Rate (BPM) + SpO₂ (%)                                                             |
| **Output Classes**             | 8 health states:<br>• Normal<br>• Light / Moderate / Intense Activity<br>• Unstable Health<br>• Abnormally High Heart Rate<br>• Excellent Health<br>• **Critical Danger** |
| **Technology**                 | ONNX Runtime Web (`onnxruntime-web`) — loads in 1–3 s, runs smoothly on any device                 |
| **Model Files**                | `hr_spo2_model.onnx` (~50 KB) + `scaler.json`                                                       |
| **Automation**                 | Auto-fetches latest data from Firebase and predicts instantly when entering the AI tab             |
| **UI**                         | Color-coded result with glowing animation (green = good, red = danger)                              |

**Highlight**: Entire AI runs **in the browser** → maximum privacy, zero server cost, works offline.

<br>

## Setup Guide

### Hardware Requirements
- ESP32 DevKit V1
- MAX30100 Pulse Oximeter & Heart Rate Sensor
- MPU6050 6-DoF Accelerometer/Gyroscope
- **SH1106 / SH1107 128×64 OLED** (I²C recommended)
- 4×4 Matrix Keypad
- A7682S GSM/GPRS Module
- Active buzzer / LED on GPIO2
- Stable 5 V power supply

### Pinout Configuration (SH110X I²C – Recommended)

| Device                  | ESP32 Pin           | Note                                      |
|-------------------------|---------------------|-------------------------------------------|
| SH110X OLED (I²C)       | SDA: 21, SCL: 22    | Use `U8g2lib` (best SH1106/SH1107 support)|
| MAX30100 & MPU6050      | SDA: 21, SCL: 22    | Shared I²C bus                            |
| A7682S (UART1)          | RX: 16, TX: 17      |                                           |
| 4×4 Keypad              | Rows: 27,14,12,13 – Cols: 32,33,25,26 |                                |
| Buzzer / Alert          | GPIO2               |                                           |

### Required Arduino Libraries
- `U8g2` by olikraus → **best SH1106/SH1107 support**
- `Adafruit MPU6050`
- `MAX30100lib` (oxullo)
- `Keypad`
- `Firebase-ESP-Client` (mobizt)

### Firebase & Web Setup
1. Create a Firebase project → enable **Realtime Database**
2. Copy `apiKey` & `databaseURL` into Arduino code and `firebaseConfig` in `index.html`
3. Place these two files in the web folder:
   - `hr_spo2_model.onnx`
   - `scaler.json`
4. Deploy via Firebase Hosting, Vercel, Netlify, or GitHub Pages  
   → Or simply open `index.html` locally (AI still works!)

<br>

## Web Dashboard
- Home page with HCMUTE branding
- Emergency contact management
- Three real-time line charts (HR, SpO₂, Steps)
- **AI Health Prediction tab** – automatic inference with stunning UI
- Fully responsive (mobile & desktop friendly)

## Image:
### Overview:
![Image](https://github.com/user-attachments/assets/50429869-d153-47e3-ab9e-ad384d849f65)
### Enter the phone number:
![Image](https://github.com/user-attachments/assets/0a1dabda-bbf2-48a6-9178-da7479389433)
### Display health parameters:
![Image](https://github.com/user-attachments/assets/4361e7e2-2d6a-45c6-8e4f-ba75ed49c63c)
### AI health prediction:
![Image](https://github.com/user-attachments/assets/7be2303c-7350-4f8b-a878-4360972f8d56)
<br>

<div align="center">

**© 2025 – Ho Chi Minh City University of Technology and Education (HCMUTE)**  
**Electronics & Communication Engineering Technology**

**Nguyễn Phạm Huy Hoàng – 22161125**  
**Trần Nguyễn Gia Huy – 22161129**

<em>“Monitor health. Alert fast. Keep loved ones safe.”</em>

</div>