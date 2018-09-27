#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

// ***** ADAFRUIT LIBRARIES *****
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
//UPDATE TO uGFX !

// ***** SR LIBRARIES *****
#include "RTOS.h"
#include "commands.h"
#include "utils.h"
#include "gfx.h"

// ***** I2C ADDRESSING *****
#define OLED_ADDRESS  0x3C

// ***** SPI ADDRESSING *****
#define SD_CS   4
#define ACC_CS  6
#define RF_CS   9

// ***** RGB-LED ADDRESSING *****.
#define LED_R_PIN A3
#define LED_G_PIN A4
#define LED_B_PIN A5

// ***** SENSOR ADDRESSING *****
#define VBAT_PIN A9
#define MVT_PIN A11
#define RST_PIN 4
//#define VIB_PIN 3

// ***** BUTTON ADDRESSING *****
#define BTN_RIGHT 5
#define BTN_LEFT 6
#define BTN_UP 9
#define BTN_DOWN 10

// ***** RF ADDRESSES *****
#define REMOTE_ADDRESS  747
#define CAR_ADDRESS     752
#define RF_FREQ         915.0
#define ENCRYPTION_KEY  1379-taystar

// ***** CAR INFO *****
#define CAR_MAKE  honda
#define CAR_MODEL accord
#define CAR_YEAR  2004

static const int PROGMEM FW_Revision = 12;
static const int PROGMEM FW_Version = 2;

// ***** OLED *****
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define TRAY_HEIGHT   14
Adafruit_SSD1306 display(RST_PIN);

// ***** SD CARD *****
bool SD_AVAILABLE;
Sd2Card card;
SdVolume volume;
SdFile root;

// ***** BATTERY *****
static const int PROGMEM BAT_CAP = 250;

/*
 *  CONTROLS:
 *      DISABLED: none
 *      MAIN MENU: Left = kill engine, Left(held) = start engine, right = lock, right(held) = unlock, up/down = open menu list
 *      POPUP: Left/right = dismiss, up/down = none
 *      QUERY: Left = yes, Right = no, up/down = none
 *      Settings Menu: Left = back, Right = enter, up = navigate up, down = navigate down
*/

bool OS_Sleeping = false;
int OS_InactiveTime = 0;
int OS_MENUSTATE = 0;   // 0=Disabled, 1=Main Menu, 2=Popup, 3=Query, 4=ScrollMenu

int LED_R, LED_G, LED_B;

int SETTING_BTNHOLD = 350;
int SETTING_MVT = 100;
int SETTING_TIMEOUT = 30;

enum Button { LEFT, RIGHT, UP, DOWN };

bool btn_left_state, btn_right_state, btn_up_state, btn_down_state;
bool btn_left_pushed = false, btn_right_pushed = false, btn_up_pushed = false, btn_down_pushed = false;
int btn_left_held, btn_right_held, btn_up_held, btn_down_held;


int USER_InactiveTime = 0;
float SIG_RF;


void LED_Set(int _r, int _g, int _b)
{
    if (_r <= 0) { _r = 0; } else if (_r >= 255) { _r = 255; }
    if (_g <= 0) { _g = 0; } else if (_g >= 255) { _g = 255; }
    if (_b <= 0) { _b = 0; } else if (_b >= 255) { _b = 255; }
    analogWrite(LED_R_PIN, _r); analogWrite(LED_G_PIN, _g); analogWrite(LED_B_PIN, _b);
}

void LED_Set(int _val)
{
    LED_Set(_val, _val, _val);
}

void LED_Off()
{
    LED_Set(0,0,0);
}

/*void LED_Blink(int _reps, int _rate, int _r, int _g, int _b)
{
    for (int i = 0; i < _reps; i++)
    {
        LED_OFF();
        delay(_rate);
        LED_Set(_r, _g, _b);
        delay(_rate);
    }
    LED_OFF();
}

void LED_Blink(int _rate, int _r, int _g, int _b)
{
    LED_Blink(1, _rate, _r, _g, _b);
}

void LED_Blink(int _r, int _g, int _b)
{
    LED_Blink(1, 100, _r, _g, _b);
}*/

bool isActive()
{
    if (analogRead(MVT_PIN) >= SETTING_MVT)
    {
        return true;
    } else {
        return false;
    }
}

