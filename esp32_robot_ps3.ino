// ESP32 Robot Control with PS3 Controller - Arduino Version
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
#define PWM_RESOLUTION 10  // 10-bit resolution (0-1023)

// Motor PWM Channels (0-7 สำหรับ motor)
#define M1A_CHANNEL 0
#define M1B_CHANNEL 1
#define M2A_CHANNEL 2
#define M2B_CHANNEL 3
#define M3A_CHANNEL 4
#define M3B_CHANNEL 5
#define M4A_CHANNEL 6
#define M4B_CHANNEL 7

// Servo objects (ใช้ ESP32Servo library แยกจาก PWM)
Servo servo_s1;
Servo servo_s2;

// OLED Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Global Variables
float posS1 = 90.0;
float posS2 = 90.0;
float targetS1 = 90.0;  // ตำแหน่งเป้าหมาย
float targetS2 = 90.0;
unsigned long lastServoUpdate = 0;
#define SERVO_UPDATE_INTERVAL 50  // ลด update rate เป็น 50ms (จาก 20ms)

// Servo Control Variables
bool servoMoving = false;
unsigned long lastButtonPress = 0;
#define BUTTON_DEBOUNCE 100  // ป้องกันการกดซ้ำเร็วเกินไป

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
  int dutyCycle = map(abs(speed), 0, 100, 0, 1023);
  
  int channelA, channelB;
  
  switch(motorNum) {
    case 1: channelA = M1A_CHANNEL; channelB = M1B_CHANNEL; break;
    case 2: channelA = M2A_CHANNEL; channelB = M2B_CHANNEL; break;
    case 3: channelA = M3A_CHANNEL; channelB = M3B_CHANNEL; break;
    case 4: channelA = M4A_CHANNEL; channelB = M4B_CHANNEL; break;
    default: return;
  }
  
  if (speed > 0) {  // Forward
    ledcWrite(channelA, 1023 - dutyCycle);
    ledcWrite(channelB, 1023);
  } else if (speed < 0) {  // Reverse
    ledcWrite(channelA, 1023);
    ledcWrite(channelB, 1023 - dutyCycle);
  } else {  // Stop
    ledcWrite(channelA, 1023);
    ledcWrite(channelB, 1023);
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

// --- Servo Control Functions (แก้ปัญหาค้าง) ---
void setupServos() {
  // ใช้ ESP32Servo library แทน PWM ธรรมดา
  // จะไม่มีปัญหา conflict กับ motor PWM
  servo_s1.setPeriodHertz(50);  // 50Hz สำหรับ servo
  servo_s2.setPeriodHertz(50);
  
  servo_s1.attach(SERVO_S1_PIN, 500, 2500);  // min, max pulse width
  servo_s2.attach(SERVO_S2_PIN, 500, 2500);
  
  // ตั้งค่าเริ่มต้น
  servo_s1.write(posS1);
  servo_s2.write(posS2);
  
  delay(100);  // รอให้ servo เคลื่อนที่ไปตำแหน่งเริ่มต้น
}

void updateServos() {
  // อัพเดท servo แบบ smooth interpolation
  if (millis() - lastServoUpdate >= SERVO_UPDATE_INTERVAL) {
    lastServoUpdate = millis();
    
    // Smooth movement - ค่อยๆ เคลื่อนที่ไปยังตำแหน่งเป้าหมาย
    float smoothFactor = 0.3;  // ยิ่งน้อย ยิ่งนุ่มนวล (0.1-0.5)
    
    posS1 += (targetS1 - posS1) * smoothFactor;
    posS2 += (targetS2 - posS2) * smoothFactor;
    
    // จำกัดค่ามุม
    posS1 = constrain(posS1, 20, 170);
    posS2 = constrain(posS2, 20, 150);
    targetS1 = constrain(targetS1, 20, 170);
    targetS2 = constrain(targetS2, 20, 150);
    
    // เขียนค่าไปที่ servo เฉพาะเมื่อเคลื่อนที่
    if (abs(posS1 - targetS1) > 0.5 || abs(posS2 - targetS2) > 0.5) {
      servo_s1.write((int)posS1);
      servo_s2.write((int)posS2);
      servoMoving = true;
    } else {
      // หยุดนิ่งแล้ว - เขียนค่าครั้งสุดท้ายแล้วไม่ต้องอัพเดทอีก
      if (servoMoving) {
        servo_s1.write((int)targetS1);
        servo_s2.write((int)targetS2);
        servoMoving = false;
      }
    }
  }
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
    // ไม่มี analog stick input - ตรวจสอบ D-Pad
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
  
  
  // 2. Servo Control - ใช้ปุ่มรูปร่างเท่านั้น (ไม่ใช้ D-Pad)
  unsigned long currentTime = millis();
  
  // Servo 1 (S1): Square=ลด, Circle=เพิ่ม
  if (Ps3.data.button.square) {
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      targetS1 -= 5.0;  // ◻️ สี่เหลี่ยม - ลดทีละ 5 องศา
      lastButtonPress = currentTime;
      Serial.print("S1 ลด -> ");
      Serial.println(targetS1);
    }
  }
  if (Ps3.data.button.circle) {
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      targetS1 += 5.0;  // ⭕ วงกลม - เพิ่มทีละ 5 องศา
      lastButtonPress = currentTime;
      Serial.print("S1 เพิ่ม -> ");
      Serial.println(targetS1);
    }
  }
  
  // Servo 2 (S2): Cross=ลด, Triangle=เพิ่ม
  if (Ps3.data.button.cross) {
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      targetS2 -= 5.0;  // ❌ กากบาท - ลดทีละ 5 องศา
      lastButtonPress = currentTime;
      Serial.print("S2 ลด -> ");
      Serial.println(targetS2);
    }
  }
  if (Ps3.data.button.triangle) {
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      targetS2 += 5.0;  // 🔺 สามเหลี่ยม - เพิ่มทีละ 5 องศา
      lastButtonPress = currentTime;
      Serial.print("S2 เพิ่ม -> ");
      Serial.println(targetS2);
    }
  }
  
  // Reset servo ไปตำแหน่งกลาง (กด Start)
  if (Ps3.data.button.start) {
    if (currentTime - lastButtonPress > BUTTON_DEBOUNCE) {
      targetS1 = 90.0;
      targetS2 = 90.0;
      lastButtonPress = currentTime;
      Serial.println("Servos reset to center");
    }
  }
  
  // แสดงตำแหน่ง servo (กด Select)
  if (Ps3.data.button.select) {
    if (currentTime - lastButtonPress > 500) {  // ทุก 500ms
      Serial.print("S1: ");
      Serial.print(posS1);
      Serial.print(" -> ");
      Serial.print(targetS1);
      Serial.print(" | S2: ");
      Serial.print(posS2);
      Serial.print(" -> ");
      Serial.println(targetS2);
      lastButtonPress = currentTime;
    }
  }
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 PS3 Robot Starting...");
  
  // Initialize OLED
  setupOLED();
  
  // Initialize Motors
  setupMotors();
  allOff();
  
  // Initialize Servos (แก้ปัญหาค้าง)
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
  delay(3000);
  
  displayStatus("Waiting for\nPS3 Controller...");
}

// --- Main Loop ---
void loop() {
  // อัพเดท servo position (แบบ non-blocking)
  updateServos();
  
  // Delay เล็กน้อยเพื่อไม่ให้ CPU เต็ม
  delay(10);
}
