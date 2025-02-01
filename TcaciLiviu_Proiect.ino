#include <LiquidCrystal.h>
#include <Bounce2.h>

// ------------------- Buzzer Configuration -------------------
// If you have a buzzer connected, define its pin here.
// If not using a buzzer, you can ignore or remove buzzer logic.
const int buzzerPin = 10; 
const unsigned long timeIsUpBlinkDuration = 5000; // 5 seconds to beep/flash at countdown end

// ------------------- LCD Configuration -------------------
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// ------------------- Button Pins -------------------
const int buttonStartStopPin = 7; 
const int buttonResetPin     = 8;
const int buttonLapPin       = 9;

// ------------------- Debouncer Objects -------------------
Bounce debouncerStartStop = Bounce();
Bounce debouncerReset     = Bounce();
Bounce debouncerLap       = Bounce();

// ------------------- Stopwatch Variables -------------------
//
// totalStartTime, totalElapsedTime: used to measure the total time
// in stopwatch mode. When you press Start, totalStartTime is set
// so we can measure how long it's been running.
//
unsigned long totalStartTime = 0;   
unsigned long totalElapsedTime = 0; 
bool isRunning = false;             // Is the stopwatch currently running?

bool isResetting = false;           
unsigned long resetStartTime = 0;   
const unsigned long resetDisplayDuration = 2000; // 2s display "Time reseted"

//
// We store laps in an array from Lap0..Lap9. Each 'Lap' has:
//   number: index from 0..9
//   time:   the totalElapsedTime at the start of that lap
//
struct Lap {
  int number;
  unsigned long time;
};
const int maxLaps = 10; 
Lap laps[maxLaps];

//
// lapStartTime:  moment we created the most recent lap
// lapCount:      index used for new laps (0..9, cycles)
// previousLapTime, previousLapNumber: used for blinking logic
//
unsigned long lapStartTime    = 0; 
unsigned long previousLapTime = 0; 
int previousLapNumber         = 0;
unsigned long lapCount        = 0;  

//
// Blinking logic for showing old Lap times, e.g. for 2s
//
bool isBlinkingLapTime = false;
unsigned long blinkStartTime = 0;
const unsigned long blinkDuration = 2000;  // 2s to blink
const unsigned long blinkInterval = 200;   // how quickly we blink
unsigned long lastBlinkToggle = 0;
bool blinkState = false;

//
// lastTimeStr: storing the last displayed text for line 1, so we only update if it changed
//
String lastTimeStr = "";

// ------------------- Timer (Countdown) Variables -------------------
//
// inTimerMode: toggles whether we are in Timer or Stopwatch
// timerMinutes, timerSeconds: the countdown time
// timerRunning: whether we're actively decrementing
//
bool inTimerMode    = false;       
unsigned int timerMinutes  = 0;    
unsigned int timerSeconds  = 0;    
bool timerRunning   = false;       

//
// lastTimerDecrement: used to track the last time we decremented the timer by 1s
//
unsigned long lastTimerDecrement = 0;  

//
// When countdown hits 00:00, we blink "TIME's UP!" for 5s
// timeIsUpBlinking: indicates that blinking is in progress
// timeIsUpBlinkStart: when we started blinking
//
bool timeIsUpBlinking = false;
unsigned long timeIsUpBlinkStart = 0;

// ------------------- Buzzer Variables -------------------
//
// If we beep for 5s at "TIME's UP," we track how long we've been beeping
//
bool buzzerActive = false; 
unsigned long buzzerStartTime = 0;

// ------------------- Long-Press on Reset -------------------
// We differentiate short press vs. long press on Reset (≥ 1s).
//
unsigned long resetPressStartTime = 0;
bool resetPressInProgress         = false;
const unsigned long RESET_LONG_PRESS_THRESHOLD = 500; // 0.5 second threshold

// ------------------- Setup & Utility Functions -------------------

//
// formatTime(msVal):
//  Convert a millisecond value into "H:MM:SS.s" format for the stopwatch.
//
String formatTime(unsigned long msVal) {
  unsigned long hours = msVal / 3600000;      
  unsigned long minutes = (msVal / 60000) % 60;
  unsigned long seconds = (msVal / 1000) % 60;
  unsigned long milliseconds = (msVal % 1000) / 100;

  String formattedTime;
  formattedTime += String(hours) + ":";
  if (minutes < 10) formattedTime += "0";
  formattedTime += String(minutes) + ":";
  if (seconds < 10) formattedTime += "0";
  formattedTime += String(seconds) + ".";
  formattedTime += String(milliseconds);

  return formattedTime;
}

