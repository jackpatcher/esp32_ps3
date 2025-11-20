// ESP32 Robot Control with PS3 Controller - Arduino Version
// esp32 2.0.17 espressif
// Fixed servo freezing issue

#include <Ps3Controller.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// --- Motor Pin Definitions ---
#define M1A_PIN 5
#define M1B_PIN 17
#define M2A_PIN 19
#define M2B_PIN 18
#define M3A_PIN 16
#define M3B_PIN 4
#define M4A_PIN 2
#define M4B_PIN 15

// --- Servo Pin Definitions ---
#define SERVO_S1_PIN 13
#define SERVO_S2_PIN 26

// --- OLED Pin Definitions ---
#define OLED_SCL 22
#define OLED_SDA 21
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- PWM Settings ---
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8  // 8-bit resolution (0-255) ตามไฟล์ old

// Motor PWM Channels (ตามไฟล์ old: 2-9 สำหรับ motor)
#define M1A_CHANNEL 6
#define M1B_CHANNEL 7
#define M2A_CHANNEL 4
#define M2B_CHANNEL 5
#define M3A_CHANNEL 3
#define M3B_CHANNEL 2
#define M4A_CHANNEL 8
#define M4B_CHANNEL 9

// Servo objects (ใช้ ESP32Servo library แยกจาก PWM)
Servo servo_s1;
Servo servo_s2;

// OLED Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Global Variables
float posS1 = 90.0;
float posS2 = 90.0;
float lastPosS1 = 90.0;  // เก็บค่าครั้งล่าสุดเพื่อเปรียบเทียบ
float lastPosS2 = 90.0;

// --- Motor Control Functions ---
void setupMotors() {
  // Setup PWM channels สำหรับ motors
  ledcSetup(M1A_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M1B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M2A_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M2B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M3A_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M3B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M4A_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(M4B_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // Attach pins to channels
  ledcAttachPin(M1A_PIN, M1A_CHANNEL);
  ledcAttachPin(M1B_PIN, M1B_CHANNEL);
  ledcAttachPin(M2A_PIN, M2A_CHANNEL);
  ledcAttachPin(M2B_PIN, M2B_CHANNEL);
  ledcAttachPin(M3A_PIN, M3A_CHANNEL);
  ledcAttachPin(M3B_PIN, M3B_CHANNEL);
  ledcAttachPin(M4A_PIN, M4A_CHANNEL);
  ledcAttachPin(M4B_PIN, M4B_CHANNEL);
}

void motor(int motorNum, int speed) {
  // speed: -100 (full reverse) ถึง 100 (full forward)
  // ใช้ 8-bit PWM (0-255) ตามไฟล์ old
  int dutyCycle = abs(speed) * 2.55;  // แปลง 0-100 เป็น 0-255
  if (dutyCycle > 255) dutyCycle = 255;
  if (dutyCycle < 0) dutyCycle = 0;
  
  int channelA, channelB;
  
  switch(motorNum) {
    case 1: channelA = M1A_CHANNEL; channelB = M1B_CHANNEL; break;
    case 2: channelA = M2A_CHANNEL; channelB = M2B_CHANNEL; break;
    case 3: channelA = M3A_CHANNEL; channelB = M3B_CHANNEL; break;
    case 4: channelA = M4A_CHANNEL; channelB = M4B_CHANNEL; break;
    default: return;
  }
  
  if (speed > 0) {  // Forward
    ledcWrite(channelA, 255 - dutyCycle);
    ledcWrite(channelB, 255);
  } else if (speed < 0) {  // Reverse
    ledcWrite(channelA, 255);
    ledcWrite(channelB, 255 - dutyCycle);
  } else {  // Stop
    ledcWrite(channelA, 255);
    ledcWrite(channelB, 255);
  }
}

void allOff() {
  motor(1, 0);
  motor(2, 0);
  motor(3, 0);
  motor(4, 0);
}

void forward(int speed) {
  motor(1, speed);
  motor(2, speed);
  motor(3, speed);
  motor(4, speed);
}

void backward(int speed) {
  motor(1, -speed);
  motor(2, -speed);
  motor(3, -speed);
  motor(4, -speed);
}

void strafeLeft(int speed) {
  motor(1, -speed);
  motor(2, speed);
  motor(3, -speed);
  motor(4, speed);
}

void strafeRight(int speed) {
  motor(1, speed);
  motor(2, -speed);
  motor(3, speed);
  motor(4, -speed);
}

// --- Servo Control Functions (แบบไฟล์ old - แก้ปัญหาค้าง) ---
void setupServos() {
  // รอให้แรงดันไฟเสถียรก่อน
  delay(500);
  
  // ใช้ค่า pulse width ตาม NKP_Servo (544-2400)
  servo_s1.setPeriodHertz(50);  // 50Hz สำหรับ servo
  servo_s2.setPeriodHertz(50);
  
  servo_s1.attach(SERVO_S1_PIN, 544, 2400);  // NKP_Servo pulse width
  servo_s2.attach(SERVO_S2_PIN, 544, 2400);
  
  // ตั้งค่าเริ่มต้น - เขียนครั้งเดียวช้าๆ
  servo_s1.write((int)posS1);
  delay(100);
  servo_s2.write((int)posS2);
  delay(100);
  
  // บันทึกค่าเริ่มต้น
  lastPosS1 = posS1;
  lastPosS2 = posS2;
  
  Serial.println("Servos initialized at center position (NKP_Servo)");
}

// --- OLED Display Functions ---
void setupOLED() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32 Robot");
    display.println("Initializing...");
    display.display();
  }
}

