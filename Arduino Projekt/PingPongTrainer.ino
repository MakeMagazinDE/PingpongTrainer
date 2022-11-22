/*
    PING PONG TRAINER
    Software: Version E1.0 - 24.9.2022
    Hardware: Revision E  - 18.9.2022
    YouTube: https://www.youtube.com/channel/UCTQz_JDB3c2g9OMSX82BWHg











    ******************************************************************************
    MIT License

    Copyright (c) 2022 Benno Lottenbach, BINGOBRICKS Switzerland

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    ******************************************************************************
*/
#include <Servo.h>
#include <Toggle.h>

//Servo deklarieren
#define cServoPin 5
#define cServoPosLeft 160           //Servoposition für Schussrichtung links
#define cServoPosCenter 90          //Servoposition für Schussrichtung mitte
#define cServoPosRight 20           //Servoposition für Schussrichtung rechts
int vServoPos;                      
byte vMoveServo = false;
Servo myServo;

// Motor B1 (Backspin) deklarieren
#define cMB1Pin 3                       // ENA -> PWM notwendig  
#define cMB1In1Pin 2                    // INB / IN2
#define cMB1In2Pin 4                    // INA / IN1
#define cMB1Throttle 0.50               // Faktor, mit welchem die Leistung des Motors gedrosselt wird. Beispiel: Bei 24V Versorgungsspannung und cMB1Throttle = 0.5 -> Am Motor liegen maximal 12V an. Falls die Schussweite trotz maximal offenem Drehregler nicht ausreicht, kann dieser Wert bis maximal auf 1 erhöht werden.
bool vMB1Invert = false;                // Auf true setzen, wenn die Drehrichtung des Motors umgekehrt werden soll
byte vMB1State = LOW;                   // Motor startet, sobald diese Variable auf HIGH ist

// Motor A1 (Topspin) deklarieren
#define cMA1Pin 11                      // ENA -> PWM notwendig  
#define cMA1In1Pin 10                   // INB / IN2
#define cMA1In2Pin 9                    // INA / IN1
#define cMA1Throttle 0.50               // Faktor, mit welchem die Leistung des Motors gedrosselt wird. Beispiel: Bei 24V Versorgungsspannung und cMB1Throttle = 0.5 -> Am Motor liegen maximal 12V an. Falls die Schussweite trotz maximal offenem Drehregler nicht ausreicht, kann dieser Wert bis maximal auf 1 erhöht werden.
bool vMA1Invert = false;                // Auf true setzen, wenn die Drehrichtung des Motors umgekehrt werden soll
byte vMA1State = LOW;                   // Motor startet, sobald diese Variable auf HIGH ist

// Motor A2 (Ballzuführung) deklarieren
#define cMA2Pin 6                       // ENB -> PWM notwendig        
#define cMA2In3Pin 7                    // INC / IN3
#define cMA2In4Pin 8                    // IND / IN4
#define cMA2Throttle 0.50               // Faktor, mit welcher die Leistung von Motor A2 (Ballzuführung) gedrosselt wird. Beispiel: 12V Motor an 24V Versorgungsspannung -> cMA2Throttle = 0.5. Achtung: Ein zu hoher Wert könnte den Motor zerstören!
bool vMA2Invert = false;                // Auf true setzen, wenn die Drehrichtung des Motors umgekehrt werden soll
byte vMA2State = LOW;                   // Motor startet, sobald diese Variable auf HIGH ist
unsigned long vLastMotorstart = 0;      //Enthält den Timestamp, an welchem letztmals die Ballzufuhr gestartet wurde.

//Schalter und Buttons deklarieren
#define cSwitchLeftPin A2
#define cSwitchCenterPin A3
#define cSwitchRightPin A4
#define cSwitchLimitPin A0
#define cButtonStartPin A1
Toggle buttonStart(cButtonStartPin);
Toggle limitSwitch(cSwitchLimitPin);
unsigned long vLastShot = 0;          //Enthält den Timestamp, an welchem letztmals der Limitsensor ausgelöst wurde

//LED deklarieren
#define cLedPin 12
#define cLedDuration 300              //Leuchtdauer der LED für 1x blinken
#define cLedOffset 500                //Offset für das Blinkintervall der LED, weil die benötigte Zeit für den Ballvorschub miteingerechnet werden sollte.
byte vLedState = LOW;
unsigned long vLastBlink = 0;

//Potentiometer deklarieren
#define cPotIntervallPin A5
#define cPotDistancePin A6
#define cPotSpinPin A7
#define cPowerCenterSpin 0.7          //Bei Nullspin reduziert sich die Leistung des Motors auf diesen Prozentwert (bei Fullspin wird auf 100% erhöht)
int vPotDistance = 0;
int vPotIntervall = 0;
float vPotSpin = 0.0;

