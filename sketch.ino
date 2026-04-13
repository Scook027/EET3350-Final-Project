#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
Servo feederServo;

// buzzer
int buzzer = 12;

// keypad
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// clock
int currentHour = 12;
int currentMinute = 0;
bool currentPM = false;

unsigned long previousMillis = 0;



// Timer struct
struct Timer {
  int hour;
  int minute;
  bool pm;
};

Timer timers[4];
int timerCount = 0;
int timerViewIndex = 0;
// menu
int menuState = 0;

void setup() {
  lcd.init();
  lcd.backlight();

  feederServo.attach(10);
  feederServo.write(0);

  pinMode(buzzer, LOW);

  lcd.print("Set Clock First");
  delay(2000);
  lcd.clear();
}

void loop() {
  digitalWrite(buzzer, LOW);
  
  updateClock();
  char key = keypad.getKey();

    if(key =='#'){
      dispenseFood();
    }

  switch(menuState) {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("A:Time B:SetTime");
      lcd.setCursor(0,1);
      lcd.print("C:SetFeed D:Exit");
      lcd.setCursor(0,2);
      lcd.print("#:Manual Feed");

      if(key == 'A') { lcd.clear(); menuState = 1; }
      if(key == 'B') { lcd.clear(); menuState = 2; }
      if(key == 'C') { lcd.clear(); menuState = 3; }
      break;

    case 1:
      showTime();
      if(key == 'B') {
        timerViewIndex += 2;
        if(timerViewIndex >=timerCount) timerViewIndex = 0;
        lcd.clear(); 
        }
        
      if(key == 'C') {
        timerViewIndex -= 2;
        if(timerViewIndex < 0) timerViewIndex = max(0, timerCount - 2);
        lcd.clear();
      }
      
      showTime();

      if(key == 'D') {
        lcd.clear();
        menuState = 0;
      }
      break;

    case 2:
      setClock();
      lcd.clear();
      menuState = 0;
      break;

    case 3:
      setTimer();
      lcd.clear();
      menuState = 0;
      break;
  }

  checkFeeding();
}

void updateClock() {
  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis >= 60000) {
    previousMillis = currentMillis;
    currentMinute++;

    if(currentMinute >= 60) {
      currentMinute = 0;
      currentHour++;

      if(currentHour == 12) {
        currentPM = !currentPM;
      }
      else if(currentHour > 12) {
        currentHour = 1;
      }
    }
  }
}
/*
void showTime() {
  lcd.setCursor(0,0);
  lcd.print("Time: ");

  lcd.print(currentHour);
  lcd.print(":");

  if(currentMinute < 10) lcd.print("0");
  lcd.print(currentMinute);

  lcd.print(currentPM ? " PM" : " AM");
}
*/
/*
void showTime() {

  // ===== Clear both rows manually (prevents ghost characters) =====
  lcd.setCursor(0,0);
  lcd.print("                "); // 16 spaces
  lcd.setCursor(0,1);
  lcd.print("                ");


//current time
  lcd.setCursor(0,0);

  lcd.print("T:");

  if(currentHour < 10) lcd.print(" ");
  lcd.print(currentHour);
  lcd.print(":");

  if(currentMinute < 10) lcd.print("0");
  lcd.print(currentMinute);
  lcd.print(currentPM ? "P" : "A");


  lcd.setCursor(11,0);
  lcd.print("Pg");
  lcd.print((timerViewIndex / 2) +1);

 //TIMERt

  lcd.setCursor(0,1);

  for(int i = 0; i < 2; i++) {

    int idx = timerViewIndex + i;

    if(idx >= timerCount) break;

    lcd.print("T");
    lcd.print(idx + 1);
    lcd.print(":");

    lcd.print(timers[idx].hour);
    lcd.print(":");

    if(timers[idx].minute < 10) lcd.print("0");
    lcd.print(timers[idx].minute);

    lcd.print(timers[idx].pm ? "P" : "A");

    if(i == 0) lcd.print(" ");
    
  }
*/
void showTime() {
  static int lastHour = -1;
  static int lastMinute = -1;
  static bool lastPM = -1;
  static int lastViewIndex = -1;

  // Only update screen if something changed
  if(currentHour == lastHour &&
     currentMinute == lastMinute &&
     currentPM == lastPM &&
     timerViewIndex == lastViewIndex) {
    return;
  }

  // Save last values
  lastHour = currentHour;
  lastMinute = currentMinute;
  lastPM = currentPM;
  lastViewIndex = timerViewIndex;

  lcd.clear();

  // ===== Top Row =====
  lcd.setCursor(0,0);

  lcd.print("T:");

  if(currentHour < 10) lcd.print(" ");
  lcd.print(currentHour);

  lcd.print(":");

  if(currentMinute < 10) lcd.print("0");
  lcd.print(currentMinute);

  lcd.print(currentPM ? "P" : "A");

  lcd.setCursor(11,0);
  lcd.print("Pg");
  lcd.print((timerViewIndex / 2) + 1);

  // ===== Bottom Row =====
  lcd.setCursor(0,1);

  for(int i = 0; i < 2; i++) {
    int idx = timerViewIndex + i;

    if(idx >= timerCount) break;

    lcd.print("T");
    lcd.print(idx + 1);
    lcd.print(":");

    lcd.print(timers[idx].hour);
    lcd.print(":");

    if(timers[idx].minute < 10) lcd.print("0");
    lcd.print(timers[idx].minute);

    lcd.print(timers[idx].pm ? "P" : "A");

    if(i == 0) lcd.print(" ");
  }
}

void setClock() {
  lcd.clear();
  lcd.print("Set HHMM:");

  currentHour = getNumber(2);
  currentMinute = getNumber(2);

  lcd.clear();
  lcd.print("1=AM 2=PM");

  char key;
  do {
    key = keypad.getKey();
  } while(key != '1' && key != '2');

  currentPM = (key == '2');
}

void setTimer() {
  if(timerCount >= 4) {
    lcd.clear();
    lcd.print("Max 4 timers");
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.print("Set HHMM:");

  int h = getNumber(2);
  int m = getNumber(2);

  lcd.clear();
  lcd.print("1=AM 2=PM");

  char key;
  do {
    key = keypad.getKey();
  } while(key != '1' && key != '2');

  timers[timerCount].hour = h;
  timers[timerCount].minute = m;
  timers[timerCount].pm = (key == '2');

  timerCount++;

  lcd.clear();
  lcd.print("Saved");
  delay(1500);
}

void checkFeeding() {
  for(int i = 0; i < timerCount; i++) {
    if(currentHour == timers[i].hour &&
       currentMinute == timers[i].minute &&
       currentPM == timers[i].pm) {

      dispenseFood();
      delay(60000);
    }
  }
}

void dispenseFood() {
  lcd.clear();
  lcd.print("Feeding...");

  feederServo.write(90);
  delay(2000);
  feederServo.write(0);

  digitalWrite(buzzer, HIGH);
  delay(1000);
  digitalWrite(buzzer, LOW);

  lcd.clear();
}

int getNumber(int digits) {
  int value = 0;
  char key;

  lcd.setCursor(0,1);

  for(int i = 0; i < digits; i++) {
    do {
      key = keypad.getKey();
    } while(key < '0' || key > '9');

    lcd.print(key);
    value = value * 10 + (key - '0');
  }

  return value;
}