//
// formatTimer(mm, ss):
//  For the countdown, produce a "MM:SS" string (no hours).
//
String formatTimer(unsigned int mm, unsigned int ss) {
  String result;
  if (mm < 10) result += "0";
  result += String(mm) + ":";
  if (ss < 10) result += "0";
  result += String(ss);
  return result;
}

void setup() {
  Serial.begin(9600);

  // If using a buzzer:
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  lcd.begin(16, 2);
  lcd.print("Stopwatch Ready");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Press Start      ");
  lcd.setCursor(0, 1);
  lcd.print("Time: 0:00:00.0 ");
  lastTimeStr = "Time: 0:00:00.0 ";

  pinMode(buttonStartStopPin, INPUT);
  pinMode(buttonResetPin, INPUT);
  pinMode(buttonLapPin, INPUT);

  debouncerStartStop.attach(buttonStartStopPin);
  debouncerStartStop.interval(25);

  debouncerReset.attach(buttonResetPin);
  debouncerReset.interval(25);

  debouncerLap.attach(buttonLapPin);
  debouncerLap.interval(25);
}

void loop() {
  debouncerStartStop.update();
  debouncerReset.update();
  debouncerLap.update();

  unsigned long currentMillis = millis();

  // If a buzzer beep is active, update that
  handleBuzzer(currentMillis);

  // Detect short vs long press on Reset
  handleResetButtonLogic(currentMillis);

  // If not in Timer mode => run the normal Stopwatch logic
  if (!inTimerMode) {
    handleStopwatchLogic(currentMillis);
  } 
  else {
    // If in Timer mode => do countdown logic
    handleTimerLogic(currentMillis);
  }

  // If "TIME's UP" is blinking for 5s in Timer mode
  if (timeIsUpBlinking) {
    if (currentMillis - timeIsUpBlinkStart <= timeIsUpBlinkDuration) {
      unsigned long elapsedBlink = (currentMillis - timeIsUpBlinkStart);
      unsigned long halfSecond   = 500;
      unsigned long blinkPhase   = (elapsedBlink / halfSecond) % 2; 
      lcd.setCursor(0, 0);
      if (blinkPhase == 0) {
        lcd.print("TIME's UP!       ");
      } else {
        lcd.print("                ");
      }
    } 
    else {
      // 5 seconds of blinking done => clear & show something else
      timeIsUpBlinking = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Timer: 00:00");
      lcd.setCursor(0, 1);
      lcd.print("Reset Timer     ");
    }
  }
}

// ------------------- STOPWATCH LOGIC -------------------
//
// handleStopwatchLogic(currentMillis):
//  - Manages Start/Stop button in stopwatch mode
//  - Manages Lap button
//  - Updates display
//
void handleStopwatchLogic(unsigned long currentMillis) {
  // Pressing the Start/Stop button
  if (debouncerStartStop.fell()) {
    // If stopwatch is currently STOPPED => start it
    if (!isRunning && !isResetting) {
      totalStartTime = currentMillis - totalElapsedTime;
      isRunning = true;
      Serial.println("Stopwatch => started.");

      // If it's the very first start => create Lap0
      if (lapCount == 0 && totalElapsedTime == 0) {
        startNewLap();
        Serial.println("Stopwatch => first Lap (Lap0) started as reference.");
      }
    }
    // If stopwatch is RUNNING => stop it
    else if (isRunning && !isResetting) {
      totalElapsedTime = currentMillis - totalStartTime;
      isRunning = false;
      Serial.println("Stopwatch => stopped.");

      int lapIndex = lapCount % maxLaps;
      previousLapTime = totalElapsedTime - laps[lapIndex].time;
      previousLapNumber = laps[lapIndex].number;

      // Show final lap time on line 0
      lcd.setCursor(0, 0);
      lcd.print("Lap" + String(previousLapNumber) + ": "
                + formatTime(previousLapTime) + "   ");
    }
  }

  // Pressing the Lap button => record a new lap if running
  if (debouncerLap.fell() && isRunning && !isResetting) {
    unsigned long currentTotalTime = currentMillis - totalStartTime;

    // The old lap index is the current lap
    int lapIndexPrev = lapCount % maxLaps;
    previousLapTime = currentTotalTime - laps[lapIndexPrev].time;
    previousLapNumber = laps[lapIndexPrev].number;

    prepareNewLap(); // increments lapCount, sets new-lap reference

    // We'll blink the lap time for ~2s
    isBlinkingLapTime = true;
    blinkStartTime = currentMillis;
    lastBlinkToggle = currentMillis;
    blinkState = true;
  }

  // If we are blinking the old lap time
  if (isBlinkingLapTime) {
    if ((currentMillis - blinkStartTime) <= blinkDuration) {
      // Toggle every 'blinkInterval' ms
      if (currentMillis - lastBlinkToggle >= blinkInterval) {
        lastBlinkToggle = currentMillis;
        blinkState = !blinkState;
        lcd.setCursor(0, 0);
        if (blinkState) {
          lcd.print("Lap" + String(previousLapNumber) + ": "
                    + formatTime(previousLapTime) + "   ");
        } else {
          lcd.print("                ");
        }
      }
      // Meanwhile, line 1 => total time
      String timeStr = "Time: " + formatTime(totalElapsedTime) + " ";
      lcd.setCursor(0, 1);
      lcd.print(timeStr);
    } 
    else {
      // done blinking => show the new Lap line
      isBlinkingLapTime = false;
      startNewLap(); // sets line 0 => "Lap#: 0..."
    }
  }

  // If the stopwatch is running => update totalElapsedTime
  if (isRunning) {
    totalElapsedTime = currentMillis - totalStartTime;
  }

  // Display total time on line 1 if not blinking & not resetting
  if (!isBlinkingLapTime && !isResetting) {
    String currentTimeStr = "Time: " + formatTime(totalElapsedTime) + " ";
    if (currentTimeStr != lastTimeStr) {
      lcd.setCursor(0, 1);
      lcd.print(currentTimeStr);
      lastTimeStr = currentTimeStr;
    }
    // Also show the "running lap" on line 0 if isRunning
    if (isRunning) {
      int lapIndex = lapCount % maxLaps;
      unsigned long currentLapTime = totalElapsedTime - laps[lapIndex].time;
      lcd.setCursor(0, 0);
      lcd.print("Lap" + String(laps[lapIndex].number) + ": "
                + formatTime(currentLapTime) + "   ");
    }
  }
}

