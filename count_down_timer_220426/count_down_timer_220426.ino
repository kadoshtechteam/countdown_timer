#include <TM1637.h>

// ==================== HARDWARE CONFIGURATION ====================
const uint8_t clk = D3;
const uint8_t dio = D4;
const uint8_t increase = D5;
const uint8_t decrease = D6;
const uint8_t startBtn = D7;
const uint8_t resetBtn = D0;
const uint8_t buzzer = D1;

TM1637 tm(clk, dio);

// ==================== TIMER CONFIGURATION ====================
const uint8_t DEFAULT_TIME = 40;
const uint8_t MAX_TIME = 99;
uint8_t minutes = DEFAULT_TIME;
uint8_t seconds = 0;

// ==================== TIMING CONSTANTS ====================
const uint8_t DEBOUNCE_DELAY = 20;
const uint16_t INITIAL_PRESS_DELAY = 300;  // Delay before continuous increment starts
const uint16_t REPEAT_DELAY = 200;         // Repeat every 200ms while held
const uint16_t COUNTDOWN_INTERVAL = 1000;

// ==================== STATE MACHINE ====================
enum State : uint8_t {
  SETUP,      // Setting time
  RUNNING,    // Countdown active
  FINISHED    // Timer complete, buzzer on
};
State state = SETUP;

// ==================== BUTTON STATE TRACKING ====================
struct ButtonState {
  uint8_t last;
  unsigned long pressTime;
  unsigned long repeatTime;
  bool continuousMode;  // True when in continuous increment mode
  bool firstPressHandled;  // True after first +1/-1 is applied
};

ButtonState incBtn = {HIGH, 0, 0, false, false};
ButtonState decBtn = {HIGH, 0, 0, false, false};
uint8_t lastResetState = HIGH;
uint8_t lastStartState = HIGH;
unsigned long lastResetTime = 0;
unsigned long lastTick = 0;

// ==================== FUNCTION DECLARATIONS ====================
void updateDisplay();
void checkResetButton();
void handleSetup();
void handleRunning();
void handleFinished();
void handleIncreaseButton();
void handleDecreaseButton();
void adjustTime(int8_t delta);

