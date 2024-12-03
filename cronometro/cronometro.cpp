#include <OneButtonTiny.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// LCD
const int LCD_COLUMNS = 16;
const int LCD_ROWS = 2;
const int LCD_REGISTER_PIN = 7;
const int LCD_ENABLE_PIN = 6;
const int LCD_DB4_PIN = 5;
const int LCD_DB5_PIN = 4;
const int LCD_DB6_PIN = 3;
const int LCD_DB7_PIN = 2;
LiquidCrystal lcd(LCD_REGISTER_PIN, LCD_ENABLE_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);

// Buttons
const int BTN_DOWN_PIN = 8;
const int BTN_UP_PIN = 9;
const int BTN_SWITCH_GO_PIN = 10;
const int BTN_MEMORY_1_PIN = 11;
const int BTN_MEMORY_2_PIN = 12;
const int BTN_CLICK_DETECT_TIME = 100;
OneButtonTiny btnSwitchGo = OneButtonTiny(BTN_SWITCH_GO_PIN,  true, true);
OneButtonTiny btnUp = OneButtonTiny(BTN_UP_PIN,  true, true);
OneButtonTiny btnDown = OneButtonTiny(BTN_DOWN_PIN,  true, true);
OneButtonTiny btnMemory1 = OneButtonTiny(BTN_MEMORY_1_PIN,  true, true);
OneButtonTiny btnMemory2 = OneButtonTiny(BTN_MEMORY_2_PIN,  true, true);

// Buzzer
const int BUZZER = 13;
const int SHORT_BEEP_DURATION = 200;

// Timing
const int MIN_SECONDS = 0;
const int MAX_SECONDS = 60 * 30;
const int MINUTES_SMALL_STEP = 1;
const int MINUTES_BIG_STEP = 5;
const int SECONDS_SMALL_STEP = 1;
const int SECONDS_BIG_STEP = 10;

// EEPROM memory
const int EEPROM_INIT_MARKER = 42;
const int MEMORY_ADDRESSES[] = {10, 20};
int memoryValues[] = {-1, -1};

// State machine
enum States {CONFIGURING, TICKING, ALERTING};
int state;

enum MenuOptions {MINUTES, SECONDS};
int currentMenuOption;
int lastDisplayedSeconds;
int remainingSeconds;
int elapsedMillisSinceLastDecrement;
int lastTickCheck;

void setUpEEPROM(){
  if (EEPROM.read(0) != EEPROM_INIT_MARKER) {
    // First execution: initialize first byte with marker value and other bytes with 0
    EEPROM.write(0, EEPROM_INIT_MARKER);
    for (int i = 1; i < EEPROM.length(); i++) {
      EEPROM.write(i, 0);
    }
  }
}

void setupPorts(){
  pinMode(BUZZER, OUTPUT);
}

void beep(){
  digitalWrite(BUZZER, HIGH);
  delay(SHORT_BEEP_DURATION);
  digitalWrite(BUZZER, LOW);
}

int readMemorySeconds(int memoryIndex){
  if (memoryValues[memoryIndex] >= 0){
    // value already read and stored in the global array: don't read from the memory again
    return memoryValues[memoryIndex];
  }

  int address = MEMORY_ADDRESSES[memoryIndex];
  byte lowByteValue = EEPROM.read(address);
  byte highByteValue = EEPROM.read(address + 1);
  int value = word(highByteValue, lowByteValue);
  value = constrain(value, MIN_SECONDS, MAX_SECONDS);
  memoryValues[memoryIndex] = value;
  return value;
}

void writeMemorySeconds(int memoryIndex, int seconds){
  if (memoryValues[memoryIndex] == seconds){
    // Memory already has the same value, no need to write it again
    beep();
    return;
  }

  int address = MEMORY_ADDRESSES[memoryIndex];
  EEPROM.write(address, lowByte(seconds));
  EEPROM.write(address + 1, highByte(seconds));
  memoryValues[memoryIndex] = seconds;
  beep();
}

