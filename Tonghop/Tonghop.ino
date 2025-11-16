#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Keypad.h>
#include <HardwareSerial.h>

// ===== Thông tin WiFi và Firebase =====
#define WIFI_SSID "Huy Hoang"
#define WIFI_PASSWORD "cudiroiseden"
#define API_KEY "AIzaSyD3_MWJ-A5wkar9UdDEjo0EuTTmmjxs-vo"
#define DATABASE_URL "https://project-2-health-default-rtdb.asia-southeast1.firebasedatabase.app"

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signUp = false;

// ===== OLED SPI =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_CS   5
#define OLED_DC   15
#define OLED_RST  4

// I2C Pins cho MAX30100 và MPU6050
#define I2C_SDA 21
#define I2C_SCL 22

// UART1 cho A7682S
#define A7682S_RX 16
#define A7682S_TX 17

//Chân GPIO cảnh báo
#define PIN_OUT 2
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
//Khởi tạo đối tượng 
// OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
// MAX30100
PulseOximeter pox;
// MPU6050
Adafruit_MPU6050 mpu;
// Keypad matrix 4x4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
// SIM A7682S
// ===== UART1 cho A7682S =====
HardwareSerial A7682S(1); // UART1

// Biến thời gian đo
#define MEASURE_TIME 60000   // 60s
uint32_t startTime = 0;
bool measuring = false;
float bpm = 0, spo2 = 0;
float pre_bpm = 0, pre_spo2 = 0; // lưu giá trị cũ
int pre_steps = 0;

//  Biến để đếm bước chân
const float threshold = 1.0;   // ngưỡng cho người lớn tuổi
const int bufferLength = 15;
float buffer[bufferLength];
int bufferIndex = 0;
int stepCount = 0;
bool stepDetected = false;
const unsigned long debounceDelay = 300; // ms
unsigned long lastStepTime = 0;

// Biến điều khiển 
bool inInputMode = false;   // Trạng thái màn hình nhập số
String phoneNumber = "";    // Số đang nhập
String savedNumbers = "";   // Danh sách các số đã lưu

// Biến kiểm tra nhịp tim và bước chân
int bpm_warning = 0, step_warning = 0;
// Biến cảnh báo
bool warning_enable = false;
unsigned long warningStartTime = 0; // Thời điểm bắt đầu cảnh báo
const unsigned long WARNING_DURATION = 90000; // 1 phút 30 giây


// ===== Gửi lệnh AT =====
void sendAT(String cmd) {
  A7682S.println(cmd);
  Serial.println(">> " + cmd);
}
// ===== Callback cập nhật mỗi khi có nhịp tim =====
void onBeatDetected() {
  Serial.println("Nhịp tim!");
}

// ====== Hiển thị giao diện ban đầu ======
void giao_dien_hien_thi() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Health Monitor");

  display.setCursor(0, 10);
  display.print("Time:");

  // HR
  display.setCursor(0, 20);
  display.print("HR:");
  display.setCursor(24, 20);
  display.print("--- bpm");
  display.setCursor(80, 20);
  display.print("pre:");
  display.setCursor(104, 20);
  display.print("---");

  // SpO2
  display.setCursor(0, 30);
  display.print("SpO2:");
  display.setCursor(36, 30);
  display.print("--- %");
  display.setCursor(80, 30);
  display.print("pre:");
  display.setCursor(104, 30);
  display.print("---");

  // Steps
  display.setCursor(0, 40);
  display.print("Steps:");
  display.setCursor(42, 40);
  display.print("---");
  display.setCursor(80, 40);
  display.print("cnt:");
  display.setCursor(104, 40);
  display.print("---");
  
  //Gợi ý nhấn nút "*"
  display.setCursor(0, 54);
  display.println("Nhan '*' de nhap SDT");
  display.display();
}
// ===== Hiển thị màn hình nhập =====
void giao_dien_nhap_sdt() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Nhap so:");
  display.setTextSize(2);
  display.setCursor(0,14);
  display.println(phoneNumber);
  display.setTextSize(1);
  display.setCursor(0,44);
  display.println("*:Thoat  D:Luu");
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.println("Luu:");
  display.setCursor(30, 54);
  display.print(savedNumbers);
  display.display();
}

// Hiển thị lưu thành công 
void man_hinh_luu() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 20);
  display.println("Da luu!");
  display.display();
}

