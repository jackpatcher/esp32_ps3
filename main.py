# main.py - MicroPython Code for ESP32 Robot Control
# Adapted from Arduino Sketch, now using event-driven control via Ps3Controller class.
#14:2B:2F:C6:43:A2 esp32 ble
from machine import Pin, PWM, I2C
import time
from ps3_micropython import Ps3, Ps3Controller # Import the custom PS3 class
from ssd1306 import SSD1306_I2C  # Import OLED driver

# --- Motor Pin Definitions (H-Bridge/Driver Connections) ---
M1A_PIN = 5   # Motor 1 Pin A (Forward/PWM Control)
M1B_PIN = 17  # Motor 1 Pin B (Reverse/PWM Control)
M2A_PIN = 19  # Motor 2 Pin A
M2B_PIN = 18  # Motor 2 Pin B
M3A_PIN = 16  # Motor 3 Pin A
M3B_PIN = 4   # Motor 3 Pin B
M4A_PIN = 2   # Motor 4 Pin A
M4B_PIN = 15  # Motor 4 Pin B

# --- Servo Pin Definitions ---
SERVO_S1_PIN = 13  # Servo 1 (myservo)
SERVO_S2_PIN = 26  # Servo 2 (myservoB)

# --- OLED Pin Definitions ---
OLED_SCL_PIN = 22  # I2C Clock
OLED_SDA_PIN = 21  # I2C Data
OLED_WIDTH = 128
OLED_HEIGHT = 64

# --- Global State Variables ---
# Initial servo positions in degrees (0-180) - Must be global for modification inside notify()
posS1 = 90.0
posS2 = 90.0

# --- PWM Setup ---
PWM_FREQ = 5000  # 5 kHz frequency for motors
SERVO_FREQ = 50  # 50 Hz frequency for servos

# Create PWM objects for each motor pin
pwm_channels = {
    'M1A': PWM(Pin(M1A_PIN), freq=PWM_FREQ),
    'M1B': PWM(Pin(M1B_PIN), freq=PWM_FREQ),
    'M2A': PWM(Pin(M2A_PIN), freq=PWM_FREQ),
    'M2B': PWM(Pin(M2B_PIN), freq=PWM_FREQ),
    'M3A': PWM(Pin(M3A_PIN), freq=PWM_FREQ),
    'M3B': PWM(Pin(M3B_PIN), freq=PWM_FREQ),
    'M4A': PWM(Pin(M4A_PIN), freq=PWM_FREQ),
    'M4B': PWM(Pin(M4B_PIN), freq=PWM_FREQ),
}

# Create PWM objects for servos
servo_s1_pwm = PWM(Pin(SERVO_S1_PIN), freq=SERVO_FREQ)
servo_s2_pwm = PWM(Pin(SERVO_S2_PIN), freq=SERVO_FREQ)

# --- OLED Setup ---
try:
    i2c = I2C(0, scl=Pin(OLED_SCL_PIN), sda=Pin(OLED_SDA_PIN), freq=400000)
    oled = SSD1306_I2C(OLED_WIDTH, OLED_HEIGHT, i2c)
    oled_available = True
    print("OLED initialized successfully")
except Exception as e:
    print(f"OLED initialization failed: {e}")
    oled_available = False
    oled = None

# --- OLED Display Functions ---

def display_text(lines, clear=True):
    """Display multiple lines of text on OLED"""
    if not oled_available or oled is None:
        return
    
    try:
        if clear:
            oled.fill(0)
        
        y_pos = 0
        for line in lines:
            oled.text(line, 0, y_pos, 1)
            y_pos += 10
        
        oled.show()
    except Exception as e:
        print(f"OLED display error: {e}")

def display_mac_address(ble_mac, classic_mac):
    """Display MAC addresses on OLED"""
    if not oled_available or oled is None:
        return
    
    try:
        oled.fill(0)
        oled.text("ESP32 BT Info", 0, 0, 1)
        oled.text("-" * 16, 0, 10, 1)
        oled.text("BLE:", 0, 20, 1)
        oled.text(ble_mac[:17], 0, 30, 1)
        oled.text("Classic (PS3):", 0, 42, 1)
        oled.text(classic_mac[:17], 0, 52, 1)
        oled.show()
    except Exception as e:
        print(f"OLED display error: {e}")

def display_status(status_text):
    """Display status message on OLED"""
    if not oled_available or oled is None:
        return
    
    try:
        oled.fill(0)
        # Split text into multiple lines if needed
        words = status_text.split()
        lines = []
        current_line = ""
        
        for word in words:
            test_line = current_line + " " + word if current_line else word
            if len(test_line) <= 16:  # Max ~16 chars per line
                current_line = test_line
            else:
                lines.append(current_line)
                current_line = word
        
        if current_line:
            lines.append(current_line)
        
        y_pos = 0
        for line in lines[:6]:  # Max 6 lines
            oled.text(line, 0, y_pos, 1)
            y_pos += 10
        
        oled.show()
    except Exception as e:
        print(f"OLED display error: {e}")

