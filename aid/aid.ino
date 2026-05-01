#include <TM1637.h>

// ==================== HARDWARE CONFIGURATION ====================
const uint8_t CLK_PIN = D3;
const uint8_t DIO_PIN = D4;
const uint8_t INC_PIN = D5;
const uint8_t DEC_PIN = D6;
const uint8_t START_PIN = D7;
const uint8_t RESET_PIN = D0;
const uint8_t BUZZER_PIN = D1;

TM1637 tm(CLK_PIN, DIO_PIN);

// ==================== TIMER CONFIGURATION ====================
const uint8_t DEFAULT_TIME = 40;
const uint8_t MAX_TIME = 99;
uint8_t minutes = DEFAULT_TIME;
uint8_t seconds = 0;

// ==================== TIMING CONSTANTS ====================
const uint16_t INITIAL_PRESS_DELAY = 300;
const uint16_t REPEAT_DELAY = 200;
const uint16_t COUNTDOWN_INTERVAL = 1000;

// ==================== STATE MACHINE ====================
enum State : uint8_t { IDLE, RUNNING };
State currentState = IDLE;

// ==================== BUTTON STATE TRACKING ====================
struct ButtonState {
  uint8_t last;
  unsigned long pressTime;
  unsigned long repeatTime;
  bool continuousMode;
};

ButtonState incBtn = {HIGH, 0, 0, false};
ButtonState decBtn = {HIGH, 0, 0, false};
uint8_t lastResetState = HIGH;
uint8_t lastStartState = HIGH;
unsigned long lastTick = 0;

// ==================== FUNCTION DECLARATIONS ====================
inline void updateDisplay();
inline void resetcheck();
inline void adjustTime(int8_t delta);
void handleIncreaseButton();
void handleDecreaseButton();
void startcheck();
void count();

// ==================== SETUP ====================
void setup() {
  // Configure pins
  pinMode(INC_PIN, INPUT_PULLUP);
  pinMode(DEC_PIN, INPUT_PULLUP);
  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize display (Grove library)
  tm.init();
  tm.set(BRIGHT_TYPICAL);  // Brightness level
  
  Serial.begin(9600);
  Serial.println(F("=== COUNTDOWN TIMER ==="));
  updateDisplay();
}

// ==================== MAIN LOOP ====================
void loop() {
  // Check reset first (highest priority)
  resetcheck();
  
  // State machine
  if (currentState == IDLE) {
    handleIncreaseButton();
    handleDecreaseButton();
    startcheck();
  } else {
    count();
  }
}

// ==================== RESET FUNCTION (INLINE FOR SPEED) ====================
inline void resetcheck() {
  uint8_t current = digitalRead(RESET_PIN);
  
  if (current == LOW && lastResetState == HIGH) {
    // Reset timer
    minutes = DEFAULT_TIME;
    seconds = 0;
    currentState = IDLE;
    digitalWrite(BUZZER_PIN, LOW);
    
    // Reset button states
    incBtn = {HIGH, 0, 0, false};
    decBtn = {HIGH, 0, 0, false};
    
    updateDisplay();
    Serial.println(F("RESET!"));
  }
  
  lastResetState = current;
}

// ==================== BUTTON HANDLERS ====================
void handleIncreaseButton() {
  uint8_t current = digitalRead(INC_PIN);
  unsigned long now = millis();
  
  if (current == LOW && incBtn.last == HIGH) {
    // Button pressed
    incBtn.pressTime = now;
    incBtn.continuousMode = false;
    adjustTime(1);
    incBtn.last = LOW;
  } 
  else if (current == LOW) {
    // Button held
    if (!incBtn.continuousMode && (now - incBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      incBtn.continuousMode = true;
      incBtn.repeatTime = now;
      adjustTime(2);
    }
    else if (incBtn.continuousMode && (now - incBtn.repeatTime >= REPEAT_DELAY)) {
      incBtn.repeatTime = now;
      adjustTime(2);
    }
  }
  else if (current == HIGH && incBtn.last == LOW) {
    // Button released
    incBtn.last = HIGH;
    incBtn.continuousMode = false;
  }
}

void handleDecreaseButton() {
  uint8_t current = digitalRead(DEC_PIN);
  unsigned long now = millis();
  
  if (current == LOW && decBtn.last == HIGH) {
    // Button pressed
    decBtn.pressTime = now;
    decBtn.continuousMode = false;
    adjustTime(-1);
    decBtn.last = LOW;
  } 
  else if (current == LOW) {
    // Button held
    if (!decBtn.continuousMode && (now - decBtn.pressTime >= INITIAL_PRESS_DELAY)) {
      decBtn.continuousMode = true;
      decBtn.repeatTime = now;
      adjustTime(-2);
    }
    else if (decBtn.continuousMode && (now - decBtn.repeatTime >= REPEAT_DELAY)) {
      decBtn.repeatTime = now;
      adjustTime(-2);
    }
  }
  else if (current == HIGH && decBtn.last == LOW) {
    // Button released
    decBtn.last = HIGH;
    decBtn.continuousMode = false;
  }
}

void startcheck() {
  uint8_t current = digitalRead(START_PIN);
  
  if (current == LOW && lastStartState == HIGH && (minutes > 0 || seconds > 0)) {
    currentState = RUNNING;
    lastTick = millis();
    Serial.print(F("START: "));
    Serial.print(minutes);
    Serial.print(F(":"));
    Serial.println(seconds);
  }
  
  lastStartState = current;
}

// ==================== COUNTDOWN FUNCTION ====================
void count() {
  resetcheck();
  
  unsigned long now = millis();
  if (now - lastTick >= COUNTDOWN_INTERVAL) {
    lastTick = now;
    
    resetcheck();
    
    if (seconds > 0) {
      seconds--;
    } else if (minutes > 0) {
      minutes--;
      seconds = 59;
      resetcheck();
    } else {
      // Timer complete
      currentState = IDLE;
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println(F("TIME'S UP!"));
    }
    
    updateDisplay();
    resetcheck();
  }
}

// ==================== HELPER FUNCTIONS ====================
inline void adjustTime(int8_t delta) {
  int16_t newMinutes = (int16_t)minutes + delta;
  minutes = (newMinutes < 0) ? 0 : (newMinutes > MAX_TIME) ? MAX_TIME : (uint8_t)newMinutes;
  updateDisplay();
}

inline void updateDisplay() {
  // Grove library uses display(position, value) method
  tm.display(0, minutes / 10);
  tm.display(1, minutes % 10);
  tm.display(2, seconds / 10);
  tm.display(3, seconds % 10);
}