void handleBtnSwitchGoClick(){
  switch (state){
    case CONFIGURING:
      // Switch menu option
      switch(currentMenuOption){
        case MINUTES:
          currentMenuOption = SECONDS;
          break;
        case SECONDS:
          currentMenuOption = MINUTES;
          break;
      }
      printMenu();
      break;
    case TICKING:
    case ALERTING:
      reset();
      break;
  }
}

void handleBtnSwitchGoLongPress(){
  switch (state){
    case CONFIGURING:
      if (remainingSeconds > 0){
        state = TICKING;
        lastTickCheck = millis();
        elapsedMillisSinceLastDecrement = 0;
        printTick();
      }
      else {
        beep();
      }
      break;
    case TICKING:
    case ALERTING:
      reset();
      break;
  }
}

void handleBtnUpClick(){
  if (state != CONFIGURING){
    return;
  }
  // Increment selected time by small step
  adjustTime(true, false);
  printMenu();
}

void handleBtnDownClick(){
  if (state != CONFIGURING){
    return;
  }
  // Decrement selected time by small step
  adjustTime(false, false);
  printMenu();
}

void handleBtnUpLongPress(){
  if (state != CONFIGURING){
    return;
  }
  // Increment selected time by big step
  adjustTime(true, true);
  printMenu();
}

void handleBtnDownLongPress(){
  if (state != CONFIGURING){
    return;
  }
  // Decrement selected time by big step
  adjustTime(false, true);
  printMenu();
}

void handleBtnMemory1Click(){
  if (state != CONFIGURING){
    return;
  }
  int seconds = readMemorySeconds(0);
  remainingSeconds = seconds;
  printMenu();
  handleBtnSwitchGoLongPress();
}

void handleBtnMemory2Click(){
  if (state != CONFIGURING){
    return;
  }
  int seconds = readMemorySeconds(1);
  remainingSeconds = seconds;
  printMenu();
  handleBtnSwitchGoLongPress();
}

void handleBtnMemory1LongPress(){
  if (state != CONFIGURING || remainingSeconds <= 0){
    return;
  }
  writeMemorySeconds(0, remainingSeconds);
  printMenu();
}

void handleBtnMemory2LongPress(){
  if (state != CONFIGURING || remainingSeconds <= 0){
    return;
  }
  writeMemorySeconds(1, remainingSeconds);
  printMenu();
}

void setupButtons(){
  // Click/press
  btnSwitchGo.attachClick(handleBtnSwitchGoClick);
  btnSwitchGo.attachLongPressStart(handleBtnSwitchGoLongPress);
  btnUp.attachClick(handleBtnUpClick);
  btnDown.attachClick(handleBtnDownClick);
  btnUp.attachLongPressStart(handleBtnUpLongPress);
  btnDown.attachLongPressStart(handleBtnDownLongPress);
  btnMemory1.attachClick(handleBtnMemory1Click);
  btnMemory2.attachClick(handleBtnMemory2Click);
  btnMemory1.attachLongPressStart(handleBtnMemory1LongPress);
  btnMemory2.attachLongPressStart(handleBtnMemory2LongPress);
  // timing
  btnSwitchGo.setClickMs(BTN_CLICK_DETECT_TIME);
  btnUp.setClickMs(BTN_CLICK_DETECT_TIME);
  btnDown.setClickMs(BTN_CLICK_DETECT_TIME);
  btnMemory1.setClickMs(BTN_CLICK_DETECT_TIME);
  btnMemory2.setClickMs(BTN_CLICK_DETECT_TIME);
}

void setupLcd(){
  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcd.clear();
}

void reset(){
  digitalWrite(BUZZER, LOW);
  state = CONFIGURING;
  remainingSeconds = MIN_SECONDS;
  elapsedMillisSinceLastDecrement = 0;
  lastTickCheck = 0;
  currentMenuOption = MINUTES;
  lastDisplayedSeconds = -1;
  printMenu();
}

void setup()
{
  setUpEEPROM();
  setupPorts();
  setupButtons();
  setupLcd();
  reset();
}