void guiLenFirebase()
{
  if (Firebase.ready() && signUp)
  {
    float bpmRounded = round(bpm);
    Firebase.RTDB.setFloat(&fbdo, "/parameter/heartbeat", bpmRounded);
    Firebase.RTDB.setFloat(&fbdo, "/parameter/spo2", spo2);
    Firebase.RTDB.setInt(&fbdo, "/parameter/steps", stepCount);

    Serial.printf("Gửi Firebase: HR=%.1f bpm | SpO2=%.1f%% | Steps=%d\n", bpm, spo2, stepCount);
  }
}

void docTuFirebase() {
  if (Firebase.ready() && signUp) {
    if (Firebase.RTDB.getString(&fbdo, "/user/phone/sdt")) {
      if (fbdo.dataType() == "string") {
        String sdt = fbdo.stringData();
        Serial.print(" Số điện thoại đọc được từ Firebase: ");
        Serial.println(sdt);
        if (sdt.length() == 10)
        savedNumbers = sdt;
      }
    } else {
      Serial.print(" Lỗi khi đọc dữ liệu: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

void xu_li_man_hinh_nhap()
{
  while (true) {
    char key2 = keypad.getKey();
    if (!key2) continue;

    if (key2 >= '0' && key2 <= '9') {
    if (phoneNumber.length() < 11) {
        phoneNumber += key2;
        giao_dien_nhap_sdt();
      }
    }
    // 🔹 Nút B: Xóa 1 ký tự cuối
    else if (key2 == 'B' && phoneNumber.length() > 0) {
      phoneNumber.remove(phoneNumber.length() - 1);
      giao_dien_nhap_sdt();
    }
    // 🔹 Nút D: Lưu số
    else if (key2 == 'D' && phoneNumber.length() > 0) {
      savedNumbers = phoneNumber;
      Serial.println("Đã lưu số mới: " + savedNumbers);
      man_hinh_luu();
      delay(1500);
      inInputMode = false;
      giao_dien_hien_thi();
      break;
    }
    // 🔹 Nút *: Thoát nhập
    else if (key2 == '*') {
      inInputMode = false;
      giao_dien_hien_thi();
      break;
    }
  }
}

void xu_li_keypad() {
  char key = keypad.getKey();
  if (!key) return;

  // ===== Nút A: Tắt cảnh báo sức khỏe =====
  if (key == 'A' && warning_enable) {
    warning_enable = false;
    digitalWrite(LED_BUILTIN, LOW);
    bpm_warning = 0;
    step_warning = 0;
    Serial.println("Cảnh báo đã được tắt thủ công bằng nút A!");
    return;
  }

  // ===== Nút C (gọi) — hoạt động ở mọi chế độ =====
  if (key == 'C' && savedNumbers.length() > 0) {
    sendAT("AT+CHUP");       // Dừng cuộc gọi cũ nếu có
    delay(1000);
    sendAT("AT+CREG?");
    delay(1000);
    sendAT("ATD" + savedNumbers + ";");
    return;
  }

  // ===== Màn hình chính =====
  if (!inInputMode) {
    if (key == '*') {
      inInputMode = true;
      phoneNumber = "";
      giao_dien_nhap_sdt();
      xu_li_man_hinh_nhap();
      }
    }
  }

// Hàm reset các biến trong quá trình đo
void reset_cac_bien_do(){
      pox.begin(); // Re-init lại cảm biến 
      pox.setOnBeatDetectedCallback(onBeatDetected);
      startTime = millis();
      measuring = true;
      stepCount = 0;
}
void dem_buoc_chan()
{
  // -------- MPU6050 Step Counter --------
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accX = a.acceleration.x;
  float accY = a.acceleration.y;
  float accZ = a.acceleration.z;
  float magnitude = sqrt(accX * accX + accY * accY + accZ * accZ);

  buffer[bufferIndex] = magnitude;
  bufferIndex = (bufferIndex + 1) % bufferLength;

  float avgMagnitude = 0;
  for (int i = 0; i < bufferLength; i++) {
    avgMagnitude += buffer[i];
  }
  avgMagnitude /= bufferLength;

  unsigned long currentMillis = millis();
  if (magnitude > (avgMagnitude + threshold)) {
    if (!stepDetected && (currentMillis - lastStepTime) > debounceDelay) {
      stepCount++;
      stepDetected = true;
      lastStepTime = currentMillis;

      Serial.print("Step detected! Count = ");
      Serial.println(stepCount);
      display.fillRect(104, 40, 24, 8, SSD1306_BLACK);
      display.setCursor(104, 40);
      display.print(stepCount);
      display.display();
    }
  } else {
    stepDetected = false;
  }
}

// ====== Hàm đo nhịp tim & SpO2 ======
void xu_li_va_hien_thi_thong_so() {
  if (measuring) {
    unsigned long elapsed = millis() - startTime;
    int remaining = (MEASURE_TIME - elapsed) / 1000;
    if (remaining < 0) remaining = 0;

    // Hiển thị đếm ngược thời gian
    display.fillRect(36, 10, 50, 10, SSD1306_BLACK);
    display.setCursor(36, 10);
    display.print(remaining);
    display.print("s");
    display.display();

    // Khi hết thời gian đo
    if (elapsed >= MEASURE_TIME) {
      bpm = pox.getHeartRate();
      spo2 = pox.getSpO2();
      // Xóa vùng hiển thị cũ
      display.fillRect(24, 20, 52, 8, SSD1306_BLACK);
      display.fillRect(36, 30, 44, 8, SSD1306_BLACK);

      // Hiển thị HR mới
      display.setCursor(24, 20);
      if (bpm > 30 && bpm < 200) display.printf("%.0f bpm", bpm);
      else display.print("--- bpm");

      // Hiển thị SpO2 mới
      display.setCursor(36, 30);
      if (spo2 > 50 && spo2 <= 100) display.printf("%.0f %%", spo2);
      else display.print("--- %");

      // ======= HIỂN THỊ BƯỚC CHÂN =======
      // Cập nhật Pre Steps
      pre_steps = stepCount;
      display.fillRect(42, 40, 36, 8, SSD1306_BLACK);
      display.setCursor(42, 40);
      display.print(pre_steps);
      display.display();

      // ======= CẬP NHẬT HR, SpO2 CŨ (Pre) =======
      display.fillRect(104, 20, 24, 8, SSD1306_BLACK);
      display.setCursor(104, 20);
      if (pre_bpm > 30 && pre_bpm < 200) display.printf("%.0f", pre_bpm);
      else display.print("---");

      display.fillRect(104, 30, 24, 8, SSD1306_BLACK);
      display.setCursor(104, 30);
      if (pre_spo2 > 50 && pre_spo2 <= 100) display.printf("%.0f", pre_spo2);
      else display.print("---");

      display.display();

      // Ghi nhận giá trị mới cho lần sau
      pre_bpm = bpm;
      pre_spo2 = spo2;
      measuring = false;
      Serial.printf("Ket qua: HR=%.0f bpm | SpO2=%.0f%% | Steps=%d\n", bpm, spo2, stepCount);
      if (bpm < 50 || bpm > 100) bpm_warning ++;
      if (pre_steps <= 80) step_warning ++;
      guiLenFirebase();
      delay(1000);
    }
  }
}


//Hàm cảnh bảo
void canh_bao_suc_khoe()
{
 if (bpm_warning >= 3 || step_warning >= 2)
  {
    if (!warning_enable)
    {
      warning_enable = true;
      warningStartTime = millis(); // Lưu thời điểm bắt đầu
      //digitalWrite(PIN_OUT, HIGH);
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Cảnh báo sức khỏe kích hoạt!");
    }
    if (warning_enable)
    {
     unsigned long elapsed = millis() - warningStartTime;

     // Nếu quá 2 phút 30s mà chưa tắt => tự động gọi
     if (elapsed >= WARNING_DURATION)
     {
      Serial.println("Quá 150s, thực hiện cuộc gọi khẩn cấp...");
      sendAT("AT+CHUP");       // Dừng cuộc gọi cũ nếu có
      delay(1000);
      sendAT("AT+CREG?");
      delay(1000);
      sendAT("ATD" + savedNumbers + ";");
      warning_enable = false;  // Reset cảnh báo sau khi gọi
      //digitalWrite(PIN_OUT, LOW);
      digitalWrite(LED_BUILTIN, LOW);
      bpm_warning = 0;
      step_warning = 0;
     }
    }
  }
}
// ===== Khởi tạo =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  // A7682S
  A7682S.begin(115200, SERIAL_8N1, A7682S_RX, A7682S_TX);
  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Lỗi OLED!"));
    while (1);
  }
  
  // MPU6050
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 OK.");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  // ===== WiFi =====
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // ===== Firebase =====
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Đăng ký Firebase thành công!");
    signUp = true;
  } else {
    Serial.printf("Lỗi Firebase: %s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  // MAX30100
  if (!pox.begin()) {
    Serial.println("Loi khoi dong MAX30100");
    while (1);
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
  giao_dien_hien_thi();
  // Bắt đầu đo
  measuring = true;
  startTime = millis();
}

// ===== Vòng lặp =====
void loop() {
  pox.update();
  dem_buoc_chan();
  xu_li_va_hien_thi_thong_so();
  canh_bao_suc_khoe();
  xu_li_keypad();
  if(!measuring) reset_cac_bien_do();
  while (A7682S.available()) Serial.write(A7682S.read());
}