void displayMacAddress(String mac) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 BT Address:");
  display.println("----------------");
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Use this for");
  display.println("PS3 pairing:");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println(mac);
  display.display();
}

void displayStatus(String status) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(status);
  display.display();
}

void displayConnected() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.println("PS3");
  display.println("Connected!");
  display.display();
}

// --- PS3 Controller Callbacks ---
void onConnect() {
  Serial.println("PS3 Controller Connected!");
  displayConnected();
  
  // เซ็ต LED และ rumble
  Ps3.setPlayer(1);
  delay(100);
}

void onDisconnect() {
  Serial.println("PS3 Controller Disconnected!");
  allOff();  // หยุด motor ทันที
  displayStatus("Disconnected\nWaiting...");
}

void notify() {
  // อ่านค่าจาก analog sticks
  int yAxis = Ps3.data.analog.stick.ly;
  int xAxis = Ps3.data.analog.stick.lx;
  int rxAxis = Ps3.data.analog.stick.rx;
  
  // 1. Analog Stick Control (Dead zone: -50 ถึง 50)
  if (yAxis <= -50) {
    forward(100);
  } else if (yAxis >= 50) {
    backward(100);
  } else if (xAxis >= 50) {
    strafeRight(100);
  } else if (xAxis <= -50) {
    strafeLeft(100);
  } else if (rxAxis >= 50) {
    // Rotate Clockwise
    motor(1, 50);
    motor(2, -50);
    motor(3, -50);
    motor(4, 50);
  } else if (rxAxis <= -50) {
    // Rotate Counter-Clockwise
    motor(1, -50);
    motor(2, 50);
    motor(3, 50);
    motor(4, -50);
  } else {
    // ไม่มี analog stick input - ตรวจสอบ D-Pad สำหรับมอเตอร์เท่านั้น
    if (Ps3.data.button.up) {
      forward(30);
    } else if (Ps3.data.button.down) {
      backward(30);
    } else if (Ps3.data.button.left) {
      strafeLeft(30);
    } else if (Ps3.data.button.right) {
      strafeRight(30);
    } else {
      // ไม่มีการควบคุมใดๆ - หยุด motor
      allOff();
    }
  }
  
  // 2. Servo Control - ใช้ปุ่มรูปร่าง (ตามไฟล์ old - ไม่มี debounce)
  // Servo 1 (S1): Cross=ลด, Triangle=เพิ่ม
  if (Ps3.data.button.cross) {
    posS1 -= 1.5;  // ❌ กากบาท - ลดทีละ 1.5 องศา
  }
  if (Ps3.data.button.triangle) {
    posS1 += 1.5;  // 🔺 สามเหลี่ยม - เพิ่มทีละ 1.5 องศา
  }
  
  // Servo 2 (S2): Square=ลด, Circle=เพิ่ม
  if (Ps3.data.button.square) {
    posS2 -= 1.5;  // ◻️ สี่เหลี่ยม - ลดทีละ 1.5 องศา
  }
  if (Ps3.data.button.circle) {
    posS2 += 1.5;  // ⭕ วงกลม - เพิ่มทีละ 1.5 องศา
  }
  
  // จำกัดค่ามุม servo (ตามไฟล์ old)
  if (posS1 > 170) posS1 = 170;
  if (posS1 < 20) posS1 = 20;
  if (posS2 > 150) posS2 = 150;
  if (posS2 < 20) posS2 = 20;
  
  // เขียนค่าไปที่ servo เฉพาะเมื่อมีการเปลี่ยนแปลงเท่านั้น
  if (abs(posS1 - lastPosS1) > 0.1) {
    servo_s1.write((int)posS1);
    lastPosS1 = posS1;
    Serial.print("S1: ");
    Serial.println((int)posS1);
  }
  if (abs(posS2 - lastPosS2) > 0.1) {
    servo_s2.write((int)posS2);
    lastPosS2 = posS2;
    Serial.print("S2: ");
    Serial.println((int)posS2);
  }
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  delay(1000);  // รอให้ระบบไฟเสถียร
  Serial.println("ESP32 PS3 Robot Starting...");
  
  // Initialize OLED
  setupOLED();
  
  // Initialize Motors ก่อน (ใช้ไฟน้อยกว่า)
  setupMotors();
  allOff();
  delay(300);
  
  // Initialize Servos ทีหลัง (แก้ปัญหาค้าง + brownout)
  setupServos();
  
  // Initialize PS3 Controller
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.attachOnDisconnect(onDisconnect);
  Ps3.begin();
  
  // Get and display Bluetooth MAC address
  String mac = Ps3.getAddress();
  Serial.println("==================================================");
  Serial.println("ESP32 Bluetooth MAC Address:");
  Serial.println(mac);
  Serial.println("==================================================");
  Serial.println("Use this address to pair your PS3 controller");
  Serial.println("Waiting for PS3 controller connection...");
  
  displayMacAddress(mac);
  delay(2000);
  
  displayStatus("Waiting for\nPS3 Controller...");
  Serial.println("System Ready!");
}

// --- Main Loop ---
void loop() {
  // ไม่ต้องอัพเดท servo ใน loop แล้ว (เขียนตรงใน notify แทน)
  delay(10);
}
