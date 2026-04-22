#include <TM1637.h>

const int clk = D3;
const int dio = D4;

TM1637 tm(clk, dio);

const int increase = D5;
const int decrease = D6;
const int startBtn = D7;
const int resetBtn = D0;

int defaultTime = 40; // minutes

// For button timing
unsigned long pressStart_inc = 0;
unsigned long pressStart_dec = 0;

bool pressed_inc = false;
bool pressed_dec = false;

void setup() {
  pinMode(increase, INPUT_PULLUP);
  pinMode(decrease, INPUT_PULLUP);
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(resetBtn, INPUT_PULLUP);

  tm.init();
  tm.set(3);

  Serial.begin(9600);
  Serial.println("Countdown Timer");
}

void loop() {

  handleIncreaseButton();
  handleDecreaseButton();

  // Reset button (simple debounce)
  if (digitalRead(resetBtn) == LOW) {
    delay(20);
    if (digitalRead(resetBtn) == LOW) {
      defaultTime = 40;
      delay(200);
    }
  }

  // Start button
  if (digitalRead(startBtn) == LOW) {
    delay(20);
    if (digitalRead(startBtn) == LOW) {
      timer();
    }
  }

  displayTime(defaultTime, 0);
}

//////////////////////////////////////////////////
// 🔼 INCREASE BUTTON (Short = +1, Long = +2)
//////////////////////////////////////////////////
void handleIncreaseButton() {

  if (digitalRead(increase) == LOW && !pressed_inc) {
    delay(20); // debounce
    if (digitalRead(increase) == LOW) {
      pressed_inc = true;
      pressStart_inc = millis();
    }
  }

  if (digitalRead(increase) == HIGH && pressed_inc) {
    delay(20);
    if (digitalRead(increase) == HIGH) {

      unsigned long duration = millis() - pressStart_inc;

      if (duration > 1000) {
        defaultTime += 2; // long press
      } else {
        defaultTime += 1; // short press
      }

      pressed_inc = false;
    }
  }
}

//////////////////////////////////////////////////
// 🔽 DECREASE BUTTON (Short = -1, Long = -2)
//////////////////////////////////////////////////
void handleDecreaseButton() {

  if (digitalRead(decrease) == LOW && !pressed_dec) {
    delay(20); // debounce
    if (digitalRead(decrease) == LOW) {
      pressed_dec = true;
      pressStart_dec = millis();
    }
  }

  if (digitalRead(decrease) == HIGH && pressed_dec) {
    delay(20);
    if (digitalRead(decrease) == HIGH) {

      unsigned long duration = millis() - pressStart_dec;

      if (duration > 1000) {
        if (defaultTime > 1) defaultTime -= 2;
      } else {
        if (defaultTime > 0) defaultTime -= 1;
      }

      pressed_dec = false;
    }
  }
}

//////////////////////////////////////////////////
// ⏱ TIMER FUNCTION
//////////////////////////////////////////////////
void timer() {
  for (int i = defaultTime; i >= 0; i--) {
    for (int j = 59; j >= 0; j--) {

      displayTime(i, j);
      delay(1000);
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