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
#define STEP_THRESHOLD 1.0       // Acceleration threshold for step detection
#define BUFFER_LENGTH 15         // Buffer size for averaging
#define DEBOUNCE_DELAY 300       // ms debounce for step detection

// MAX30100 Parameters
#define REPORTING_PERIOD_MS 1000 // Update interval for heart rate and SpO2
#define EMA_ALPHA 0.2            // EMA coefficient for filtering

// Keypad 4x4 Configuration
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
HardwareSerial A7682S(1); // UART1
SemaphoreHandle_t i2cMutex;
QueueHandle_t sensorQueue;

// ================= Global Variables ===================
float buffer[BUFFER_LENGTH];
int bufferIndex = 0;
int stepCount = 0;
bool stepDetected = false;
unsigned long lastStepTime = 0;
float filteredBpm = 0.0;
float filteredSpo2 = 0.0;
String phoneNumber = "";
bool displayMode = true; // true: Sensor Data, false: Phone Number Input

// ================= Data Structure ===================
struct SensorData {
  float bpm;
  float spo2;
  int stepCount;
};

// ================= Helper Functions ===================
void onBeatDetected() {
  Serial.println("♥ Beat!");
}

float exponentialMovingAverage(float currentValue, float previousFilteredValue, float alpha) {
  return (alpha * currentValue) + ((1 - alpha) * previousFilteredValue);
}

void displayLoadingScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Khoi tao...");
  display.display();
  delay(2000);
}

void updateSensorDisplay(const SensorData &data) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  // Title
  display.setCursor(0, 0);
  display.println("Smart Health Monitor");

  // Steps
  display.setCursor(0, 15);
  display.print("Steps: ");
  display.print(data.stepCount);

  // BPM
  display.setCursor(0, 30);
  display.print("BPM: ");
  display.print(data.bpm, 0);

  // SpO2
  display.setCursor(0, 45);
  display.print("SpO2: ");
  display.print(data.spo2, 0);
  display.print(" %");

  display.display();
}

void updatePhoneDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Nhap so:");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println(phoneNumber);
  display.display();
}

bool initializeOLED() {
  if (!display.begin(0, true)) {
    Serial.println(F("SH1106 allocation failed"));
    return false;
  }
  display.setContrast(255);
  return true;
}

bool initializeMPU6050() {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
    if (!mpu.begin()) {
      Serial.println("Không tìm thấy MPU6050!");
      xSemaphoreGive(i2cMutex);
      return false;
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
    xSemaphoreGive(i2cMutex);
    Serial.println("MPU6050 OK.");
    return true;
  }
  return false;
}

bool initializeMAX30100() {
  if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
    if (!pox.begin()) {
      Serial.println("Không tìm thấy MAX30100!");
      xSemaphoreGive(i2cMutex);
      return false;
    }
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    xSemaphoreGive(i2cMutex);
    Serial.println("MAX30100 OK.");
    return true;
  }
  return false;
}

float calculateMagnitude(sensors_event_t &accel) {
  float x = accel.acceleration.x;
  float y = accel.acceleration.y;
  float z = accel.acceleration.z;
  return sqrt(x * x + y * y + z * z);
}

float getAverageMagnitude() {
  float sum = 0;
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    sum += buffer[i];
  }
  return sum / BUFFER_LENGTH;
}

void sendAT(String cmd) {
  A7682S.println(cmd);
  Serial.println(">> " + cmd);
}