# --- Servo Control Function ---

def servo_write(servo_pwm, angle):
    """
    Sets the servo angle (0-180 degrees) using PWM duty cycle.
    """
    min_duty = 1640  # 0.5ms pulse for 50Hz (approx 0 deg)
    max_duty = 8200  # 2.5ms pulse for 50Hz (approx 180 deg)
    
    # Ensure angle is within bounds for mapping
    angle = max(0, min(180, angle))
    
    duty = int(min_duty + (max_duty - min_duty) * angle / 180)
    servo_pwm.duty_u16(duty)

# --- Motor Control Functions ---

def map_speed_to_duty(speed):
    """Maps 0-100 speed input to 0-1023 PWM duty cycle."""
    motor_duty = int(abs(speed) * 1023 // 100)
    return min(1023, max(0, motor_duty))

def motor(pin_num, speed):
    """
    Controls a single motor using the non-standard two-PWM-pin logic.
    - Speed range: -100 (full reverse) to 100 (full forward).
    """
    
    if pin_num == 1:
        pin_a_pwm = pwm_channels['M1A']
        pin_b_pwm = pwm_channels['M1B']
    elif pin_num == 2:
        pin_a_pwm = pwm_channels['M2A']
        pin_b_pwm = pwm_channels['M2B']
    elif pin_num == 3:
        pin_a_pwm = pwm_channels['M3A']
        pin_b_pwm = pwm_channels['M3B']
    elif pin_num == 4:
        pin_a_pwm = pwm_channels['M4A']
        pin_b_pwm = pwm_channels['M4B']
    else:
        return
    
    max_duty = 1023
    motor_duty = map_speed_to_duty(speed)
    
    if speed > 0:  # Forward
        pin_a_pwm.duty(max_duty - motor_duty)
        pin_b_pwm.duty(max_duty)
    elif speed < 0:  # Reverse
        pin_a_pwm.duty(max_duty)
        pin_b_pwm.duty(max_duty - motor_duty)
    else:  # Stop (Brake)
        pin_a_pwm.duty(max_duty)
        pin_b_pwm.duty(max_duty)

# --- Robot Movement Abstraction Functions ---

def ao():
    """All Off (Stop all motors)."""
    motor(1, 0)
    motor(2, 0)
    motor(3, 0)
    motor(4, 0)

def fd(_Speed):
    """Forward movement for Mecanum/4-wheel drive."""
    motor(1, _Speed)
    motor(2, _Speed)
    motor(3, _Speed)
    motor(4, _Speed)

def bk(_Speed):
    """Backward movement for Mecanum/4-wheel drive."""
    motor(1, -_Speed)
    motor(2, -_Speed)
    motor(3, -_Speed)
    motor(4, -_Speed)

def sl(_Speed):
    """Strafe Left (Side Left) for Mecanum drive."""
    motor(1, -_Speed)
    motor(2, _Speed)
    motor(3, -_Speed)
    motor(4, _Speed)

def sr(_Speed):
    """Strafe Right (Side Right) for Mecanum drive."""
    motor(1, _Speed)
    motor(2, -_Speed)
    motor(3, _Speed)
    motor(4, -_Speed)

# --- PS3 Controller Callbacks (Event Handlers) ---

def notify():
    """
    Main event callback, executed when the controller state changes.
    This function contains the core control logic from the original Arduino sketch.
    """
    global posS1, posS2 # Must use global to modify the servo position variables
    
    # Read the current state from the global Ps3 object
    ps3_data = Ps3.data
    yAxisValue = ps3_data.analog.stick.ly
    xAxisValue = ps3_data.analog.stick.lx
    RXAxisValue = ps3_data.analog.stick.rx
    
    # Check Dead Zone/Threshold (-50 to 50)
    is_driving = False
    
    # 1. Analog Stick Control (Driving)
    if yAxisValue <= -50:
        fd(100)
        is_driving = True
    elif yAxisValue >= 50:
        bk(100)
        is_driving = True
    elif xAxisValue >= 50:
        sr(100)
        is_driving = True
    elif xAxisValue <= -50:
        sl(100)
        is_driving = True
    elif RXAxisValue >= 50: # Rotate Clockwise
        motor(1, 50)
        motor(2, -50)
        motor(3, -50)
        motor(4, 50)
        is_driving = True
    elif RXAxisValue <= -50: # Rotate Counter-Clockwise
        motor(1, -50)
        motor(2, 50)
        motor(3, 50)
        motor(4, -50)
        is_driving = True
    
    # 2. D-Pad Control (Lower Speed Movement/Override if not analog driving)
    if not is_driving:
        if ps3_data.button.left:
            sl(30)
        elif ps3_data.button.right:
            sr(30)
        elif ps3_data.button.up:
            fd(30)
        elif ps3_data.button.down:
            bk(30)
        else:
            ao() # Stop if no movement button is pressed
            
    # 3. Servo Control (Buttons)
    # The original code used a timer for cross button, which is complex in MicroPython.
    # We apply the change immediately upon button press being detected.
    if ps3_data.button.cross:
        posS1 -= 1.5
        print("Servo S1: Decrease")
    if ps3_data.button.triangle:
        posS1 += 1.5
        print("Servo S1: Increase")
    if ps3_data.button.square:
        posS2 -= 1.5
        print("Servo S2: Decrease")
    if ps3_data.button.circle:
        posS2 += 1.5
        print("Servo S2: Increase")
    
    # 4. Servo Angle Limiting and Writing
    if posS1 > 170: posS1 = 170.0
    if posS1 < 20: posS1 = 20.0
    if posS2 > 150: posS2 = 150.0
    if posS2 < 20: posS2 = 20.0
    
    servo_write(servo_s1_pwm, posS1)
    servo_write(servo_s2_pwm, posS2)
    
    # Debugging Output
    print(f"S1: {posS1:.1f} deg, S2: {posS2:.1f} deg, LY: {yAxisValue}, RX: {RXAxisValue}")

def onConnect():
    """Executed when the PS3 Controller connects."""
    print("Connected! MAC Address:", Ps3.getAddress())
    Ps3.setPlayer(1) # Set Player LED (Placeholder)
    Ps3.setRumble(0.5, 500) # Short rumble (Placeholder)

def onDisConnect():
    """Executed when the PS3 Controller disconnects."""
    print("Disconnected!")
    ao() # Stop motors immediately

# --- Main Execution ---

def setup():
    """Initializes hardware and the PS3 controller interface."""
    print("MicroPython Setup: Initializing pins, servos, and PS3 interface...")
    
    # Display initial message on OLED
    display_status("Initializing...")
    
    # Initial motor stop state
    ao()
    
    # Initial servo positions
    servo_write(servo_s1_pwm, posS1)
    servo_write(servo_s2_pwm, posS2)
    
    # Attach Callbacks and Start PS3 Connection
    Ps3.attach(notify)
    Ps3.attachOnConnect(onConnect)
    Ps3.attachOnDisconnect(onDisConnect)
    
    # NOTE: Ps3.begin() is now calling the real C-Binding initialization.
    # The loop will wait until the connection is successful.
    Ps3.begin()
    
    # แสดง Bluetooth address ของ ESP32 หลังจาก begin()
    print("\n" + "="*50)
    print("ESP32 Bluetooth Addresses:")
    print("-" * 50)
    ble_mac = Ps3.getAddress()
    bt_classic_mac = Ps3.getClassicAddress()
    print(f"BLE MAC:              {ble_mac}")
    print(f"Bluetooth Classic:    {bt_classic_mac}")
    print("-" * 50)
    print("For PS3 Controller, use: " + bt_classic_mac)
    print("="*50 + "\n")
    
    # แสดงบน OLED
    display_mac_address(ble_mac, bt_classic_mac)
    time.sleep(3)  # แสดง MAC address 3 วินาที
    
    print("Waiting for PS3 controller connection...")
    print("Please press the PS button on your controller.\n")
    
    # แสดงสถานะรอการเชื่อมต่อบน OLED
    display_status("Waiting for PS3 controller...")
    
    # รอจนกว่าจะเชื่อมต่อสำเร็จ (timeout 60 วินาที)
    if Ps3.waitForConnection(timeout_ms=60000):
        print("\n" + "="*50)
        print("Controller connected! You can now control the robot.")
        print("="*50 + "\n")
        
        # แสดงสถานะเชื่อมต่อสำเร็จบน OLED
        display_status("PS3 Connected! Ready to control")
        time.sleep(2)
        
        # Main loop - ทำงานต่อเนื่องหลังเชื่อมต่อสำเร็จ
        while True:
            Ps3.loop()
            time.sleep_ms(10)  # Small delay to prevent CPU hogging
    else:
        print("Failed to connect to PS3 controller.")
        print("Please check:")
        print("1. Controller is paired with Bluetooth Classic MAC:", Ps3.getClassicAddress())
        print("2. Controller battery is charged")
        print("3. Controller is in pairing mode")
        
        # แสดงสถานะล้มเหลวบน OLED
        display_status("Connection Failed! Check pairing")

if __name__ == "__main__":
    print("start main")
    setup()
 