/*void Vibrate(int _reps, int _rate)
{
    digitalWrite(VIB_PIN, LOW);
    for (int i = 0; i < _reps; i++)
    {
        digitalWrite(VIB_PIN, HIGH);
        delay(_rate);
    }
    digitalWrite(VIB_PIN, LOW);
}

void Vibrate(int _duration)
{
    digitalWrite(VIB_PIN, HIGH);
    delay(_duration);
    digitalWrite(VIB_PIN, LOW);
}*/



void IO_Setup()
{
    pinMode(LED_R_PIN, OUTPUT);
    pinMode(LED_G_PIN, OUTPUT);
    pinMode(LED_B_PIN, OUTPUT);
    //pinMode(VIB_PIN, OUTPUT);
    pinMode(VBAT_PIN, INPUT);
    pinMode(BTN_LEFT, INPUT);
    pinMode(BTN_RIGHT, INPUT);
    pinMode(BTN_UP, INPUT);
    pinMode(BTN_DOWN, INPUT);
}

void OLED_Setup()
{
    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.display();
    display.clearDisplay();
    display.drawBitmap(0,0,splash_logo,128,64,WHITE);
    display.display();
}

void LED_Setup()
{
    LED_Off();
    delay(100);
    LED_Set(255,0,0);
    delay(100);
    LED_Set(0,255,0);
    delay(100);
    LED_Set(0,0,255);
    delay(100);
    LED_Off();
}

void RF_Setup()
{
  
}

void SD_Setup()
{
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    SD_AVAILABLE = false;
  } else {
    SD_AVAILABLE = true;
  }
}

void Accel_Setup()
{
  
}

/*void BootScreen(int _progress)
{
    int pseg = _progress/10;
    display.clearDisplay();
    display.setTextColor(BLACK,WHITE);
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("  BOOTING  ");
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(54,40);
    display.print(_progress); display.println("%");
    for (int i = 0; i <= 10; i++)
    {
        if (i <= pseg)
        {
            display.fillRect(2+(12*i),50,11,14,WHITE);
        } else {
            display.drawRect(2+(12*i),50,11,14,WHITE);
        }
    }
    display.display();
}*/

void BootUp()
{
    IO_Setup();
    OLED_Setup();
    LED_Setup();
    RF_Setup();
    SD_Setup();
    Accel_Setup();
    display.clearDisplay();
}



void Shutdown()
{

}

void Reboot()
{

}

void EnableSerial(int _baud)
{
    Serial.begin(_baud);
    Serial.println("Starting Serial...");
}

void DisableSerial()
{
    Serial.println("Ending Serial...");
    Serial.end();
}

float GetBatteryVoltage()
{
    return analogRead(VBAT_PIN)*6.6/1024;
}

float GetBatteryLevel()
{
    return (GetBatteryVoltage()-3)*100/1.2;
}

bool IsConnectedToCar()
{
    return true;
}

float SignalStrength()
{
    return random(0,100);
}

bool IsCarLocked()
{
    return true;
}

bool IsCarRunning()
{
    return false;
}

bool Alerts()
{
    return false;
}

void DrawBattery(int _p, int _x, int _y)
{
    display.drawRect(_x, _y, 16, 12, WHITE);
    display.fillRect(_x+16, _y+3, 2, 6, WHITE);
    if (_p >= 75)
    { display.fillRect(_x+11, _y+1, 4, 10, WHITE); }
    if (_p >= 50)
    { display.fillRect(_x+6, _y+1, 4, 10, WHITE); }
    if (_p >= 25)
    { display.fillRect(_x+1, _y+1, 4, 10, WHITE); }
}

void DrawBatteryLevel(int _p, int _x, int _y)
{
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(_x, _y);
    display.print(_p); display.println("%");
}

void DrawRfRange(int _r, int _x, int _y)
{
    if (_r >= 75)
    { display.fillRect(_x+12, _y, 2, 12, WHITE); }
    if (_r >= 50)
    { display.fillRect(_x+8, _y+3, 2, 9, WHITE); }
    if (_r >= 25)
    { display.fillRect(_x+4, _y+6, 2, 6, WHITE); }
    if (_r > 0)
    { display.fillRect(_x, _y+9, 2, 3, WHITE); }
    if (_r <= 0)
    {
        display.drawChar(_x+4, _y+4, 'X', WHITE, BLACK, 1);
    }
}

void DrawCarLock(bool _val, int _x, int _y)
{
    if (_val)
    {
        display.drawBitmap(_x, _y, icon_lock, 16, 16, WHITE);
    } else {
        display.drawBitmap(_x, _y, icon_unlock, 16, 16, WHITE);
    }
}

