#include <TM1637.h>

const int clk = D3;
const int dio = D4;

TM1637 tm(clk, dio);

const int increase = D5;
const int decrease = D6;
const int startBtn = D7;
const int resetBtn = D0;
const int buzzer = D1;  // Buzzer pin

int defaultTime = 40; // minutes

// For button timing
unsigned long pressStart_inc = 0;
unsigned long pressStart_dec = 0;
unsigned long lastIncrementTime = 0;
unsigned long lastDecrementTime = 0;

bool pressed_inc = false;
bool pressed_dec = false;
bool longPressActive_inc = false;
bool longPressActive_dec = false;

// Timer state
bool timerRunning = false;
unsigned long lastTimerUpdate = 0;
int currentMinutes = 0;
int currentSeconds = 0;

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
  Serial.println("Countdown Timer");
}

void loop() {
  if (!timerRunning) {
    // Only handle increase/decrease when timer is not running
    handleIncreaseButton();
    handleDecreaseButton();
    handleResetButton();
    handleStartButton();
    displayTime(defaultTime, 0);
  } else {
    // Timer is running
    handleTimer();
    handleResetButton();  // Check reset even during timer
  }
}

//////////////////////////////////////////////////
// 🔼 INCREASE BUTTON (Short = +1, Long = auto-increment)
//////////////////////////////////////////////////
void handleIncreaseButton() {
  if (digitalRead(increase) == LOW && !pressed_inc) {
    delay(20); // debounce
    if (digitalRead(increase) == LOW) {
      pressed_inc = true;
      pressStart_inc = millis();
      lastIncrementTime = millis();
      longPressActive_inc = false;
    }
  }

  if (digitalRead(increase) == LOW && pressed_inc) {
    unsigned long duration = millis() - pressStart_inc;
    
    if (duration > 1000 && !longPressActive_inc) {
      // Long press detected, start auto-increment
      longPressActive_inc = true;
      defaultTime += 2;
      if (defaultTime > 999) defaultTime = 999;
      lastIncrementTime = millis();
    }
    
    if (longPressActive_inc && (millis() - lastIncrementTime >= 200)) {
      // Auto increment every 200ms
      defaultTime += 2;
      if (defaultTime > 999) defaultTime = 999;
      lastIncrementTime = millis();
    }
  }

  if (digitalRead(increase) == HIGH && pressed_inc) {
    delay(20);
    if (digitalRead(increase) == HIGH) {
      if (!longPressActive_inc) {
        // Short press
        defaultTime += 1;
        if (defaultTime > 999) defaultTime = 999;
      }
      pressed_inc = false;
      longPressActive_inc = false;
    }
  }
}

//////////////////////////////////////////////////
// 🔽 DECREASE BUTTON (Short = -1, Long = auto-decrement)
//////////////////////////////////////////////////
void handleDecreaseButton() {
  if (digitalRead(decrease) == LOW && !pressed_dec) {
    delay(20); // debounce
    if (digitalRead(decrease) == LOW) {
      pressed_dec = true;
      pressStart_dec = millis();
      lastDecrementTime = millis();
      longPressActive_dec = false;
    }
  }

  if (digitalRead(decrease) == LOW && pressed_dec) {
    unsigned long duration = millis() - pressStart_dec;
    
    if (duration > 1000 && !longPressActive_dec) {
      // Long press detected, start auto-decrement
      longPressActive_dec = true;
      if (defaultTime > 1) defaultTime -= 2;
      if (defaultTime < 0) defaultTime = 0;
      lastDecrementTime = millis();
    }
    
    if (longPressActive_dec && (millis() - lastDecrementTime >= 200)) {
      // Auto decrement every 200ms
      if (defaultTime > 1) defaultTime -= 2;
      if (defaultTime < 0) defaultTime = 0;
      lastDecrementTime = millis();
    }
  }

  if (digitalRead(decrease) == HIGH && pressed_dec) {
    delay(20);
    if (digitalRead(decrease) == HIGH) {
      if (!longPressActive_dec) {
        // Short press
        if (defaultTime > 0) defaultTime -= 1;
      }
      pressed_dec = false;
      longPressActive_dec = false;
    }
  }
}

//////////////////////////////////////////////////
// 🔄 RESET BUTTON
//////////////////////////////////////////////////
void handleResetButton() {
  if (digitalRead(resetBtn) == LOW) {
    delay(20);
    if (digitalRead(resetBtn) == LOW) {
      if (timerRunning) {
        // Stop timer and reset
        timerRunning = false;
        digitalWrite(buzzer, LOW);
      }
      defaultTime = 40;
      displayTime(defaultTime, 0);
      delay(200);
      
      // Wait for button release
      while (digitalRead(resetBtn) == LOW) {
        delay(10);
      }
    }
  }
}

//////////////////////////////////////////////////
// ▶ START BUTTON
//////////////////////////////////////////////////
void handleStartButton() {
  if (digitalRead(startBtn) == LOW) {
    delay(20);
    if (digitalRead(startBtn) == LOW) {
      if (!timerRunning && defaultTime > 0) {
        timerRunning = true;
        currentMinutes = defaultTime;
        currentSeconds = 0;  // Start at 0 seconds
        lastTimerUpdate = millis();
      }
      
      // Wait for button release
      while (digitalRead(startBtn) == LOW) {
        delay(10);
      }
    }
  }
}

//////////////////////////////////////////////////
// ⏱ TIMER FUNCTION (non-blocking)
//////////////////////////////////////////////////
void handleTimer() {
  if (timerRunning) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastTimerUpdate >= 1000) {
      lastTimerUpdate = currentTime;
      
      // Decrement logic: go from MM:00 to MM-1:59
      if (currentSeconds == 0) {
        if (currentMinutes > 0) {
          currentMinutes--;
          currentSeconds = 59;
        } else {
          // Timer finished
          timerRunning = false;
          // Beep pattern for timer end
          for (int i = 0; i < 3; i++) {
            digitalWrite(buzzer, HIGH);
            delay(500);
            digitalWrite(buzzer, LOW);
            delay(300);
          }
          return;
        }
      } else {
        currentSeconds--;
      }
      
      displayTime(currentMinutes, currentSeconds);
      
      // Beep for last 10 seconds (when minutes = 0 and seconds <= 10)
      if (currentMinutes == 0 && currentSeconds <= 10 && currentSeconds > 0) {
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
      }
    }
  }
}

//////////////////////////////////////////////////
// 🖥 DISPLAY FUNCTION (MM:SS)
//////////////////////////////////////////////////
void displayTime(int minutes, int seconds) {
  tm.display(0, minutes / 10);
  tm.display(1, minutes % 10);
  tm.display(2, seconds / 10);
  tm.display(3, seconds % 10);
  tm.point(1);
}