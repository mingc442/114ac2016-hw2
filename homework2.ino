
/*
  Mood Buddy 心情娃娃

  Arduino Nano + RGB LED + Button

  RGB 共陽極：
  U -> 5V
  R -> D9
  G -> D10
  B -> D11

  Button：
  IN  -> D2
  VCC -> 5V
  GND -> GND

  遊戲：
  1. 一般模式從10分開始
  2. 每按一次 +1分
  3. 5秒沒按，每秒 -1分
  4. 20分變成藍色
  5. 藍色後，3秒內連按5次
  6. 觸發2秒彩虹獎勵
  7. 獎勵結束後重新開始

  額外功能：
  首次到15分時，紫粉色閃3次
*/

// ===================================
// 1. 腳位
// ===================================

const int BUTTON_PIN = 2;

const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

// 按鈕按下輸出LOW
// 如果你的模組按下輸出HIGH，改成HIGH
const int PRESSED_STATE = LOW;


// ===================================
// 2. 遊戲模式
// ===================================

enum GameMode {
  NORMAL,       // 一般模式
  COMBO_READY,  // 藍燈，等待連擊
  REWARD        // 獎勵模式
};

GameMode mode = NORMAL;

int mood = 10;

const int MAX_MOOD = 20;
const int MIN_MOOD = 0;


// ===================================
// 3. 成就記憶
// ===================================

bool memoryUnlocked = false;
bool memoryEffectActive = false;

unsigned long memoryStartTime = 0;


// ===================================
// 4. 連擊設定
// ===================================

int comboCount = 0;

unsigned long comboStartTime = 0;
unsigned long rewardStartTime = 0;

const unsigned long COMBO_WINDOW = 3000;
const unsigned long REWARD_TIME = 2000;


// ===================================
// 5. 時間設定
// ===================================

const unsigned long WAIT_TIME = 5000;
const unsigned long LOSE_INTERVAL = 1000;
const unsigned long DEBOUNCE_TIME = 40;

const unsigned long MEMORY_TIME = 900;

unsigned long lastPressTime = 0;
unsigned long lastLoseTime = 0;

unsigned long lastDebounceTime = 0;

int lastReading;
int stableButtonState;


// ===================================
// 6. RGB 控制（共陽極）
// ===================================

void setColor(int r, int g, int b) {

  r = constrain(r, 0, 255);
  g = constrain(g, 0, 255);
  b = constrain(b, 0, 255);

  // 共陽極：反轉PWM
  analogWrite(RED_PIN, 255 - r);
  analogWrite(GREEN_PIN, 255 - g);
  analogWrite(BLUE_PIN, 255 - b);
}


// ===================================
// 7. 一般模式顏色
// ===================================

void showMoodColor() {

  int r = 0;
  int g = 0;
  int b = 0;

  if (mood <= 10) {

    // 紅 -> 綠
    r = map(mood, 0, 10, 255, 0);
    g = map(mood, 0, 10, 0, 255);

  } else {

    // 綠 -> 藍
    g = map(mood, 10, 20, 255, 0);
    b = map(mood, 10, 20, 0, 255);
  }

  setColor(r, g, b);
}


// ===================================
// 8. 心情記憶燈
// ===================================

void showMemoryEffect(unsigned long now) {

  unsigned long elapsed = now - memoryStartTime;

  if ((elapsed / 150) % 2 == 0) {
    setColor(255, 0, 180);
  } else {
    setColor(0, 0, 0);
  }
}


// ===================================
// 9. 彩虹獎勵燈
// ===================================

void showReward(unsigned long now) {

  unsigned long elapsed = now - rewardStartTime;

  int phase = (elapsed / 100) % 6;

  switch (phase) {

    case 0:
      setColor(255, 0, 0);
      break;

    case 1:
      setColor(255, 120, 0);
      break;

    case 2:
      setColor(255, 255, 0);
      break;

    case 3:
      setColor(0, 255, 0);
      break;

    case 4:
      setColor(0, 100, 255);
      break;

    case 5:
      setColor(180, 0, 255);
      break;
  }
}


// ===================================
// 10. 重新開始
// ===================================