void DrawCarRun(bool _val, int _x, int _y)
{
    if (_val)
    {
        display.drawBitmap(_x, _y, icon_run, 16, 16, WHITE);
    }
}

void DrawAlerts(bool _val, int _x, int _y)
{
    if (_val)
    {
        display.drawBitmap(_x, _y, icon_alert, 16, 16, WHITE);
    }
}

void TrayBar(int _rfRange, bool _carLocked, bool _carRunning, bool _alerts, int _battery) //range, locked, running, alerts --- battery
{
    int iconSize = 14;
    int startHeight = 2;
    int padding = 2;
    const int trayBarheight = 14;

    DrawRfRange(_rfRange, 2, startHeight);
    DrawCarLock(_carLocked, 18, startHeight-1);
    DrawCarRun(_carRunning, 34, startHeight-1);
    DrawAlerts(_alerts, 50, startHeight-1);
    DrawBattery(_battery, 108, startHeight);
    DrawBatteryLevel(_battery, 82, startHeight + 4);
    display.display();
}

void TrayBar()
{
    TrayBar(SignalStrength(), IsCarLocked(), IsCarRunning(), Alerts(), GetBatteryLevel());
}

void DrawCarViews()
{
    display.drawBitmap(20,0,car_top,112,64,WHITE);

    display.display();
}

void DrawBtnInfo(int _m)
{
    // CHAR SIZE = 5 * 8
    display.setTextColor(WHITE);
    display.setTextSize(1);
    if (_m == 1)
    {
        display.setCursor(5, 56);
        display.print("start");
        display.setCursor(103, 56);
        display.print("lock");
    } else if (_m == 2) {
        
    }
    display.display();
}

void DrawButtonInfo(char _left[], char _right[])
{
    int padding = 5;
    int s = 128-(StringLength(_right)+padding);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(padding, 56);
    display.print(_left);
    display.setCursor(s, 56);
    display.print(_right);
}

void MainMenu()
{
    OS_MENUSTATE = 1;
    display.clearDisplay();

    TrayBar();
    DrawCarViews();
    DrawBtnInfo(OS_MENUSTATE);
}

/*void OnBtn (Button btn)
{
    
}

void OnBtn_Held (Button btn, int _d)
{

}

void OnBtn_Held (Button btn)
{
    OnBtn_Held (btn, 0);
}*/




/*void BtnListen(Button _btn, int _pin, bool _pushed, int _held)
{
    bool _state = digitalRead(_pin);
    if (_state == HIGH) {
        _pushed = true;
        _held += 1;
    }
    if (_pushed && _state == LOW)
    {  
        if (_held <= SETTING_BTNHOLD*10)
        {
            OnBtn (_btn);
            Serial.print(_btn); Serial.println(" button was pushed.");
        } else {
            OnBtn_Held(_btn, _held/10);
            Serial.print(_btn); Serial.print(" button was held for"); Serial.print(_held/10); Serial.println(" ms.");
        }
        _pushed = false;
        _held = 0;
    }
}*/

void ListenToBtns()
{
    /*BtnListen(LEFT, BTN_LEFT, btn_left_pushed, btn_left_held);
    BtnListen(RIGHT, BTN_RIGHT, btn_right_pushed, btn_right_held);
    BtnListen(UP, BTN_UP, btn_up_pushed, btn_up_held);
    BtnListen(DOWN, BTN_DOWN, btn_down_pushed, btn_down_held);*/

    
    btn_left_state = digitalRead(BTN_LEFT);
    if (btn_left_state == HIGH) {
        btn_left_pushed = true;
        btn_left_held += 1;
    }
    if (btn_left_pushed && btn_left_state == LOW)
    {  
        if (btn_left_held <= SETTING_BTNHOLD*10)
        {
            OnBtn_Left();
        } else {
            OnBtn_Left_Held(btn_left_held/10);
        }
        btn_left_pushed = false;
        btn_left_held = 0;
    }

    btn_right_state = digitalRead(BTN_RIGHT);
    if (btn_right_state == HIGH) {
        btn_right_pushed = true;
        btn_right_held += 1;
    }
    if (btn_right_pushed && btn_right_state == LOW)
    {  
        if (btn_right_held <= SETTING_BTNHOLD*10)
        {
            OnBtn_Right();
        } else {
            OnBtn_Right_Held(btn_right_held/10);
        }
        btn_right_pushed = false;
        btn_right_held = 0;
    }

    btn_up_state = digitalRead(BTN_UP);
    if (btn_up_state == HIGH) {
        btn_up_pushed = true;
        btn_up_held += 1;
    }
    if (btn_up_pushed && btn_up_state == LOW)
    {  
        if (btn_up_held <= SETTING_BTNHOLD*10)
        {
            OnBtn_Up();
        } else {
            OnBtn_Up_Held(btn_up_held/10);
        }
        btn_up_pushed = false;
        btn_up_held = 0;
    }

    btn_down_state = digitalRead(BTN_DOWN);
    if (btn_down_state == HIGH) {
        btn_down_pushed = true;
        btn_down_held += 1;
    }
    if (btn_down_pushed && btn_down_state == LOW)
    {  
        if (btn_down_held <= SETTING_BTNHOLD*10)
        {
            OnBtn_Down();
        } else {
            OnBtn_Down_Held(btn_down_held/10);
        }
        btn_down_pushed = false;
        btn_down_held = 0;
    }
}

