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
/*
The void set up is where I initialize all the components, like the LCD screen, the servo motor, and the buzzer. 
On startup, it also displays a message to set the clock.
*/

void loop() {
  digitalWrite(buzzer, LOW);
  
  updateClock();
  char key = keypad.getKey();

    if(key =='#'){  //manual feed
      dispenseFood();
    }

  switch(menuState) {    //menu navigation
    case 0:     //main menu
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

    case 1:  //vies time and timers
      showTime();
      if(key == 'B') {  //scroll forward in the timers screen
        timerViewIndex += 2;
        if(timerViewIndex >=timerCount) timerViewIndex = 0;
        lcd.clear(); 
        }
        
      if(key == 'C') {  //scroll backwards in the timers screen
        timerViewIndex -= 2;
        if(timerViewIndex < 0) timerViewIndex = max(0, timerCount - 2);
        lcd.clear();
      }
      
      showTime();

      if(key == 'D') {  //exit to main menu
        lcd.clear();
        menuState = 0;
      }
      break;

    case 2:  //set clock
      setClock();
      lcd.clear();
      menuState = 0;
      break;

    case 3:  //set timers
      setTimer();
      lcd.clear();
      menuState = 0;
      break;
  }

  checkFeeding();  //check if its time to feed
}
/*
The void loop is the main loop that runs continuously and controls the system. 
It updates the clock, reads the keypad inputs, navigates the menu system, allows the manual feed with the # key,
and calls the feeding check to trigger the automatic feeding.
*/

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
This is how i keep track of time, using millis().
 It increments minutes every 60,000ms, rolls over minutes to hours, and implements a 12-hour clock and AM/PM.
*/

/*
void showTime() {  //1st attempt
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
void showTime() {  //2nd attempt

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
  These are my different attemps at my showTime() function. The first 1 was the first attempt that worked, but only showed the time 
  it didnt show any timers. The second one was my first attempt to show the timers with the time, but when i set the clock, the time and 
  the page number would glitch together, and it was made worse when i added the timers.
*/

void showTime() { 
  static int lastHour = -1;
  static int lastMinute = -1;
  static bool lastPM = -1;
  static int lastViewIndex = -1;

  // Only update screen if something changed because of the issues i was having previously
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
/*
This was my succesfull showTime function, it displays the current time and page number in the top row, and up to 2 timers per page. 
  */

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
/*
this function allows the user to input the time, by putting 2-digits for the hour and minutes, then choose AM or PM
  */

void setTimer() {
  if(timerCount >= 4) {
    lcd.clear();
    lcd.print("Max 4 timers");
    delay(2000);
    return;
  }
/*
This function allows a max of 4 timers, a user input of AM/PM and stores the values in my timer array.
    */
  
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
/*
this function compares the current time with all the timers stored, and if a match is found it dispenses food
and then waits 60 seconds to prevent the servo from re-triggering.
  */

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
/*
This function controls the feeding process, to actuates the servo to release the food, activates the buzzer to alert you when the feeding is happening,
and it displays the feeding message on the LCD screen
  */

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
  /*
This functions reads the input from the keypad, only accepts the 0-9 when inputing the time or timers, displays the input on LCD and the 
keys are used, and returns the final value.
    */

  return value;
}
