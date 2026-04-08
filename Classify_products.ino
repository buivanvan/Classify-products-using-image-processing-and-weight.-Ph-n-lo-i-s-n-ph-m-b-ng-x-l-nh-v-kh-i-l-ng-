#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Q2HX711.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ===== WiFi =====
const char* ssid = "HONOR500";
const char* password = "123456789";

// ===== MQTT Broker =====
const char* mqtt_server = "ba14e7ca940f45318d2ec89a055d51a6.s1.eu.hivemq.cloud";   // hoặc IP broker của bạn
const int mqtt_port = 8883;
const char* mqtt_user = "vanviet";
const char* mqtt_pass = "Viet1234";
const char* mqtt_topic_pub = "sanluong";
// ===== LED =====
const int ledPin = 2;
// ===== Blink WiFi =====
unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool ledState = LOW;

// ===== SSL Client =====
WiFiClientSecure espSecureClient;
PubSubClient client(espSecureClient);

// ===== Kết nối WiFi =====
void setup_wifi() {
  delay(10);
  // Serial.println();
  // Serial.print("Dang ket noi WiFi: ");
  // Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  // Serial.println();
  // Serial.println("WiFi da ket noi");
  // Serial.print("IP: ");
  // Serial.println(WiFi.localIP());
}

unsigned long lastMqttRetry = 0;
uint8_t state = 0;

void reconnect() {
  if (client.connected()) {
    state = 1;
    return;
  }

  state = 0;

  if (millis() - lastMqttRetry < 5000) return;
  lastMqttRetry = millis();

  String clientId = "ESP32Client-";
  clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    state = 1;
  }
}

// ===== Chớp LED khi WiFi đã kết nối =====
void blinkLedWhenWifiConnected() {
  if (WiFi.status() == WL_CONNECTED && state == 1) {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  } else {
    digitalWrite(ledPin, LOW);
  }
}

// ======= LCD I2C =======
// Địa chỉ thường là 0x27 hoặc 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ======= HX711 ========
const byte hx711_data_pin = 5;
const byte hx711_clock_pin = 4;
Q2HX711 hx711(hx711_data_pin, hx711_clock_pin);

long zero = 167247.5 - 38;
float do_phan_giai = 3.8; // g
int m = 0; // khối lượng vật
unsigned long last_mea = 0;

// ======= Servo =========
Servo servo_pl1;
Servo servo_pl2;
Servo servo_gat;

int servo2Pin = 13;     // phân loại 2
int servo1Pin = 14;     // phân loại 1
int servo_gat_Pin = 27; // gạt

// ====== Hồng ngoại ======
// có vật = 0, không vật = 1
#define IR1 32
#define IR2 33
int sensor1, sensor2;

// ======= Serial =======
char data;

// ====== Phân loại ======
uint8_t loai = 0;

// ====== Đếm sản phẩm ======
// demSP[0] = loại 1
// demSP[1] = loại 2
// demSP[2] = loại 3
unsigned long demSP[3] = {0, 0, 0};

// ====== Hiển thị màu ======
String mauNhanDien = "None";

// ====== Góc servo ======
const int GAT_HOME = 0;
const int GAT_RUN  = 70;

const int PL1_HOME = 90;
const int PL1_RUN  = 30;

const int PL2_HOME = 90;
const int PL2_RUN  = 30;

// ====== Trạng thái non-blocking ======
bool gatDangChay = false;
unsigned long gatStartTime = 0;
const unsigned long gatDuration = 1000; // 1 giây

bool pl1DangChay = false;
unsigned long pl1StartTime = 0;
const unsigned long pl1Duration = 4000; // 4 giây

bool pl2DangChay = false;
unsigned long pl2StartTime = 0;
const unsigned long pl2Duration = 4000; // 4 giây

// chống lặp cảm biến
bool daKichIR1 = false;
bool daKichIR2 = false;

// cờ để biết lần này servo nào cần cộng đếm khi quay về
bool choCongLoai1 = false;
bool choCongLoai2 = false;
bool daDemLoai3 = false;

// ====== LCD update ======
unsigned long lastLCD = 0;
const unsigned long lcdInterval = 200;