void Sleep()
{
    display.dim(true);
    display.clearDisplay();
    display.display();
    OS_Sleeping = true;
}

void Awake()
{
    display.dim(false);
    MainMenu();
    display.display();
    OS_Sleeping = false;
}

int inactiveTicks = 0;

void ListenToActivity()
{
    int ticksPerSec = 1000;
    if (!isActive())
    {
        inactiveTicks += 1;
    } else {
        USER_InactiveTime = 0;
    }

    if (inactiveTicks >= ticksPerSec)
    {
        USER_InactiveTime += 1;
        inactiveTicks = 0;
        //Serial.print("USER has been inactive for "); Serial.print(USER_InactiveTime); Serial.println(" seconds.");
    }
}

// ***** BUTTON ACTIVITY *****

// LEFT

void OnBtn_Left()
{
    Serial.println("Left button was pushed.");
}

void OnBtn_Left_Held(int _d)
{
    Serial.print("Left button was held for (ms) "); Serial.println(_d);
}

void OnBtn_Left_Held()
{
    OnBtn_Left_Held(0);
}

// RIGHT

void OnBtn_Right()
{
    Serial.println("Right button was pushed.");
}

void OnBtn_Right_Held(int _d)
{
    Serial.print("Right button was held for (ms) "); Serial.println(_d);
}

void OnBtn_Right_Held()
{
    OnBtn_Right_Held(0);
}

// UP

void OnBtn_Up()
{
    Serial.println("Up button was pushed.");
}

void OnBtn_Up_Held(int _d)
{
    Serial.print("Up button was held for (ms) "); Serial.println(_d);
}

void OnBtn_Up_Held()
{
    OnBtn_Up_Held(0);
}

// DOWN

void OnBtn_Down()
{
    Serial.println("Down button was pushed.");
}

void OnBtn_Down_Held(int _d)
{
    Serial.print("Down button was held for (ms) "); Serial.println(_d);
}

void OnBtn_Down_Held()
{
    OnBtn_Down_Held(0);
}

// REMOTE WAS MOVED

void On_Movement()
{
    Serial.println("There was movement!");
}

// ***** LOOP FUNCTIONS *****

void OS_LOOP()
{
    // Updates every tick of the CPU
    ListenToBtns();
    ListenToActivity();
}

void OS_LOOP_10()
{
    // Updates every 10 ticks of the CPU
}

void OS_LOOP_100()
{
    // Updates every 100 ticks of the CPU
    
}

void OS_LOOP_1000()
{
    // Updates every 1000 ticks of the CPU
    
}

// **********

void setup() {
    BootUp();
    MainMenu();
}

// ***** CPU LIMITERS / DIVIDERS *****
int CPU_TICK = 0;
int CPU_TICK_10 = 0;
int CPU_TICK_100 = 0;
int CPU_TICK_1000 = 0;
bool OS_RUN = true;

void loop() {
    CPU_TICK_10 += 1;
    CPU_TICK_100 += 1;
    CPU_TICK_1000 += 1;
    OS_LOOP();
    if (CPU_TICK_10 >= 10) {
        OS_LOOP_10();
        CPU_TICK_10 = 0;
    }
    if (CPU_TICK_100 >= 100) {
        OS_LOOP_100();
        CPU_TICK_100 = 0;
    }
    if (CPU_TICK_1000 >= 1000) {
        OS_LOOP_1000();
        CPU_TICK_1000 = 0;
    }
}