// ------------------- TIMER MODE LOGIC -------------------
//
// handleTimerLogic(currentMillis):
//   If inTimerMode is TRUE, we do a simple countdown in mm:ss
//   The user can press Start/Stop to start/pause countdown
//   Lap button increments seconds +10
//   Once it hits 00:00 => beep 5s & blink "TIME's UP!"
//
void handleTimerLogic(unsigned long currentMillis) {
  // If "timeIsUpBlinking" is active, skip normal countdown logic
  if (timeIsUpBlinking) {
    return;
  }

  // 1) Start/Stop => toggles 'timerRunning'
  if (debouncerStartStop.fell()) {
    timerRunning = !timerRunning;
    Serial.println(timerRunning ? "Timer => Started" : "Timer => Paused");
  }

  // 2) Lap => +10 seconds
  if (debouncerLap.fell()) {
    timerSeconds += 10;
    if (timerSeconds >= 60) {
      timerMinutes += timerSeconds / 60;
      timerSeconds = timerSeconds % 60;
    }
    Serial.print("Timer => +10s => ");
    Serial.print(timerMinutes);
    Serial.print(":");
    Serial.println(timerSeconds);
  }

  // 3) Decrement each second if running
  if (timerRunning) {
    if (currentMillis - lastTimerDecrement >= 1000) {
      lastTimerDecrement = currentMillis;

      // Decrement logic
      if (timerSeconds > 0) {
        timerSeconds--;
      } else {
        if (timerMinutes > 0) {
          timerMinutes--;
          timerSeconds = 59;
        } else {
          // Timer done => beep + blink "TIME's UP!"
          timerRunning = false;
          Serial.println("Timer => 00:00 => times up!");
          timeIsUpBlinking = true;
          timeIsUpBlinkStart = currentMillis;
          buzzerActive = true;
          buzzerStartTime = currentMillis;

          lcd.clear();
        }
      }
    }
  }

  // 4) If not time up => display Timer
  if (!timeIsUpBlinking) {
    String timerStr = "T: " + formatTimer(timerMinutes, timerSeconds);
    lcd.setCursor(0, 0);
    lcd.print(timerStr + "    ");

    lcd.setCursor(0, 1);
    if (timerRunning) {
      lcd.print("COUNTING...      ");
    } else {
      lcd.print("Press Lap(+10s)  ");
    }
  }
}

