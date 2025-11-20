#include <Ps3Controller.h>
#include "NKP_Servo.h"

#define M1A 5
#define M1B 17
#define M2A 19
#define M2B 18
#define M3A 16
#define M3B 4
#define M4A 2
#define M4B 15
#define Knob_pin 33

Servo myservo;
Servo myservoB;

float posS1 = 90;
float posS2 = 90;

void notify() {
  int yAxisValue = (Ps3.data.analog.stick.ly);  //Left stick  - y axis - forward/backward car movement
  int xAxisValue = (Ps3.data.analog.stick.lx);  //Right stick - x axis - left/right car movement
  int RXAxisValue = (Ps3.data.analog.stick.rx);  //Right stick - x axis - left/right car movement

  //  Serial.println("    ");

  if (yAxisValue <= -50)                              //Move car Forward
  {
    fd(100);                                          //สามารถปรับความเร็วได้
  } else if (yAxisValue >= 50)                        //Move car Backward
  {
    bk(100);                                          //สามารถปรับความเร็วได้
  }

  else if (xAxisValue >= 50)                          //Move car Right
  {
    sr(100);                                          //สามารถปรับความเร็วได้
  } else if (xAxisValue <= -50)                       //Move car Left
  {
    sl(100);                                          //สามารถปรับความเร็วได้
  }


  else if (RXAxisValue >= 50)  //Move car Right
  {
    motor(1,50);                                      //สามารถปรับความเร็วได้
    motor(2,-50);                                     //สามารถปรับความเร็วได้
    motor(3,-50);                                     //สามารถปรับความเร็วได้
    motor(4,50);                                      //สามารถปรับความเร็วได้
  } else if (RXAxisValue <= -50)  //Move car Left
  {
    motor(1,-50);                                     //สามารถปรับความเร็วได้
    motor(2,50);                                      //สามารถปรับความเร็วได้
    motor(3,50);                                      //สามารถปรับความเร็วได้
    motor(4,-50);                                     //สามารถปรับความเร็วได้
  }


  else if (Ps3.data.button.left) {
    Serial.println("Pressing the cross button");
    sl(30);                                           //ถ้ากดปุ่ม ลูกศรซ้าย (D-Pad Left) สั่งให้หุ่นยนต์ เลี้ยวซ้าย ด้วยความเร็ว 30 สามารถปรับความเร็วได้
  } else if (Ps3.data.button.right) {
    Serial.println("Pressing the cross button");
    sr(30);                                           //ถ้ากดปุ่ม ลูกศรขวา (D-Pad Right) สั่งให้หุ่นยนต์ เลี้ยวขวา ด้วยความเร็ว 30 สามารถปรับความเร็วได้
  } else if (Ps3.data.button.up) {
    Serial.println("Pressing the cross button");
    fd(30);                                           //ถ้ากดปุ่ม ลูกศรขึ้น (D-Pad Up) สั่งให้หุ่นยนต์ เดินหน้า ด้วยความเร็ว 30 สามารถปรับความเร็วได้
  } else if (Ps3.data.button.down) {
    Serial.println("Pressing the cross button");
    bk(30);                                           //ถ้ากดปุ่ม ลูกศรลง (D-Pad Down) สั่งให้หุ่นยนต์ ถอยหลัง ด้วยความเร็ว 30 สามารถปรับความเร็วได้
  } else
  {
    ao();                                             //สั่ง หยุดมอเตอร์ทั้ง 4 ตัว
  }
  if (Ps3.data.button.cross) {
    Serial.println("Pressing the cross button");    //เมื่อกดปุ่ม กากบาท (Cross) ลดค่ามุมของเซอร์โว posS1 ลงทีละ 1.5 องศา สามารถเพิ่มได้แต่แนะนำลองเพิ่มทีละนิด 0.5-1
    int timer = millis();
    posS1 -= 1.5;
  }
  if (Ps3.data.button.triangle) {
    Serial.println("Pressing the triangle button");   //เมื่อกดปุ่ม สามเหลี่ยม (Triangle) เพิ่มมุม posS1 ขึ้นทีละ 1.5 องศา  สามารถเพิ่มได้แต่แนะนำลองเพิ่มทีละนิด 0.5-1
    posS1 += 1.5;
  }

  if (Ps3.data.button.square) {
    Serial.println("Pressing the square button");     //เมื่อกดปุ่ม สี่เหลี่ยม (Square) ลดมุม posS2 ลงทีละ 1.5 องศา สามารถเพิ่มได้แต่แนะนำลองเพิ่มทีละนิด 0.5-1
    posS2 -= 1.5;
  }

  if (Ps3.data.button.circle) {
    Serial.println("Pressing the circle button");     //เมื่อกดปุ่ม วงกลม (Circle)  เพิ่มมุม posS2 ขึ้นทีละ 1.5 องศา  สามารถเพิ่มได้แต่แนะนำลองเพิ่มทีละนิด 0.5-1
    posS2 += 1.5;
  }
  if (posS1 > 170) posS1 = 170;                       //ถ้า posS1 (องศาของ servo ตัวที่ 1) มีค่ามากกว่า 170 องศา → ให้จำกัดไว้ที่ 170  //ป้องกันเซอร์โวหมุนเกินมุมสูงสุดที่เราต้องการ
  if (posS1 < 20) posS1 = 20;                         //ถ้า posS1 น้อยกว่า 20 องศา → ให้จำกัดที่ 20 //ป้องกันการหมุนกลับเกินไปด้านต่ำเกิน
  if (posS2 > 150) posS2 = 150;                         //จำกัดค่ามากสุดของ posS2 (servo ตัวที่ 2) ไว้ที่ 90 องศา
  if (posS2 < 20) posS2 = 20;                           //จำกัดค่าต่ำสุดของ posS2 ไว้ที่ 0 องศา
  myservo.write((int)posS1);
  myservoB.write((int)posS2);
}

