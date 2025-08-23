#include <Wire.h> // библиотека для управления устройствами по I2C 
#include <LiquidCrystal_I2C.h> // подключаем библиотеку для LCD 1602
#include <math.h>

LiquidCrystal_I2C lcd(0x27,16,2); // присваиваем имя lcd для дисплея 16х2
int Vpin = A0; // «+» подсоединяем к аналоговому выводу А0
float totalResistance;
float arduinoResistance;
float arduinoAnlogVoltage;
float arduinoVoltage;
float trueVoltage;
float maxVoltage = 305;

const size_t relayIn = 7;

const size_t START_BUTTON = 8;
const size_t CHARGE_BUTTON = 9;

int ledRedPin = 3; 
int ledYellowPin = 4; 
int ledGreenPin = 5; 

bool chargeButtonWasPressed = false;
bool startButtonWasPressed = false;

const int transistorPin = 6; // Пин для управления транзистором


void setup() // процедура setup
{

  
  Serial.begin(9600);
  lcd.init(); // инициализация LCD дисплея
  lcd.backlight(); // включение подсветки дисплея
  lcd.print("vol = ");

  pinMode(transistorPin, OUTPUT); // Настраиваем пин как выход
  digitalWrite(transistorPin, LOW);
  
  // delay(1000);
  // digitalWrite(transistorPin, LOW);

  pinMode(ledRedPin, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);

  pinMode(relayIn, OUTPUT);
  pinMode(CHARGE_BUTTON, INPUT);
}
 
void loop() // процедура loop
{
  // 1 - отпущена, 0 - нажата
  bool chargeState = digitalRead(CHARGE_BUTTON);
  bool startState = digitalRead(START_BUTTON);

  if (!chargeState || chargeButtonWasPressed)
  {
    chargeButtonWasPressed = true;
    digitalWrite(relayIn, HIGH);
  }

  if (!startState || startButtonWasPressed)
  {
    startButtonWasPressed = true;
    digitalWrite(relayIn, LOW);
    delay(2000);
    digitalWrite(transistorPin, HIGH);
  }
  
    
  

  totalResistance = 1009750;
  arduinoResistance = 9750;
  arduinoAnlogVoltage = analogRead(Vpin); 
  // получаем измеряемое напряжение
  arduinoVoltage = arduinoAnlogVoltage/1023*5.0;
  trueVoltage = arduinoVoltage*totalResistance/arduinoResistance;

  if (abs(trueVoltage) > 0.001)
    digitalWrite(ledRedPin, HIGH);
  else
    digitalWrite(ledRedPin, LOW);

  if (trueVoltage - 0.5 * maxVoltage > 0.001)
    digitalWrite(ledYellowPin, HIGH);
  else
    digitalWrite(ledYellowPin, LOW);

  if (trueVoltage - 0.9 * maxVoltage > 0.001)
    digitalWrite(ledGreenPin, HIGH);
  else
    digitalWrite(ledGreenPin, LOW);

  lcd.setCursor(6, 0);
  lcd.print(trueVoltage);
  Serial.println("V");
  lcd.print("V");
  delay(300);
  lcd.print("     ");
  //lcd.clear();
}