// ------------------- RESET BUTTON LOGIC -------------------
//
// handleResetButtonLogic(currentMillis):
//  Distinguish between a short press (<1s) => normal reset
//  or a long press (>=1s) => toggle Timer Mode
//
void handleResetButtonLogic(unsigned long currentMillis) {
  // Depending on your wiring, you might do "rose => press started" 
  // or the other way around. Adjust as needed.
  if (debouncerReset.rose()) {
    resetPressStartTime = currentMillis;
    resetPressInProgress = true;
    Serial.println("Reset => Press started (rose).");
  }

  if (debouncerReset.fell() && resetPressInProgress) {
    resetPressInProgress = false;
    unsigned long pressDuration = currentMillis - resetPressStartTime;
    Serial.print("Reset => Press ended (fell). Duration(ms): ");
    Serial.println(pressDuration);

    // If pressDuration >= 1s => toggle Timer/Stopwatch
    if (pressDuration >= RESET_LONG_PRESS_THRESHOLD) {
      inTimerMode = !inTimerMode;
      Serial.print("LONG press => Toggling mode => ");
      Serial.println(inTimerMode ? "Timer" : "Stopwatch");

      lcd.clear();
      if (inTimerMode) {
        // Enter Timer mode
        lcd.setCursor(0, 0);
        lcd.print("Timer Ready");
        Serial.println(" -> 'Timer Ready' for 2s...");
        delay(2000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Press Start");
        Serial.println(" -> 'Press Start' displayed (Timer).");
      } else {
        // Return to Stopwatch mode
        lcd.setCursor(0, 0);
        lcd.print("Press Start      ");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00:00.0 ");
        lastTimeStr = "Time: 0:00:00.0 ";
        Serial.println(" -> Returning to Stopwatch. 'Press Start' displayed.");
      }
    }
    else {
      // Short press => reset logic
      Serial.println("SHORT press => normal reset logic if in Stopwatch, else Timer reset.");
      if (!inTimerMode) {
        // Full reset if in stopwatch mode
        isRunning = false;
        totalElapsedTime = 0;
        lapStartTime = 0;
        previousLapTime = 0;
        previousLapNumber = 0;
        lapCount = 0;

        for (int i = 0; i < maxLaps; i++) {
          laps[i].number = 0;
          laps[i].time = 0;
        }

        isBlinkingLapTime = false;
        blinkState = false;
        isResetting = true;
        resetStartTime = currentMillis;

        lcd.setCursor(0, 0);
        lcd.print("Time reseted     ");
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
      else {
        // If short-press in Timer mode => reset the countdown
        timerMinutes = 0;
        timerSeconds = 0;
        timerRunning = false;
        Serial.println("SHORT press in Timer => reset countdown to 00:00");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timer: 00:00");
        lcd.setCursor(0, 1);
        lcd.print("Reset Timer     ");
      }
    }
  }

  // If we are in a "reset display" phase, handle it
  if (isResetting) {
    if (currentMillis - resetStartTime >= resetDisplayDuration) {
      isResetting = false;
      lcd.setCursor(0, 0);
      lcd.print("Press Start      ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:00:00.0 ");
      lastTimeStr = "Time: 0:00:00.0 ";
      Serial.println("Stopwatch reset done => 'Press Start'.");
    }
  }
}

// ------------------- HELPER FUNCTIONS -------------------
//
// Increments lapCount, sets up the new-lap reference time
// so that we measure the difference for the next stop/lap
//
void prepareNewLap() {
  lapCount++;
  int lapIndex = lapCount % maxLaps;
  laps[lapIndex].number = lapCount % maxLaps;
  laps[lapIndex].time = totalElapsedTime;
}

//
// startNewLap():
//  Called either for the very first Lap0 or after blinking an old Lap time.
//  Displays "LapX: 0.00..." on line 0, sets laps[lapIndex] to current totalElapsedTime
//
void startNewLap() {
  int lapIndex = lapCount % maxLaps;
  laps[lapIndex].number = lapCount % maxLaps;
  laps[lapIndex].time   = totalElapsedTime;

  lcd.setCursor(0, 0);
  lcd.print("Lap" + String(laps[lapIndex].number)
            + ": " + formatTime(0) + "   ");

  lapStartTime = millis();
  Serial.print("startNewLap => Lap ");
  Serial.println(laps[lapIndex].number);
}

//
// If the buzzer is active, beep for 5s (timeIsUpBlinkDuration).
// Then turn it off automatically.
//
void handleBuzzer(unsigned long currentMillis) {
  if (buzzerActive) {
    // Continuous beep
    digitalWrite(buzzerPin, HIGH);
    if ((currentMillis - buzzerStartTime) >= timeIsUpBlinkDuration) {
      buzzerActive = false;
      digitalWrite(buzzerPin, LOW);
      Serial.println("Buzzer OFF => 5s beep done.");
    }
  }
}