void onConnect() {
  Serial.println("Connected!.");
}

void onDisConnect() {
}

void motor(int pin, int Speeds) {
  int _SpeedsA;
  int _SpeedsB;
  int _SpeedsC;
  int _SpeedsD;
  if (pin == 1) {
    _SpeedsA = abs(Speeds);
    _SpeedsA = _SpeedsA * 2.55;
    if (_SpeedsA > 255) {
      _SpeedsA = 255;
    } else if (_SpeedsA < -255) {
      _SpeedsA = -255;
    }
    if (Speeds > 0) {
      ledcWrite(6, 255 - abs(_SpeedsA));
      ledcWrite(7, 255);
    } else if (Speeds <= 0) {
      ledcWrite(6, 255);
      ledcWrite(7, 255 - abs(_SpeedsA));
    }
  }
  if (pin == 2) {
    _SpeedsB = abs(Speeds);

    _SpeedsB = _SpeedsB * 2.55;
    if (_SpeedsB > 255) {
      _SpeedsB = 255;
    } else if (_SpeedsB < -255) {
      _SpeedsB = -255;
    }
    if (Speeds > 0) {
      ledcWrite(4, 255 - abs(_SpeedsB));
      ledcWrite(5, 255);
    } else if (Speeds <= 0) {
      ledcWrite(4, 255);
      ledcWrite(5, 255 - abs(_SpeedsB));
    }
  }
  if (pin == 3) {
    _SpeedsC = abs(Speeds);

    _SpeedsC = _SpeedsC * 2.55;
    if (_SpeedsC > 255) {
      _SpeedsC = 255;
    } else if (_SpeedsC < -255) {
      _SpeedsC = -255;
    }
    if (Speeds > 0) {
      ledcWrite(3, 255 - abs(_SpeedsC));
      ledcWrite(2, 255);
    } else if (Speeds <= 0) {
      ledcWrite(3, 255);
      ledcWrite(2, 255 - abs(_SpeedsC));
    }
  }
  if (pin == 4) {
    _SpeedsD = abs(Speeds);

    _SpeedsD = _SpeedsD * 2.55;
    if (_SpeedsD > 255) {
      _SpeedsD = 255;
    } else if (_SpeedsD < -255) {
      _SpeedsD = -255;
    }
    if (Speeds > 0) {
      ledcWrite(8, 255 - abs(_SpeedsD));
      ledcWrite(9, 255);
    } else if (Speeds <= 0) {
      ledcWrite(8, 255);
      ledcWrite(9, 255 - abs(_SpeedsD));
    }
  }
}
void MT(int speeda, int speedb, int time_speed) {
  motor(1, speeda);
  motor(2, speedb);
  delay(time_speed);
}
void ao() {
  ledcWrite(2, 255);
  ledcWrite(3, 255);
  ledcWrite(4, 255);
  ledcWrite(5, 255);
  ledcWrite(6, 255);
  ledcWrite(7, 255);
  ledcWrite(8, 255);
  ledcWrite(9, 255);
}
void aot(int _time) {
  motor(1, 0);
  motor(2, 0);
  delay(_time);
}
void fd(int _Speed) {
  motor(1, _Speed);
  motor(2, _Speed);
  motor(3, _Speed);
  motor(4, _Speed);
}
void bk(int _Speed) {
  motor(1, -_Speed);
  motor(2, -_Speed);
  motor(3, -_Speed);
  motor(4, -_Speed);
}
void sl(int _Speed) {
  motor(1, -_Speed);
  motor(2, _Speed);
  motor(3, -_Speed);
  motor(4, _Speed);
}
void sr(int _Speed) {
  motor(1, _Speed);
  motor(2, -_Speed);
  motor(3, _Speed);
  motor(4, -_Speed);
}
void fd2(int _Speed) {
  motor(1, _Speed);
  motor(2, _Speed);
}
void bk2(int _Speed) {
  motor(1, -_Speed);
  motor(2, -_Speed);
}
void sl2(int _Speed) {
  motor(1, -_Speed);
  motor(2, _Speed);
}
void sr2(int _Speed) {
  motor(1, _Speed);
  motor(2, -_Speed);
}