//Weitere Variablen und Konstanten
#define cStartDelay 5000              //Zeit in ms vom Drücken des Startbuttons bis zur ersten Schussabgabe vergehen soll. (Damit sich der Spieler in Position bringen kann.)
#define cStopDelay 6000               //Zeitlimite in ms, die zwischen zwei Schussabgaben maximal verstreichen darf, bis die Ballzuführung gestoppt wird. 6000 ms entsprechen ca. 1 Umdrehung.
unsigned long vTimeStamp = 0;
byte vState = 0;                      //0 = Standby, 1 = Starting, 2 = Running


void setup() {
  //Serieller Monitor initialisieren
  //Serial.begin(9600);

  //Schalter und Buttons initialisieren
  pinMode(cSwitchLeftPin, INPUT_PULLUP);
  pinMode(cSwitchCenterPin, INPUT_PULLUP);
  pinMode(cSwitchRightPin, INPUT_PULLUP);
  pinMode(cSwitchLimitPin, INPUT_PULLUP);
  pinMode(cButtonStartPin, INPUT_PULLUP);
  buttonStart.begin(cButtonStartPin);
  buttonStart.setToggleState(0);    // set initial state 0 or 1
  buttonStart.setToggleTrigger(0);  // set trigger onPress: 0, or onRelease: 1
  limitSwitch.begin(cSwitchLimitPin);

  //Motoren initialisieren
  pinMode(cMB1Pin, OUTPUT);
  pinMode(cMA1Pin, OUTPUT);
  pinMode(cMA2Pin, OUTPUT);
  pinMode(cMB1In1Pin, OUTPUT);
  pinMode(cMB1In2Pin, OUTPUT);
  pinMode(cMA1In1Pin, OUTPUT);
  pinMode(cMA1In2Pin, OUTPUT);
  pinMode(cMA2In3Pin, OUTPUT);
  pinMode(cMA2In4Pin, OUTPUT);
  digitalWrite(cMB1In1Pin, LOW);
  digitalWrite(cMB1In2Pin, LOW);
  digitalWrite(cMA1In1Pin, LOW);
  digitalWrite(cMA1In2Pin, LOW);
  digitalWrite(cMA2In3Pin, LOW);
  digitalWrite(cMA2In4Pin, LOW);
  analogWrite(cMA2Pin, 255 * cMA2Throttle);
  myServo.attach(cServoPin);

  //Drehregler initialisieren
  pinMode(cPotDistancePin, INPUT);
  pinMode(cPotIntervallPin, INPUT);
  pinMode(cPotSpinPin, INPUT);

  //LED initialisieren
  pinMode(cLedPin, OUTPUT);
  digitalWrite(cLedPin, vLedState);
  pinMode(LED_BUILTIN, OUTPUT);
  
  //Abschluss der Setup-Routine:
  //Servo in allen drei Richtungen positionieren und in der Mitte "parken"
  myServo.write(cServoPosLeft);
  delay(500);
  myServo.write(cServoPosRight);
  delay(500);
  myServo.write(cServoPosCenter);
  //Interne LED 3x kurz blinken lassen
  for (byte i = 0; i < 3; i++) {
    delay(300);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void loop() {
  //Serielle Ausgaben zum Debuggen
  //Serial.print(" - StartButton: " + String(digitalRead(cButtonStartPin)));
  //Serial.print(" - ButtonToggle: " + String(buttonStart.toggle()));
  //Serial.print(" - Status: " + String(vState));
  //Serial.print(" - Intervall: " + String(vPotIntervall));
  //Serial.print(" - Distanz: " + String(vPotDistance));
  //Serial.print(" - Spin: " + String(vPotSpin));
  //Serial.print(" - MA1Power: " + String(fGetMPower('A')));
  //Serial.print(" - MB1Power: " + String(fGetMPower('B')));
  //Serial.print(" - LimitPin: " + String(digitalRead(cSwitchLimitPin)));
  //Serial.print(" - LimitOnPress: " + String(limitSwitch.onPress()));
  //Serial.print(" - Ballzufuhr MA2State: " + String(vMA2State));
  //Serial.print(" - Schusss M1State: " + String(vMB1State));
  //Serial.print(" - Servoflag: " + String(vMoveServo));
  //Serial.print(" - Nächste Pos: " + String(vServoPos));
  //Serial.print(" - Links: " + String(digitalRead(cSwitchLeftPin)));
  //Serial.print(" - Mitte: " + String(digitalRead(cSwitchCenterPin)));
  //Serial.print(" - Rechts: " + String(digitalRead(cSwitchRightPin)));
  //Serial.println("");


  fGetPotValues();
  fLedBlink();
  fMotorControl();
  fSetState();
  fState0Standby();
  fState1Ready();
  fState2Running();
}