void resetGame(unsigned long now) {

  mood = 10;
  mode = NORMAL;

  comboCount = 0;
  memoryEffectActive = false;

  lastPressTime = now;
  lastLoseTime = now;

  Serial.println("NEW GAME!");
  Serial.println("NORMAL MODE");
  Serial.println("Mood = 10");
}


// ===================================
// 11. 初始化
// ===================================

void setup() {

  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, HIGH);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  lastReading = digitalRead(BUTTON_PIN);
  stableButtonState = lastReading;

  lastPressTime = millis();
  lastLoseTime = millis();

  Serial.println("Mood Buddy Start!");
  Serial.println("NORMAL MODE");
  Serial.println("Mood = 10");

  showMoodColor();
}


// ===================================
// 12. 主程式
// ===================================

void loop() {

  unsigned long now = millis();

  // -----------------------------------
  // A. 按鈕防彈跳
  // -----------------------------------

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = now;
  }

  bool pressedEvent = false;

  if (now - lastDebounceTime >= DEBOUNCE_TIME) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      // 只在按下瞬間觸發
      if (stableButtonState == PRESSED_STATE) {
        pressedEvent = true;
      }
    }
  }

  lastReading = reading;


  // -----------------------------------
  // B. 獎勵模式
  // -----------------------------------

  if (mode == REWARD) {

    if (now - rewardStartTime >= REWARD_TIME) {

      resetGame(now);
      showMoodColor();

    } else {

      showReward(now);
    }

    return;
  }


  // -----------------------------------
  // C. 一般模式：按鈕加分
  // -----------------------------------

  if (pressedEvent) {

    lastPressTime = now;
    lastLoseTime = now;

    if (mode == NORMAL) {

      if (mood < MAX_MOOD) {
        mood++;
      }

      Serial.print("Mood = ");
      Serial.println(mood);

      // 首次15分，解鎖記憶
      if (mood >= 15 && !memoryUnlocked) {

        memoryUnlocked = true;
        memoryEffectActive = true;
        memoryStartTime = now;

        Serial.println("MEMORY UNLOCKED!");
      }

      // 20分，進入連擊準備模式
      if (mood == 20) {

        mode = COMBO_READY;

        comboCount = 0;

        Serial.println("BLUE!");
        Serial.println("COMBO READY!");
        Serial.println("Press 5 times in 3 seconds");
      }

    }

    // ---------------------------------
    // D. 藍燈後開始計算連擊
    // ---------------------------------

    else if (mode == COMBO_READY) {

      // 第一次按下，開始3秒計時
      if (comboCount == 0 ||
          now - comboStartTime > COMBO_WINDOW) {

        comboCount = 1;
        comboStartTime = now;

      } else {

        comboCount++;
      }

      Serial.print("Combo = ");
      Serial.println(comboCount);

      // 3秒內按滿5次
      if (comboCount >= 5) {

        mode = REWARD;

        rewardStartTime = now;

        memoryEffectActive = false;

        Serial.println("REWARD MODE!");

        showReward(now);
        return;
      }
    }
  }


  // -----------------------------------
  // E. 5秒沒按，心情下降
  // -----------------------------------

  if (now - lastPressTime >= WAIT_TIME &&
      now - lastLoseTime >= LOSE_INTERVAL) {

    if (mood > MIN_MOOD) {

      mood--;

      Serial.print("Mood decreased = ");
      Serial.println(mood);
    }

    lastLoseTime = now;

    // 如果分數掉到20以下，
    // 取消連擊準備，回一般模式
    if (mode == COMBO_READY && mood < 20) {

      mode = NORMAL;
      comboCount = 0;

      Serial.println("BACK TO NORMAL");
    }
  }


  // -----------------------------------
  // F. 記憶燈時間結束
  // -----------------------------------

  if (memoryEffectActive &&
      now - memoryStartTime >= MEMORY_TIME) {

    memoryEffectActive = false;
  }


  // -----------------------------------
  // G. 顯示燈光
  // -----------------------------------

  if (memoryEffectActive) {

    showMemoryEffect(now);

  } else {

    showMoodColor();
  }
}