void setup() {
  Serial.begin(115200);
  Ps3.attach(notify);
  Ps3.attachOnConnect(onConnect);
  Ps3.attachOnDisconnect(onDisConnect);
  Ps3.begin();

  String address = Ps3.getAddress();

  Serial.print("The ESP32's Bluetooth MAC address is: ");
  Serial.println(address);
  Serial.println("Ready.");
  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(M2B, OUTPUT);
  pinMode(M3A, OUTPUT);
  pinMode(M3B, OUTPUT);
  pinMode(M4A, OUTPUT);
  pinMode(M4B, OUTPUT);
  pinMode(12, OUTPUT);
  ledcSetup(6, 5000, 8);
  ledcSetup(7, 5000, 8);
  ledcSetup(4, 5000, 8);
  ledcSetup(5, 5000, 8);
  ledcSetup(3, 5000, 8);
  ledcSetup(2, 5000, 8);
  ledcSetup(8, 5000, 8);
  ledcSetup(9, 5000, 8);
  ledcAttachPin(M1A, 6);
  ledcAttachPin(M1B, 7);
  ledcAttachPin(M2A, 4);
  ledcAttachPin(M2B, 5);
  ledcAttachPin(M3A, 3);
  ledcAttachPin(M3B, 2);
  ledcAttachPin(M4A, 8);
  ledcAttachPin(M4B, 9);

  myservo.attach(13); //Pin ของ Servo 1
  myservo.write(90);  //องศาเริ่มต้นของ Servo 1
  myservoB.attach(26);  //Pin ของ Servo 2
  myservoB.write(90); //องศาเริ่มต้นของ Servo 2
}

void loop() {
}