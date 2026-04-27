#include <TM1637.h>

const int clk = D3;
const int dio = D4;
TM1637 tm(clk, dio);

const int increase = D5;
const int decrease = D6;
const int startBtn = D7;
const int resetBtn = D0;

const int buzzer = D1;
int defaultTime = 40;
int minutes = 40;
int seconds = 0;
bool isRunning = false;
const unsigned long debounceDelay = 50;
const unsigned long longPressDelay = 500;  // 500ms to trigger long press
const unsigned long repeatDelay = 200;      // Repeat every 200ms during long press

unsigned long lastTick = 0;
unsigned long lastButtonPressTime = 0;
unsigned long lastRepeatTime = 0;
int lastIncreaseState = HIGH;
int lastDecreaseState = HIGH;
bool increaseLongPressActive = false;
bool decreaseLongPressActive = false;

void setup() {
  pinMode(increase, INPUT_PULLUP);
  pinMode(decrease, INPUT_PULLUP);
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(resetBtn, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  tm.init();
  tm.set(3);
  
  Serial.begin(9600);
  Serial.println("Timer Started - +1/-1 on single press, +2/-2 on long press");
  displayTime(minutes, seconds);
}

void loop() {
  // Read button states
  int increase_state = digitalRead(increase);
  int decrease_state = digitalRead(decrease);
  int startBtn_state = digitalRead(startBtn);
  int resetBtn_state = digitalRead(resetBtn);
  
  // Handle increase button
  handleIncreaseButton(increase_state);
  
  // Handle decrease button
  handleDecreaseButton(decrease_state);
  
  // Handle reset button
  if (resetBtn_state == LOW) {
    delay(debounceDelay);
    if (digitalRead(resetBtn) == LOW) {
      minutes = defaultTime;
      seconds = 0;
      isRunning = false;
      displayTime(minutes, seconds);
      digitalWrite(buzzer, LOW);
      Serial.println("Timer Reset!");
      delay(200);
    }
  }
  
  // Handle start/pause button
  if (startBtn_state == LOW) {
    delay(debounceDelay);
    if (digitalRead(startBtn) == LOW) {
      if (minutes > 0 || seconds > 0) {
        isRunning = !isRunning;
        Serial.println(isRunning ? "Timer Started!" : "Timer Paused!");
      }
      delay(200);
    }
  }
  
  // Timer countdown logic
  if (isRunning) {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastTick >= 1000) {
      lastTick = currentMillis;
      
      if (seconds == 0) {
        if (minutes > 0) {
          minutes--;
          seconds = 59;
        } else {
          isRunning = false;
          digitalWrite(buzzer, HIGH);
          Serial.println("TIME'S UP!");
          
          while(digitalRead(resetBtn) == HIGH && digitalRead(startBtn) == HIGH) {
            delay(100);
          }
          digitalWrite(buzzer, LOW);
          
          if(digitalRead(resetBtn) == LOW) {
            minutes = defaultTime;
            seconds = 0;
            displayTime(minutes, seconds);
          }
          delay(200);
        }
      } else {
        seconds--;
      }
      
      displayTime(minutes, seconds);
    }
  }
}

void handleIncreaseButton(int currentState) {
  if (currentState == LOW && lastIncreaseState == HIGH) {
    // Button just pressed
    lastButtonPressTime = millis();
    increaseLongPressActive = false;
    lastIncreaseState = LOW;
  } 
  else if (currentState == LOW && lastIncreaseState == LOW) {
    // Button is being held
    if (!increaseLongPressActive && (millis() - lastButtonPressTime >= longPressDelay)) {
      // Long press just started - add 2
      increaseLongPressActive = true;
      lastRepeatTime = millis();
      minutes += 2;
      if (minutes > 99) minutes = 99;
      displayTime(minutes, seconds);
      Serial.println("Long press +2");
    }
    else if (increaseLongPressActive && (millis() - lastRepeatTime >= repeatDelay)) {
      // Continue adding 2 every repeatDelay milliseconds
      minutes += 2;
      if (minutes > 99) minutes = 99;
      displayTime(minutes, seconds);
      lastRepeatTime = millis();
      Serial.println("Repeat +2");
    }
  }
  else if (currentState == HIGH && lastIncreaseState == LOW) {
    // Button released
    if (!increaseLongPressActive) {
      // Short press - add 1
      minutes += 1;
      if (minutes > 99) minutes = 99;
      displayTime(minutes, seconds);
      Serial.println("Single press +1");
    }
    lastIncreaseState = HIGH;
    increaseLongPressActive = false;
  }
}

void handleDecreaseButton(int currentState) {
  if (currentState == LOW && lastDecreaseState == HIGH) {
    // Button just pressed
    lastButtonPressTime = millis();
    decreaseLongPressActive = false;
    lastDecreaseState = LOW;
  } 
  else if (currentState == LOW && lastDecreaseState == LOW) {
    // Button is being held
    if (!decreaseLongPressActive && (millis() - lastButtonPressTime >= longPressDelay)) {
      // Long press just started - subtract 2
      decreaseLongPressActive = true;
      lastRepeatTime = millis();
      minutes -= 2;
      if (minutes < 0) minutes = 0;
      displayTime(minutes, seconds);
      Serial.println("Long press -2");
    }
    else if (decreaseLongPressActive && (millis() - lastRepeatTime >= repeatDelay)) {
      // Continue subtracting 2 every repeatDelay milliseconds
      minutes -= 2;
      if (minutes < 0) minutes = 0;
      displayTime(minutes, seconds);
      lastRepeatTime = millis();
      Serial.println("Repeat -2");
    }
  }
  else if (currentState == HIGH && lastDecreaseState == LOW) {
    // Button released
    if (!decreaseLongPressActive) {
      // Short press - subtract 1
      minutes -= 1;
      if (minutes < 0) minutes = 0;
      displayTime(minutes, seconds);
      Serial.println("Single press -1");
    }
    lastDecreaseState = HIGH;
    decreaseLongPressActive = false;
  }
}

void displayTime(int minutes, int seconds) {
  tm.display(0, minutes / 10);
  tm.display(1, minutes % 10);
  tm.display(2, seconds / 10);
  tm.display(3, seconds % 10);
}