void printFormattedTime(int cursorColumn, int cursorRow, int totalSeconds){
  lcd.setCursor(cursorColumn, cursorRow);
  int minutes = abs(totalSeconds / 60);
  int seconds = abs(totalSeconds % 60);

  char buffer[7]; // "-MM:SS"
  snprintf(
    buffer, 
    sizeof(buffer), 
    "%c%02d:%02d", 
    totalSeconds < 0 ? '-' : ' ',
    minutes,
    seconds
  );
  lcd.print(buffer);
}

void printMenu(){
  lcd.clear();

  // Print current menu option
  lcd.setCursor(0, 0);
    switch(currentMenuOption){
    case MINUTES:
      lcd.print(" \\/   ");
      break;
    case SECONDS:
      lcd.print("    \\/");
      break;
  }

  // Print memory values
  int secondsM1 = readMemorySeconds(0);
  printFormattedTime(10, 0, secondsM1);
  lcd.setCursor(8, 0);
  lcd.print("M1>");
  
  int secondsM2 = readMemorySeconds(1);
  printFormattedTime(10, 1, secondsM2);
  lcd.setCursor(8, 1);
  lcd.print("M2>");

  // Print selected time
  printFormattedTime(0, 1, remainingSeconds);
}

void printTick(){
  // Print remaining time
  if (lastDisplayedSeconds == remainingSeconds){
    return;
  }
  lcd.clear();
  printFormattedTime(0, 1, remainingSeconds);
  lastDisplayedSeconds = remainingSeconds;
}

void adjustTime(bool isIncrement, bool isBigStep) {
  // Adjust the timer by incrementing/decrementing minutes/seconds by small/big steps.
  int increment = 0;
  int step = 1;

  int wholeMinutes = remainingSeconds / 60;
  if (currentMenuOption == MINUTES) {
    // Ensure that minutes rollover without affecting the minutes.
    step = isBigStep ? MINUTES_BIG_STEP : MINUTES_SMALL_STEP;
    
    // If decrementing and whole minutes is already < step, decrement the whole minutes without changing the seconds' part
    if (wholeMinutes < step and !isIncrement){
      increment = -wholeMinutes * 60;
    }
    else {
      increment = 60 * step;
      if (!isIncrement){
        increment = -increment;
      }
    }
  } else if (currentMenuOption == SECONDS){
    // Ensure that seconds rollover without affecting the minutes.
    step = isBigStep ? SECONDS_BIG_STEP : SECONDS_SMALL_STEP;

    int secondsModulus = remainingSeconds % 60;
    if (isIncrement){
      // If incrementing and the seconds part is already >= (60 - step), it must become 0 without changing the minutes' part.
      if (secondsModulus >= (60 - step)){
        increment = -secondsModulus;
      } else {
        increment = step;
      }
    } else {
      // If decrementing and the seconds part is already <= step, it must become 0 without changing the minutes' part.
      // Unless the seconds part is already 0, in which case it will become (60 - step) also without changing the minutes' part.
      if (secondsModulus == 0){
        increment = 60 - step;
      }
      else if (secondsModulus <= step){
        increment = -secondsModulus;
      } else {
        increment = -step;
      }
    }
  }
  else {
    return;
  }

  remainingSeconds = constrain(remainingSeconds + increment, MIN_SECONDS, MAX_SECONDS);
}

void tick(){
  // Decrement remainingSeconds at each elapsed second
  int currentMillis = millis();
  int delta = currentMillis - lastTickCheck;
  elapsedMillisSinceLastDecrement += delta;
  if (elapsedMillisSinceLastDecrement >= 1000){
    remainingSeconds--;
    elapsedMillisSinceLastDecrement = 0;
    if (state == ALERTING){
      digitalWrite(BUZZER, HIGH);
      remainingSeconds = constrain(remainingSeconds, -MAX_SECONDS, 0);
    }
    printTick();
  }
  lastTickCheck = currentMillis;
}

void loop()
{
  btnSwitchGo.tick();
  btnUp.tick();
  btnDown.tick();
  btnMemory1.tick();
  btnMemory2.tick();

  switch (state){
    case TICKING:
      tick();
      if (remainingSeconds <= 0){
        state = ALERTING;
        return;
      }
      break;
    case ALERTING:
      tick();
      break;
  }
}