// ==================================================
// Hàm hiển thị LCD
// ==================================================
void updateLCD() {
  if (millis() - lastLCD < lcdInterval) return;
  lastLCD = millis();

  // ===== Hàng 1: 1:02 2:05 3:01 =====
  char line1[17];
  snprintf(line1, sizeof(line1), "1:%02lu 2:%02lu 3:%02lu",
           demSP[0], demSP[1], demSP[2]);

  lcd.setCursor(0, 0);
  lcd.print(line1);

  // ===== Hàng 2: M:25g C:GREEN =====
  char line2[17];
  snprintf(line2, sizeof(line2), "m:%dg C:%s", m, mauNhanDien.c_str());

  lcd.setCursor(0, 1);
  lcd.print("                "); // xoa dong cu
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ==================================================
// Hàm bắt đầu xoay servo_gat
// ==================================================
void batDauGat() {
  if (!gatDangChay) {
    servo_gat.write(GAT_RUN);
    gatDangChay = true;
    gatStartTime = millis();
  }
}

// ==================================================
// Hàm bắt đầu xoay servo_pl1
// ==================================================
void batDauPL1() {
  if (!pl1DangChay) {
    servo_pl1.write(PL1_RUN);
    pl1DangChay = true;
    pl1StartTime = millis();
    choCongLoai1 = true;
  }
}

// ==================================================
// Hàm bắt đầu xoay servo_pl2
// ==================================================
void batDauPL2() {
  if (!pl2DangChay) {
    servo_pl2.write(PL2_RUN);
    pl2DangChay = true;
    pl2StartTime = millis();
    choCongLoai2 = true;
  }
}

// ==================================================
// Update servo_gat về vị trí ban đầu sau 1s
// ==================================================
void updateServoGat() {
  if (gatDangChay && millis() - gatStartTime >= gatDuration) {
    servo_gat.write(GAT_HOME);
    gatDangChay = false;
  }
}

// ==================================================
// Update servo_pl1 về vị trí ban đầu sau 2s
// Sau khi về thì tăng đếm loại 1
// ==================================================
void updateServoPL1() {
  if (pl1DangChay && millis() - pl1StartTime >= pl1Duration) {
    servo_pl1.write(PL1_HOME);
    pl1DangChay = false;

    if (choCongLoai1) {
      demSP[0]++;
      publishSanLuong();
      choCongLoai1 = false;
    }
  }
}

// ==================================================
// Update servo_pl2 về vị trí ban đầu sau 2s
// Sau khi về thì tăng đếm loại 2
// ==================================================
void updateServoPL2() {
  if (pl2DangChay && millis() - pl2StartTime >= pl2Duration) {
    servo_pl2.write(PL2_HOME);
    pl2DangChay = false;

    if (choCongLoai2) {
      demSP[1]++;
      publishSanLuong();
      choCongLoai2 = false;
    }
  }
}

void publishSanLuong() {
  char payload[100];
  snprintf(payload, sizeof(payload),
           "%lu:%lu:%lu",
           demSP[0], demSP[1], demSP[2]);

  if (WiFi.status() == WL_CONNECTED && client.connected()) {
    client.publish(mqtt_topic_pub, payload);
  }
}

void setup() {
  Serial.begin(115200);
  //=== led blink====
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // ===== LCD =====
  Wire.begin(21, 22);   // SDA = 21, SCL = 22
  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Phan loai SP");
  lcd.setCursor(0, 1);
  lcd.print("Khoi dong...");
  delay(1000);
  lcd.clear();

  servo_pl1.setPeriodHertz(50);
  servo_pl2.setPeriodHertz(50);
  servo_gat.setPeriodHertz(50);

  servo_pl1.attach(servo1Pin, 500, 2400);
  servo_pl2.attach(servo2Pin, 500, 2400);
  servo_gat.attach(servo_gat_Pin, 500, 2400);

  servo_pl1.write(PL1_HOME);
  servo_pl2.write(PL2_HOME);
  servo_gat.write(GAT_HOME);

  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);

  zero = (hx711.read() / 100.0);

  // ==== wifi and MQTT ===
  setup_wifi();

  // Cách nhanh để test TLS
  // Không kiểm tra chứng chỉ server
  espSecureClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  blinkLedWhenWifiConnected();

  reconnect();
  client.loop();
  // ===== Đọc cảm biến =====
  sensor1 = digitalRead(IR1);
  sensor2 = digitalRead(IR2);

  // ===== Đo khối lượng mỗi 1 giây =====
  if (millis() - last_mea > 1000) {
    m = ((zero - (hx711.read() / 100.0)) / do_phan_giai);
    if(m < 0) m = 0;
    if (m <= 15) mauNhanDien = "None";

    if (m > 15) {
      Serial.println("begin");
    }

    last_mea = millis();
  }

  // ===== Nhận dữ liệu serial =====
  if (Serial.available() > 0) {
    data = Serial.read();

    if (data == 'g') {
      loai = 1;
      mauNhanDien = "Green";
      daDemLoai3 = false;
    }
    else if (data == 'r' && m <= 60) {
      loai = 2;
      mauNhanDien = "Red";
      daDemLoai3 = false;
    }
    else if (data == 'r' && m > 60) {
      loai = 3;
      mauNhanDien = "Red";
      daDemLoai3 = false;
    }

    // Khi có vật thì mới gạt
    if (m > 15) {
      batDauGat();
    }
  }

  // ===== Loại 1 =====
  if (loai == 1) {
    if (sensor1 == 0 && !daKichIR1) {
      batDauPL1();
      daKichIR1 = true;
      loai = 0;
    }

    if (sensor1 == 1) {
      daKichIR1 = false;
    }
  }

  // ===== Loại 2 =====
  if (loai == 2) {
    if (sensor2 == 0 && !daKichIR2) {
      batDauPL2();
      daKichIR2 = true;
      loai = 0;
    }

    if (sensor2 == 1) {
      daKichIR2 = false;
    }
  }

  // ===== Loại 3: đi thẳng =====
  if (loai == 3) {
    if (sensor2 == 0 && !daDemLoai3) {
      demSP[2]++;
      publishSanLuong();
      daDemLoai3 = true;
      loai = 0;

      // Serial.print("Loai 3: ");
      // Serial.println(demSP[2]);
    }
  }

  // reset cờ đếm loại 3 khi vật đi qua xong
  if (sensor2 == 1) {
    daDemLoai3 = false;
  }

  // ===== Update non-blocking =====
  updateServoGat();
  updateServoPL1();
  updateServoPL2();
  updateLCD();
}