// ================= FreeRTOS Tasks ===================
void readSensorTask(void *pvParameters) {
  while (true) {
    SensorData data = {filteredBpm, filteredSpo2, stepCount};

    // Read MPU6050
    sensors_event_t a, g, temp;
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
      if (!mpu.getEvent(&a, &g, &temp)) {
        Serial.println("Lỗi đọc MPU6050!");
        xSemaphoreGive(i2cMutex);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        continue;
      }
      xSemaphoreGive(i2cMutex);
    }

    float magnitude = calculateMagnitude(a);
    buffer[bufferIndex] = magnitude;
    bufferIndex = (bufferIndex + 1) % BUFFER_LENGTH;

    float avgMagnitude = getAverageMagnitude();
    unsigned long currentMillis = millis();

    if (magnitude > (avgMagnitude + STEP_THRESHOLD)) {
      if (!stepDetected && (currentMillis - lastStepTime) > DEBOUNCE_DELAY) {
        stepCount++;
        stepDetected = true;
        lastStepTime = currentMillis;
        Serial.print("Step detected! Count = ");
        Serial.println(stepCount);
        data.stepCount = stepCount;
      }
    } else {
      stepDetected = false;
    }

    // Read MAX30100
    if (xSemaphoreTake(i2cMutex, portMAX_DELAY)) {
      pox.update();
      xSemaphoreGive(i2cMutex);
    }

    float bpm = pox.getHeartRate();
    float spo2 = pox.getSpO2();
    filteredBpm = exponentialMovingAverage(bpm, filteredBpm, EMA_ALPHA);
    filteredSpo2 = exponentialMovingAverage(spo2, filteredSpo2, EMA_ALPHA);
    data.bpm = filteredBpm;
    data.spo2 = filteredSpo2;

    xQueueSend(sensorQueue, &data, portMAX_DELAY);
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void displaySensorDataTask(void *pvParameters) {
  SensorData data;
  uint32_t lastDisplay = 0;

  while (true) {
    if (xQueueReceive(sensorQueue, &data, portMAX_DELAY)) {
      if (millis() - lastDisplay >= REPORTING_PERIOD_MS) {
        // Print to Serial
        Serial.print("BPM: "); Serial.print(data.bpm, 2);
        Serial.print(" | SpO2: "); Serial.print(data.spo2, 2);
        Serial.print(" | Steps: "); Serial.println(data.stepCount);

        // Update OLED based on display mode
        if (displayMode) {
          updateSensorDisplay(data);
        }

        lastDisplay = millis();
      }
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void keypadTask(void *pvParameters) {
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == 'D') {
        displayMode = !displayMode; // Toggle display mode
        if (displayMode) {
          // Sensor data mode, display will be handled by displaySensorDataTask
        } else {
          updatePhoneDisplay(); // Immediately show phone number input screen
        }
      } else if (!displayMode) {
        // Handle keypad input only in phone number mode
        if (key >= '0' && key <= '9') {
          phoneNumber += key;
          updatePhoneDisplay();
        } else if (key == 'B' && phoneNumber.length() > 0) {
          phoneNumber.remove(phoneNumber.length() - 1);
          updatePhoneDisplay();
        } else if (key == 'A' && phoneNumber.length() > 0) {
          sendAT("ATD" + phoneNumber + ";");
          updatePhoneDisplay();
        } else if (key == 'C') {
          sendAT("ATH");
          updatePhoneDisplay();
        }
      }
    }

    // Handle A7682S responses
    while (A7682S.available()) {
      Serial.write(A7682S.read());
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ================== Setup ==================
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // Set I2C clock to 400kHz
  A7682S.begin(115200, SERIAL_8N1, A7682S_RX, A7682S_TX);

  // Initialize mutex and queue
  i2cMutex = xSemaphoreCreateMutex();
  sensorQueue = xQueueCreate(10, sizeof(SensorData));

  // Initialize OLED
  if (!initializeOLED()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("OLED Loi!");
    display.display();
    while (1);
  }
  displayLoadingScreen();

  // Initialize MPU6050
  if (!initializeMPU6050()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MPU6050 Loi!");
    display.display();
    while (1);
  }

  // Initialize MAX30100
  if (!initializeMAX30100()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MAX30100 Loi!");
    display.display();
    while (1);
  }

  // Initialize buffer
  for (int i = 0; i < BUFFER_LENGTH; i++) {
    buffer[i] = 0;
  }

  // Create FreeRTOS tasks
  xTaskCreatePinnedToCore(readSensorTask, "ReadSensor", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(displaySensorDataTask, "DisplayData", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(keypadTask, "KeypadTask", 4096, NULL, 1, NULL, 0);
}

// ================== Loop ==================
void loop() {
  vTaskDelay(100 / portTICK_PERIOD_MS); // Keep loop simple
}