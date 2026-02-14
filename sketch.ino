#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

LiquidCrystal_I2C lcd(0x27,16,2);
int Vpin = A0;
float totalResistance;
float arduinoResistance;
float arduinoAnlogVoltage;
float arduinoVoltage;
float trueVoltage;

// Напряжения для разных режимов
const float voltageLevels[] = {150.0, 200.0, 250.0, 300.0};
const int numVoltageLevels = 4;
int currentVoltageIndex = 0;
float maxVoltage = voltageLevels[currentVoltageIndex];

const size_t relayIn = 7;
const size_t START_BUTTON = 8;
const size_t CHARGE_BUTTON = 9;

int ledRedPin = 3; 
int ledYellowPin = 4; 
int ledGreenPin = 5; 

enum SystemState {
  VOLTAGE_SELECT, // Выбор напряжения
  IDLE,           // Ожидание зарядки
  CHARGING,       // Зарядка
  CHARGED,        // Заряжено
  FIRING,        // Выстрел
  READY          // Готов к новой зарядке
};

SystemState currentState = VOLTAGE_SELECT;
bool lastChargeButtonState = HIGH;
bool lastStartButtonState = HIGH;

const int transistorPin = 6;

unsigned long firingStartTime = 0;
const unsigned long FIRING_DURATION = 100; // Длительность импульса выстрела (мс)
const unsigned long READY_DELAY = 1000;    // Задержка перед готовностью (мс)

// Переменные для мигания текста в состоянии READY
unsigned long lastDisplayChangeTime = 0;
const unsigned long DISPLAY_CHANGE_INTERVAL = 1000; // Интервал смены текста (мс)
bool showReadyText = true; // Флаг для определения, что показывать

// Флаг для предотвращения многократного срабатывания при долгом нажатии
bool fireButtonPressedInIdle = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Program started");
  
  lcd.init();
  lcd.backlight();
  
  lcd.print("Select Voltage:");
  lcd.setCursor(0, 1);
  lcd.print("Max: ");
  lcd.print(maxVoltage);
  lcd.print("V     ");

  pinMode(transistorPin, OUTPUT);
  digitalWrite(transistorPin, LOW);
  
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);

  pinMode(relayIn, OUTPUT);
  digitalWrite(relayIn, LOW);
  
  pinMode(CHARGE_BUTTON, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  
  lastDisplayChangeTime = millis();
}

void loop() {
  bool chargeButtonState = digitalRead(CHARGE_BUTTON);
  bool startButtonState = digitalRead(START_BUTTON);
  
  updateVoltage();
  
  controlLEDs();
  
  switch(currentState) {
    case VOLTAGE_SELECT:
      handleVoltageSelectState(startButtonState, chargeButtonState);
      break;
      
    case IDLE:
      handleIdleState(chargeButtonState, startButtonState);
      break;
      
    case CHARGING:
      handleChargingState(chargeButtonState);
      break;
      
    case CHARGED:
      handleChargedState(startButtonState);
      break;
      
    case FIRING:
      handleFiringState();
      break;
      
    case READY:
      handleReadyState();
      break;
  }
  
  // Сохраняем состояния кнопок для следующего цикла
  lastChargeButtonState = chargeButtonState;
  lastStartButtonState = startButtonState;
  
  delay(50);
}

void updateVoltage() {
  totalResistance = 1009750;
  arduinoResistance = 9750;
  arduinoAnlogVoltage = analogRead(Vpin); 
  arduinoVoltage = arduinoAnlogVoltage / 1023 * 5.0;
  trueVoltage = arduinoVoltage * totalResistance / arduinoResistance;
  
  // Обновление дисплея (верхняя строка)
  if (currentState != VOLTAGE_SELECT) {
    lcd.setCursor(0, 0);
    lcd.print("Voltage: ");
    lcd.setCursor(9, 0);
    lcd.print(trueVoltage, 1);
    lcd.print("V   ");
  }
}

void controlLEDs() {
  // В состоянии выбора напряжения все светодиоды выключены
  if (currentState == VOLTAGE_SELECT) {
    digitalWrite(ledRedPin, LOW);
    digitalWrite(ledYellowPin, LOW);
    digitalWrite(ledGreenPin, LOW);
    return;
  }
  
  // Красный светодиод - есть любое напряжение
  if (trueVoltage > 0.1) {
    digitalWrite(ledRedPin, HIGH);
  } else {
    digitalWrite(ledRedPin, LOW);
  }
  
  // Желтый светодиод - напряжение > 50% от максимального
  if (trueVoltage > 0.5 * maxVoltage) {
    digitalWrite(ledYellowPin, HIGH);
  } else {
    digitalWrite(ledYellowPin, LOW);
  }
  
  // Зеленый светодиод - напряжение > 90% от максимального
  if (trueVoltage > 0.9 * maxVoltage) {
    digitalWrite(ledGreenPin, HIGH);
  } else {
    digitalWrite(ledGreenPin, LOW);
  }
}

void updateReadyDisplay() {
  // Проверяем, нужно ли обновить дисплей
  if (millis() - lastDisplayChangeTime >= DISPLAY_CHANGE_INTERVAL) {
    lastDisplayChangeTime = millis();
    showReadyText = !showReadyText; // Переключаем флаг
    
    lcd.setCursor(0, 1);
    lcd.print("                "); // Очищаем строку
    
    if (showReadyText) {
      lcd.setCursor(0, 1);
      lcd.print("READY");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Max: ");
      lcd.print(maxVoltage);
      lcd.print("V");
    }
  }
}

