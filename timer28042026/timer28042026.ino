#include <TM1637.h>

// Hardware pins
const int clk = D3;
const int dio = D4;
TM1637 tm(clk, dio);

const int increase = D5;
const int decrease = D6;
const int startBtn = D7;
const int resetBtn = D0;
const int buzzer = D1;

// Global button state variables (needed by check functions)
int increase_state = HIGH;
int decrease_state = HIGH;
int startBtn_state = HIGH;
int resetBtn_state = HIGH;

// Timer configuration
int defaultTime = 40;
int minutes = 40;
int seconds = 0;

unsigned long lastTick = 0;

// Button state tracking
struct ButtonState {
  int last;
  unsigned long pressTime;
  unsigned long repeatTime;
  bool continuousMode;
  bool firstPressHandled;
};

ButtonState incBtn = {HIGH, 0, 0, false, false};
ButtonState decBtn = {HIGH, 0, 0, false, false};

// Timing constants
const unsigned long INITIAL_PRESS_DELAY = 300;  // Delay before continuous increment starts
const unsigned long REPEAT_DELAY = 200;         // Repeat every 200ms while held


// STATE MACHINE DEFINITION
enum TimerState {
  IDLE,      // Setting up time with increase/decrease buttons
  RUNNING    // Timer counting down
};

TimerState currentState = IDLE;

