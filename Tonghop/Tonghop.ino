#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <MAX30100_PulseOximeter.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <Keypad.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ================= Configuration ===================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED SPI Pins
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     15
#define OLED_CS     5
#define OLED_RESET  4

// I2C Pins for MPU6050 and MAX30100
#define I2C_SDA 21
#define I2C_SCL 22

// UART1 for A7682S
#define A7682S_RX 16
#define A7682S_TX 17

// Step Counter Parameters
#define STEP_THRESHOLD 1.0
#define BUFFER_LENGTH 15
#define DEBOUNCE_DELAY 300

// MAX30100 Parameters
#define REPORTING_PERIOD_MS 1000
#define EMA_ALPHA 0.2

// Health Warning
#define HR_LOW 50
#define HR_HIGH 100
#define STEP_LOW_LIMIT 80
#define WARNING_DURATION 90000  // 90s
#define HR_WARNING_COUNT 3
#define STEP_WARNING_COUNT 2

// WiFi & Firebase
#define WIFI_SSID "Phong Tro Tang 3.2"
#define WIFI_PASSWORD "99999999"
#define API_KEY "AIzaSyD3_MWJ-A5wkar9UdDEjo0EuTTmmjxs-vo"
#define DATABASE_URL "https://project-2-health-default-rtdb.asia-southeast1.firebasedatabase.app"

// Keypad 4x4
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[4] = {27,14,12,13};
byte colPins[4] = {32,33,25,26};

// ================= Global Objects ===================
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_MPU6050 mpu;
PulseOximeter pox;
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
HardwareSerial A7682S(1);
SemaphoreHandle_t i2cMutex;
QueueHandle_t sensorQueue;

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseReady = false;

// ================= Global Variables ===================
float buffer[BUFFER_LENGTH];
int bufferIndex = 0;
int stepCount = 0;
int pre_steps = 0;
bool stepDetected = false;
unsigned long lastStepTime = 0;
float filteredBpm = 0.0;
float filteredSpo2 = 0.0;
float pre_bpm = 0.0, pre_spo2 = 0.0;
String phoneNumber = "";
String savedNumbers = "";
bool displayMode = true; // true: Sensor, false: Input

// Warning System
int bpm_warning = 0, step_warning = 0;
bool warning_enable = false;
unsigned long warningStartTime = 0;

// Data Structure
struct SensorData {
  float bpm;
  float spo2;
  int steps;
  int pre_steps;
  float pre_bpm;
  float pre_spo2;
};

// ================= Helper Functions ===================
void onBeatDetected() {
  Serial.println("Beat!");
}

float ema(float current, float prev, float alpha) {
  return alpha * current + (1 - alpha) * prev;
}

void sendAT(String cmd) {
  A7682S.println(cmd);
  Serial.println(">> " + cmd);
}

void displayLoadingScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Khoi tao he thong...");
  display.display();
  delay(2000);
}

void updateSensorDisplay(const SensorData &data) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("Smart Health Monitor");

  // HR
  display.setCursor(0, 12);
  display.print("HR: ");
  if (data.bpm > 30 && data.bpm < 200) display.printf("%.0f bpm", data.bpm);
  else display.print("--- bpm");

  display.setCursor(80, 12);
  display.print("pre:");
  if (data.pre_bpm > 30) display.printf("%.0f", data.pre_bpm);
  else display.print("---");

  // SpO2
  display.setCursor(0, 22);
  display.print("SpO2: ");
  if (data.spo2 > 50 && data.spo2 <= 100) display.printf("%.0f %%", data.spo2);
  else display.print("--- %");

  display.setCursor(80, 22);
  display.print("pre:");
  if (data.pre_spo2 > 50) display.printf("%.0f", data.pre_spo2);
  else display.print("---");

  // Steps
  display.setCursor(0, 32);
  display.print("Steps: ");
  display.print(data.steps);

  display.setCursor(80, 32);
  display.print("cnt:");
  display.print(data.pre_steps);

  // Warning
  if (warning_enable) {
    display.setCursor(0, 45);
    display.print("CANH BAO! Nhan A de tat");
  } else {
    display.setCursor(0, 45);
    display.println("Nhan D de nhap SDT");
  }

  // Phone
  display.setCursor(0, 55);
  display.print("SDT: ");
  if (savedNumbers.length() > 0) {
    display.print(savedNumbers);
  } else {
    display.print("Chua co");
  }

  display.display();
}

void updatePhoneDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Nhap so dien thoai:");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(phoneNumber);
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.println("A: Goi  B: Xoa  C: Ngat  D: Thoat");
  display.display();
}

void guiLenFirebase(const SensorData &data) {
  if (!firebaseReady) return;

  float bpmRounded = round(data.bpm);
  Firebase.RTDB.setFloat(&fbdo, "/parameter/heartbeat", bpmRounded);
  Firebase.RTDB.setFloat(&fbdo, "/parameter/spo2", data.spo2);
  Firebase.RTDB.setInt(&fbdo, "/parameter/steps", data.steps);
  Serial.printf("Firebase: HR=%.1f | SpO2=%.1f%% | Steps=%d\n", data.bpm, data.spo2, data.steps);
}

void docTuFirebase() {
  if (!firebaseReady) return;

  if (Firebase.RTDB.getString(&fbdo, "/user/phone/sdt")) {
    if (fbdo.dataType() == "string") {
      String sdt = fbdo.stringData();
      if (sdt.length() == 10) {
        savedNumbers = sdt;
        Serial.println("Doc SDT tu Firebase: " + savedNumbers);
      }
    }
  }
}

// ================= FreeRTOS Tasks ===================
void readSensorTask(void *pvParameters) {
  SensorData data;
  uint32_t lastReport = 0;
  bool first = true;

  while (true) {
    data.bpm = filteredBpm;
    data.spo2 = filteredSpo2;
    data.steps = stepCount;
    data.pre_steps = pre_steps;
    data.pre_bpm = pre_bpm;
    data.pre_spo2 = pre_spo2;

    // Read MPU6050
    sensors_event_t a, g, temp;
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
      mpu.getEvent(&a, &g, &temp);
      xSemaphoreGive(i2cMutex);
    }

    float magnitude = sqrt(a.acceleration.x*a.acceleration.x +
                           a.acceleration.y*a.acceleration.y +
                           a.acceleration.z*a.acceleration.z);
    buffer[bufferIndex] = magnitude;
    bufferIndex = (bufferIndex + 1) % BUFFER_LENGTH;
    float avg = 0;
    for (int i = 0; i < BUFFER_LENGTH; i++) avg += buffer[i];
    avg /= BUFFER_LENGTH;

    unsigned long now = millis();
    if (magnitude > avg + STEP_THRESHOLD && !stepDetected && (now - lastStepTime) > DEBOUNCE_DELAY) {
      stepCount++;
      stepDetected = true;
      lastStepTime = now;
    } else if (magnitude <= avg + STEP_THRESHOLD) {
      stepDetected = false;
    }

    // Read MAX30100
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
      pox.update();
      xSemaphoreGive(i2cMutex);
    }

    float rawBpm = pox.getHeartRate();
    float rawSpo2 = pox.getSpO2();
    filteredBpm = ema(rawBpm, filteredBpm, EMA_ALPHA);
    filteredSpo2 = ema(rawSpo2, filteredSpo2, EMA_ALPHA);

    // Update every 60s
    if (now - lastReport >= 60000 || first) {
      first = false;
      lastReport = now;

      pre_bpm = filteredBpm;
      pre_spo2 = filteredSpo2;
      pre_steps = stepCount;

      data.pre_bpm = pre_bpm;
      data.pre_spo2 = pre_spo2;
      data.pre_steps = pre_steps;

      // Warning logic
      if (filteredBpm < HR_LOW || filteredBpm > HR_HIGH) bpm_warning++;
      else bpm_warning = 0;

      if (stepCount <= STEP_LOW_LIMIT) step_warning++;
      else step_warning = 0;

      if (bpm_warning >= HR_WARNING_COUNT || step_warning >= STEP_WARNING_COUNT) {
        if (!warning_enable) {
          warning_enable = true;
          warningStartTime = millis();
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.println("CANH BAO KICH HOAT!");
        }
      }

      guiLenFirebase(data);
      stepCount = 0; // Reset sau mỗi phút
    }

    data.bpm = filteredBpm;
    data.spo2 = filteredSpo2;
    data.steps = stepCount;

    xQueueSend(sensorQueue, &data, portMAX_DELAY);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void displayTask(void *pvParameters) {
  SensorData data;

  while (true) {
    if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {
      if (displayMode) {
        updateSensorDisplay(data);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void warningTask(void *pvParameters) {
  while (true) {
    if (warning_enable) {
      unsigned long elapsed = millis() - warningStartTime;
      if (elapsed >= WARNING_DURATION) {
        Serial.println("90s qua, goi khan cap...");
        sendAT("AT+CHUP");
        delay(1000);
        sendAT("ATD" + savedNumbers + ";");
        warning_enable = false;
        digitalWrite(LED_BUILTIN, LOW);
        bpm_warning = 0;
        step_warning = 0;
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void keypadTask(void *pvParameters) {
  while (true) {
    char key = keypad.getKey();
    if (key) {
      // Tắt cảnh báo bằng A
      if (key == 'A' && warning_enable) {
        warning_enable = false;
        digitalWrite(LED_BUILTIN, LOW);
        bpm_warning = 0;
        step_warning = 0;
        Serial.println("Da tat canh bao!");
        continue;
      }

      // Gọi nhanh bằng C
      if (key == 'C' && savedNumbers.length() > 0) {
        sendAT("AT+CHUP");
        delay(1000);
        sendAT("ATD" + savedNumbers + ";");
        continue;
      }

      // Chuyển chế độ nhập số bằng D
      if (key == 'D') {
        displayMode = !displayMode;
        phoneNumber = "";
        if (!displayMode) updatePhoneDisplay();
        continue;
      }

      // Chỉ xử lý nhập số khi ở chế độ nhập
      if (!displayMode) {
        if (key >= '0' && key <= '9' && phoneNumber.length() < 11) {
          phoneNumber += key;
          updatePhoneDisplay();
        } else if (key == 'B' && phoneNumber.length() > 0) {
          phoneNumber.remove(phoneNumber.length() - 1);
          updatePhoneDisplay();
        } else if (key == 'A' && phoneNumber.length() > 0) {
          sendAT("ATD" + phoneNumber + ";");
        } else if (key == 'C') {
          sendAT("ATH");
        }
      }
    }

    while (A7682S.available()) {
      Serial.write(A7682S.read());
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ================= Setup ==================
void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  A7682S.begin(115200, SERIAL_8N1, A7682S_RX, A7682S_TX);

  i2cMutex = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(10, sizeof(SensorData));

  // OLED
  if (!display.begin(0, true)) {
    Serial.println("OLED Loi!");
    while (1);
  }
  displayLoadingScreen();

  // MPU6050
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
    if (!mpu.begin()) {
      Serial.println("MPU6050 Loi!");
      xSemaphoreGive(i2cMutex);
      while (1);
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
    xSemaphoreGive(i2cMutex);
  }

  // MAX30100
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
    if (!pox.begin()) {
      Serial.println("MAX30100 Loi!");
      xSemaphoreGive(i2cMutex);
      while (1);
    }
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    xSemaphoreGive(i2cMutex);
  }

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK!");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;
  if (Firebase.signUp(&config, &auth, "", "")) {
    firebaseReady = true;
    Serial.println("Firebase OK!");
  } else {
    Serial.println("Firebase loi: " + config.signer.signupError.message);
  }
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  docTuFirebase();

  // Tasks
  xTaskCreatePinnedToCore(readSensorTask, "Sensor", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(displayTask, "Display", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(keypadTask, "Keypad", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(warningTask, "Warning", 2048, NULL, 1, NULL, 0);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}