void handleVoltageSelectState(bool startButtonState, bool chargeButtonState) {

  if (startButtonState == LOW && lastStartButtonState == HIGH) {
    // Переключаем на следующий уровень напряжения
    currentVoltageIndex = (currentVoltageIndex + 1) % numVoltageLevels;
    maxVoltage = voltageLevels[currentVoltageIndex];
    
    Serial.print("Voltage level changed to: ");
    Serial.println(maxVoltage);
    
    // Обновляем дисплей
    lcd.setCursor(0, 1);
    lcd.print("Max: ");
    lcd.print(maxVoltage);
    lcd.print("V     ");
    
    // Мигаем зеленым светодиодом для индикации изменения
    digitalWrite(ledGreenPin, HIGH);
    delay(100);
    digitalWrite(ledGreenPin, LOW);
  }
  
  // Нажатие кнопки зарядки подтверждает выбор и переходит к режиму IDLE
  if (chargeButtonState == LOW && lastChargeButtonState == HIGH) {
    currentState = IDLE;
    fireButtonPressedInIdle = false; // Сбрасываем флаг
    Serial.println("Voltage selected, going to IDLE mode");
    
    // Обновляем дисплей для состояния IDLE
    lcd.clear();
    lcd.print("Voltage: ");
    lcd.setCursor(9, 0);
    lcd.print("0.0V   ");
    updateReadyDisplay();
  }
}

void handleIdleState(bool chargeButtonState, bool startButtonState) {
  // В состоянии IDLE показываем чередующийся текст
  updateReadyDisplay();
  
  // Нажатие кнопки выстрела в состоянии IDLE - переход к выбору напряжения
  if (startButtonState == LOW && lastStartButtonState == HIGH) {
    if (!fireButtonPressedInIdle) {
      fireButtonPressedInIdle = true;
      
      // Переходим к выбору напряжения
      currentState = VOLTAGE_SELECT;
      Serial.println("Going to voltage select mode from IDLE");
      
      // Обновляем дисплей
      lcd.clear();
      lcd.print("Select Voltage:");
      lcd.setCursor(0, 1);
      lcd.print("Max: ");
      lcd.print(maxVoltage);
      lcd.print("V     ");
      
      return; // Выходим, чтобы не обрабатывать нажатие кнопки зарядки
    }
  }
  
  // Сброс флага, когда кнопка отпущена
  if (startButtonState == HIGH) {
    fireButtonPressedInIdle = false;
  }
  
  // Нажатие кнопки зарядки (только если кнопка выстрела не была нажата)
  if (chargeButtonState == LOW && lastChargeButtonState == HIGH && !fireButtonPressedInIdle) {
    digitalWrite(relayIn, HIGH); // Включаем реле зарядки
    currentState = CHARGING;
    Serial.println("Charging started");
    
    lcd.setCursor(0, 1);
    lcd.print("Charging...  ");
  }
}

void handleChargingState(bool chargeButtonState) {
  // Отпускание кнопки зарядки
  if (chargeButtonState == HIGH && lastChargeButtonState == LOW) {
    digitalWrite(relayIn, LOW); // Выключаем реле зарядки
    currentState = CHARGED;
    Serial.println("Charging stopped");
    
    lcd.setCursor(0, 1);
    lcd.print("Charged      ");
  }
  
  // Автоматический переход при полной зарядке
  if (trueVoltage > 0.95 * maxVoltage) {
    digitalWrite(relayIn, LOW);
    currentState = CHARGED;
    Serial.println("Auto: Charging complete");
    
    lcd.setCursor(0, 1);
    lcd.print("Charged      ");
  }
}

void handleChargedState(bool startButtonState) {
  lcd.setCursor(0, 1);
  lcd.print("Press FIRE   ");
  
  // Нажатие кнопки выстрела
  if (startButtonState == LOW && lastStartButtonState == HIGH) {
    currentState = FIRING;
    firingStartTime = millis();
    
    // Активация выстрела
    digitalWrite(transistorPin, HIGH);
    Serial.println("Firing!");
    
    lcd.setCursor(0, 1);
    lcd.print("FIRING!      ");
  }
}

void handleFiringState() {
  // Проверяем, прошло ли время выстрела
  if (millis() - firingStartTime >= FIRING_DURATION) {
    digitalWrite(transistorPin, LOW); // Выключаем транзистор
    Serial.println("Firing complete");
    
    currentState = READY;
    firingStartTime = millis(); // Сбрасываем таймер для состояния READY
    
    lcd.setCursor(0, 1);
    lcd.print("Discharging  ");
  }
}

void handleReadyState() {
  // В состоянии READY показываем "Discharging"
  lcd.setCursor(0, 1);
  lcd.print("Discharging  ");
  
  // Ждем указанное время перед переходом в IDLE
  if (millis() - firingStartTime >= READY_DELAY) {
    // Сбрасываем все флаги
    digitalWrite(relayIn, LOW);
    digitalWrite(transistorPin, LOW);
    
    // Сбрасываем таймер для дисплея
    lastDisplayChangeTime = millis();
    showReadyText = true;
    
    currentState = IDLE;
    fireButtonPressedInIdle = false; // Сбрасываем флаг
    Serial.println("System ready for new charge");
  }
}