void setup() {
  pinMode(increase, INPUT_PULLUP);
  pinMode(decrease, INPUT_PULLUP);
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(resetBtn, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  
  digitalWrite(buzzer, LOW);  // Buzzer OFF initially
  
  tm.init();
  tm.set(3);
  
  Serial.begin(9600);
  Serial.println("=== COUNTDOWN TIMER STATE MACHINE ===");
  Serial.println("State: IDLE");
  displayTime(minutes, seconds);
  delay(20);
}

void loop() {
  // Read all button states and store in global variables
  increase_state = digitalRead(increase);
  decrease_state = digitalRead(decrease);
  startBtn_state = digitalRead(startBtn);
  resetBtn_state = digitalRead(resetBtn);

  // Always check reset button first (works in all states)
  resetcheck();

  // State machine action logic
  switch(currentState){
    case IDLE: 
      handleIncreaseButton();
      handleDecreaseButton();
      startcheck();
      resetcheck();
      break;

    case RUNNING:
    resetcheck();
      count();
      resetcheck();
      break;
  }
}

void handleDecreaseButton() {
  int current = digitalRead(decrease);
  unsigned long now = millis();
  
  if (current == LOW && decBtn.last == HIGH) {
    // Button just pressed - apply -1 immediately
    decBtn.pressTime = now;
    decBtn.repeatTime = now;
    decBtn.continuousMode = false;
    decBtn.firstPressHandled = true;
    minutes -= 1;  // -1 on press
    if (minutes < 0) minutes = 0;
    displayTime(minutes, seconds);
    decBtn.last = LOW;
  } 
  else if (current == LOW && decBtn.last == LOW) {
    // Button held - check if we should start continuous mode
    if (!decBtn.continuousMode && (now - decBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      // Start continuous mode
      decBtn.continuousMode = true;
      decBtn.repeatTime = now;
      minutes -= 2;  // First -2 in continuous mode
      if (minutes < 0) minutes = 0;
      displayTime(minutes, seconds);
    }
    else if (decBtn.continuousMode && (now - decBtn.repeatTime >= REPEAT_DELAY)) {
      // Continue subtracting -2 every REPEAT_DELAY
      decBtn.repeatTime = now;
      minutes -= 2;
      if (minutes < 0) minutes = 0;
      displayTime(minutes, seconds);
    }
  }
  else if (current == HIGH && decBtn.last == LOW) {
    // Button released - reset state
    decBtn.last = HIGH;
    decBtn.continuousMode = false;
    decBtn.firstPressHandled = false;
  }
}

void handleIncreaseButton() {
  int current = digitalRead(increase);
  unsigned long now = millis();
  
  if (current == LOW && incBtn.last == HIGH) {
    // Button just pressed - apply +1 immediately
    incBtn.pressTime = now;
    incBtn.repeatTime = now;
    incBtn.continuousMode = false;
    incBtn.firstPressHandled = true;
    minutes += 1;  // +1 on press
    if (minutes > 99) minutes = 99;
    displayTime(minutes, seconds);
    incBtn.last = LOW;
  } 
  else if (current == LOW && incBtn.last == LOW) {
    // Button held - check if we should start continuous mode
    if (!incBtn.continuousMode && (now - incBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      // Start continuous mode
      incBtn.continuousMode = true;
      incBtn.repeatTime = now;
      minutes += 2;  // First +2 in continuous mode
      if (minutes > 99) minutes = 99;
      displayTime(minutes, seconds);
    }
    else if (incBtn.continuousMode && (now - incBtn.repeatTime >= REPEAT_DELAY)) {
      // Continue adding +2 every REPEAT_DELAY
      incBtn.repeatTime = now;
      minutes += 2;
      if (minutes > 99) minutes = 99;
      displayTime(minutes, seconds);
    }
  }
  else if (current == HIGH && incBtn.last == LOW) {
    // Button released - reset state
    incBtn.last = HIGH;
    incBtn.continuousMode = false;
    incBtn.firstPressHandled = false;
  }
}

void startcheck(){
  static int lastStartState = HIGH;
  
  if(startBtn_state == LOW && lastStartState == HIGH){
    // Button just pressed (edge detection)
    resetcheck();
    if(minutes > 0 || seconds > 0){
      currentState = RUNNING;
      lastTick = millis();  // Initialize countdown timer
      Serial.print("Timer Started! Time: ");
      Serial.print(minutes);
      Serial.print(":");
      Serial.println(seconds);
      resetcheck();
      count();
    }
  }
  lastStartState = startBtn_state;
  
}

inline void resetcheck(){
  static int lastResetState = HIGH;
  
  // Edge detection: only trigger on button press (HIGH → LOW)
  if(resetBtn_state == LOW && lastResetState == HIGH){
    // Reset timer values
    minutes = defaultTime;
    seconds = 0;
    currentState = IDLE;
    
    // Turn off buzzer and update display
    digitalWrite(buzzer, LOW);
    displayTime(minutes, seconds);
    
    // Reset button states
    incBtn.last = HIGH;
    incBtn.continuousMode = false;
    incBtn.firstPressHandled = false;
    decBtn.last = HIGH;
    decBtn.continuousMode = false;
    decBtn.firstPressHandled = false;
    
    Serial.println("RESET!");
  }
  
  lastResetState = resetBtn_state;
}

void displayTime(int minutes, int seconds) {
  tm.display(0, minutes / 10);
  tm.display(1, minutes % 10);
  tm.display(2, seconds / 10);
  tm.display(3, seconds % 10);
}

void count(){
  // Always check reset button during countdown
  resetcheck();
  
  // Countdown logic
  unsigned long currentMillis = millis();
  if (currentMillis - lastTick >= 1000) {
    lastTick = currentMillis;
    
    // Check reset before any time change
    resetcheck();
    
    if (seconds == 0) {
      // Seconds transition: 00 → 59 (borrow from minutes)
      resetcheck();  // Allow reset before minute change
      
      if (minutes > 0) {
        minutes--;
        resetcheck();  // Allow reset after minute decrement
        seconds = 59;
        resetcheck();  // Allow reset after seconds reset
      } else {
        // Timer reached zero - transition to FINISHED state
        currentState = IDLE;
        digitalWrite(buzzer, HIGH);  // Turn buzzer ON only when reaching zero
        Serial.println("State: FINISHED");
        Serial.println("TIME'S UP!");
      }
    } else {
      // Normal seconds transition: decrement by 1
      resetcheck();  // Allow reset before seconds change
      seconds--;
      resetcheck();  // Allow reset after seconds change
    }
    
    displayTime(minutes, seconds);
    resetcheck();  // Allow reset after display update
  }
  
  // Check reset even when not updating (between seconds)
  resetcheck();
}