// ==================== SETUP ====================
void setup() {
  // Configure pins
  pinMode(increase, INPUT_PULLUP);
  pinMode(decrease, INPUT_PULLUP);
  pinMode(startBtn, INPUT_PULLUP);
  pinMode(resetBtn, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  
  // Initialize display
  tm.init();
  tm.set(3);
  
  Serial.begin(9600);
  Serial.println(F("=== COUNTDOWN TIMER ==="));
  updateDisplay();
}

// ==================== MAIN LOOP ====================
void loop() {
  checkResetButton();
  
  switch (state) {
    case SETUP:
      handleSetup();
      break;
    case RUNNING:
      handleRunning();
      break;
    case FINISHED:
      handleFinished();
      break;
  }
}

// ==================== STATE HANDLERS ====================
void handleSetup() {
  handleIncreaseButton();
  checkResetButton();
  handleDecreaseButton();
  checkResetButton();
  
  uint8_t startState = digitalRead(startBtn);
  if (startState == LOW && lastStartState == HIGH) {
    if (minutes > 0 || seconds > 0) {
      // Reset button states before transitioning
      incBtn.continuousMode = false;
      incBtn.firstPressHandled = false;
      decBtn.continuousMode = false;
      decBtn.firstPressHandled = false;
      
      state = RUNNING;
      lastTick = millis();
      Serial.print(F("RUNNING - "));
      Serial.print(minutes);
      Serial.print(F(":"));
      Serial.println(seconds);
    } else {
      Serial.println(F("Cannot start: Time is 00:00"));
    }
  }
  lastStartState = startState;
}

void handleRunning() {
  // Update other button states to prevent false triggers
  incBtn.last = digitalRead(increase);
  decBtn.last = digitalRead(decrease);
  lastStartState = digitalRead(startBtn);
  
  checkResetButton();
  
  unsigned long now = millis();
  // Handle millis() overflow safely
  if ((now - lastTick >= COUNTDOWN_INTERVAL) || (now < lastTick)) {
    lastTick = now;
    
    if (seconds > 0) {
      seconds--;
    } else if (minutes > 0) {
      minutes--;
      seconds = 59;
    } else {
      // Timer reached zero
      state = FINISHED;
      digitalWrite(buzzer, HIGH);
      Serial.println(F("FINISHED - TIME'S UP!"));
    }
    updateDisplay();
    
    #ifdef DEBUG
    Serial.print(minutes);
    Serial.print(F(":"));
    Serial.println(seconds);
    #endif
  }
  
  checkResetButton();
}

void handleFinished() {
  incBtn.last = digitalRead(increase);
  decBtn.last = digitalRead(decrease);
  lastStartState = digitalRead(startBtn);
  checkResetButton();
}

// ==================== BUTTON HANDLERS ====================
void handleIncreaseButton() {
  uint8_t current = digitalRead(increase);
  unsigned long now = millis();
  
  if (current == LOW && incBtn.last == HIGH) {
    // Button just pressed - apply +1 immediately
    incBtn.pressTime = now;
    incBtn.repeatTime = now;
    incBtn.continuousMode = false;
    incBtn.firstPressHandled = true;
    adjustTime(1);  // +1 on press
    incBtn.last = LOW;
  } 
  else if (current == LOW && incBtn.last == LOW) {
    // Button held - check if we should start continuous mode
    if (!incBtn.continuousMode && (now - incBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      // Start continuous mode
      incBtn.continuousMode = true;
      incBtn.repeatTime = now;
      adjustTime(2);  // First +2 in continuous mode
    }
    else if (incBtn.continuousMode && (now - incBtn.repeatTime >= REPEAT_DELAY)) {
      // Continue adding +2 every REPEAT_DELAY
      incBtn.repeatTime = now;
      adjustTime(2);
    }
  }
  else if (current == HIGH && incBtn.last == LOW) {
    // Button released - reset state
    incBtn.last = HIGH;
    incBtn.continuousMode = false;
    incBtn.firstPressHandled = false;
  }
}

void handleDecreaseButton() {
  uint8_t current = digitalRead(decrease);
  unsigned long now = millis();
  
  if (current == LOW && decBtn.last == HIGH) {
    // Button just pressed - apply -1 immediately
    decBtn.pressTime = now;
    decBtn.repeatTime = now;
    decBtn.continuousMode = false;
    decBtn.firstPressHandled = true;
    adjustTime(-1);  // -1 on press
    decBtn.last = LOW;
  } 
  else if (current == LOW && decBtn.last == LOW) {
    // Button held - check if we should start continuous mode
    if (!decBtn.continuousMode && (now - decBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      // Start continuous mode
      decBtn.continuousMode = true;
      decBtn.repeatTime = now;
      adjustTime(-2);  // First -2 in continuous mode
    }
    else if (decBtn.continuousMode && (now - decBtn.repeatTime >= REPEAT_DELAY)) {
      // Continue subtracting -2 every REPEAT_DELAY
      decBtn.repeatTime = now;
      adjustTime(-2);
    }
  }
  else if (current == HIGH && decBtn.last == LOW) {
    // Button released - reset state
    decBtn.last = HIGH;
    decBtn.continuousMode = false;
    decBtn.firstPressHandled = false;
  }
}

void adjustTime(int8_t delta) {
  int16_t newMinutes = (int16_t)minutes + delta;
  // Constrain to valid range
  if (newMinutes < 0) newMinutes = 0;
  if (newMinutes > MAX_TIME) newMinutes = MAX_TIME;
  minutes = (uint8_t)newMinutes;
  updateDisplay();
  
  #ifdef DEBUG
  Serial.print(F("Time adjusted: "));
  Serial.print(minutes);
  Serial.println(F(" min"));
  #endif
}

void checkResetButton() {
  uint8_t current = digitalRead(resetBtn);
  
  if (current == LOW && lastResetState == HIGH) {
    unsigned long now = millis();
    if (now - lastResetTime >= DEBOUNCE_DELAY) {
      minutes = DEFAULT_TIME;
      seconds = 0;
      state = SETUP;
      digitalWrite(buzzer, LOW);
      
      // Reset all button states completely
      incBtn.last = HIGH;
      incBtn.continuousMode = false;
      incBtn.firstPressHandled = false;
      decBtn.last = HIGH;
      decBtn.continuousMode = false;
      decBtn.firstPressHandled = false;
      
      updateDisplay();
      Serial.println(F("RESET to SETUP"));
      lastResetTime = now;
    }
  }
  lastResetState = current;
}

// ==================== DISPLAY ====================
inline void updateDisplay() {
  tm.display(0, minutes / 10);
  tm.display(1, minutes % 10);
  tm.display(2, seconds / 10);
  tm.display(3, seconds % 10);
  // Note: Not all TM1637 libraries support tm.point()
  // If your library doesn't have this method, comment out the line below
  // tm.point(true);  // Enable colon separator
}
