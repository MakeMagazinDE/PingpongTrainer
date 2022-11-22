//Drehregler auslesen
void fGetPotValues() {
  int vTemp = 0;

  vPotDistance = map(analogRead(cPotDistancePin), 0, 1023, 255, 0);         //Reglerwert von 0-1023 wird auf den Bereich 255-0 umgerechnet
  vPotIntervall = map(analogRead(cPotIntervallPin), 0, 1023, 0, 4000);    //Reglerwert von 0-1023 wird auf den Bereich 0-4000 [ms] umgerechnet
  //vPotSpin = map(analogRead(cPotSpinPin), 0, 1023, 100, -100);                 //Reglerwert von 0-1023 wird auf den Bereich -100% bis +100% umgerechnet
  vTemp = map(analogRead(cPotSpinPin), 0, 1023, 100, -100);                 //Reglerwert von 0-1023 wird auf den Bereich -100 bis +100 umgerechnet
  vPotSpin = vTemp / 100.0;                                               //Die Dezimalstelle hinter dem Komma ist wichtig für die Float-Berechnung!
}

//LED im am Drehregler eingestellten Intervall blinken lassen
void fLedBlink() {
  if (vState != 0) {  //Im Status 1 oder 2 die LED dauernd leuchten lassen.
    vLedState = HIGH;
  } else {  //Ansonsten die LED gemäss dem eingestellten Intervall blinken lassen.
    switch (vLedState) {
      case LOW:
        if ((millis() - vLastBlink) >= (vPotIntervall + cLedOffset)) {
          vLedState = HIGH;
          vLastBlink = millis();
        }
        break;

      case HIGH:
        if ((millis() - vLastBlink) >= cLedDuration) {
          vLedState = LOW;
          vLastBlink = millis();
        }
        break;
    }
  }
  digitalWrite(cLedPin, vLedState);
}

//Motoren anhand der gesetzten Stati und Leistungswerte starten/stoppen
void fMotorControl() {

  if (vMB1Invert) {
    digitalWrite(cMB1In1Pin, vMB1State);
    digitalWrite(cMB1In2Pin, LOW);
  } else {
    digitalWrite(cMB1In1Pin, LOW);
    digitalWrite(cMB1In2Pin, vMB1State);
  }

  if (vMA1Invert) {
    digitalWrite(cMA1In1Pin, LOW);
    digitalWrite(cMA1In2Pin, vMA1State);
  } else {
    digitalWrite(cMA1In1Pin, vMA1State);
    digitalWrite(cMA1In2Pin, LOW);
  }

  if (vMA2Invert) {
    digitalWrite(cMA2In3Pin, vMA2State);
    digitalWrite(cMA2In4Pin, LOW);
  } else {
    digitalWrite(cMA2In3Pin, LOW);
    digitalWrite(cMA2In4Pin, vMA2State);
  }

  //Motoren mit dem errechneten Leistungswert betreiben
  analogWrite(cMA1Pin, fGetMPower('A'));
  analogWrite(cMB1Pin, fGetMPower('B'));

  //Da die Leistung für den Motor 3 (Ballvorschub) immer gleich bleiben soll, wird diese in der Setup-Routine einmalig gesetzt und hier auskommentiert.
  //analogWrite(cMA2Pin, cMA2Throttle);

  //Wenn das MoveServo-Flag gesetzt ist, kann die Servoposition angefahren werden.
  if (vMoveServo) {
    myServo.write(vServoPos);
    vMoveServo = false;
  }
}

byte fGetMPower(char vMotor) {
  byte vMxMax = 0;
  byte vMxMid = 0;
  byte vMxMidToMax = 0;
  byte vMxPower = 0;
  float vPotSpinAbs = 0.0; 

  vPotSpinAbs = fabs(vPotSpin);         //abs() funktioniere nicht bei float-Datentypen. Ich konnte das allerdings nicht nachvollziehen. -> https://github.com/arduino/reference-en/issues/362
  
  vMxMax = vPotDistance;                //Maximale Leistung unter Berücksichtigung der eingestellten Schussweite
  vMxMid = vMxMax * cPowerCenterSpin;   //Leistung bei Nullspin (Mittelposition)
  vMxMidToMax = vMxMax - vMxMid;        //Leistungssteigerung von Nullspin bis Fullspin

  switch (vMotor) {

    case 'A':
      if (vPotSpin >= 0) {
        vMxPower = vMxMid + (vMxMidToMax * vPotSpinAbs);
      } else {
        vMxPower = vMxMid * (1 - vPotSpinAbs);
      }
      vMxPower = vMxPower * cMA1Throttle;
      break;

    case 'B':
      if (vPotSpin >= 0) {
        vMxPower = vMxMid * (1.0 - vPotSpinAbs);
      } else {
        vMxPower = vMxMid + (vMxMidToMax * vPotSpinAbs);
      }
      vMxPower = vMxPower * cMB1Throttle;
      break;
  }

  return vMxPower;
}


//Diese Routine entscheidet, welcher Status gesetzt werden soll.
//Status 0 - Standby - Alle Motoren ausgeschaltet. Das Gerät wartet auf den Start.
//Status 1 - Starting - Der Startknopf wurde gedrückt. Der Benutzer hat nun Zeit, sich an seine Spielposition zu begeben.
//Status 2 - Running - Der Roboter ist in Betrieb und schiesst die Bälle dem Spieler zu.
void fSetState() {

  //Status Startbutton abfragen
  buttonStart.poll();
  if (buttonStart.toggle() == 0) {
    vState = 0;
  }

  if (buttonStart.toggle() == 1) {
    switch (vState) {
      case 0:
        //Start initialisieren:
        vLastMotorstart = 0;                                          //Prüfung auf StopDelay ausschalten
        fDirection();                                                 //erste Servoposition ermitteln
        vMoveServo = true;                                            //Servoflag setzen
        fMotorControl();                                              //Servo positionieren lassen
        vTimeStamp = millis();
        vState = 1;
        break;

      case 1:                                                           //Wenn im Status 1..
        if ((millis() - vTimeStamp) >= cStartDelay) {                   //..mit Start warten, bis Wartezeit verstrichen ist..
          vState = 2;                                                   //...dann Start mit Status 2 auslösen.
        }
        break;

      case 2:                                                           //Wenn im Status 2..
        if (vLastMotorstart > 0) {                                      //..und die Prüfung auf das StopDelay aktiviert ist..
          if (millis() - vLastMotorstart >= cStopDelay) {               //..und der Ballvorschub schon über der Zeitlimite für das StopDelay läuft..
            buttonStart.setToggleState(0);                              //..dann den Stopp-Knopf (virtuell) drücken..
            vState = 0;                                                 //..und den Status auf Standby 0 setzen.
          }
        }
        break;
    }
  }
}



void fState0Standby() {
  if (vState == 0) {  //Standby
    //Alle Motoren stoppen
    vMB1State = LOW;
    vMA1State = LOW;
    vMA2State = LOW;

    //Servo in der Mitte positionieren lassen
    vServoPos = cServoPosCenter;
    vMoveServo = true;
  }
}


void fState1Ready() {
  if (vState == 1) {            //Diese Anweisungen werden mehrmals durchlaufen, solange der Status 1 ist
    vMB1State = HIGH;            //Schussmotor B1 laufen lassen
    vMA1State = HIGH;            //Schussmotor A1 laufen lassen
  }
}

void fState2Running() {
  if (vState == 2) {
    limitSwitch.poll();           //Limitswitch abfragen, um aktuellen Zustand zu speichern.
    if (limitSwitch.onPress()) {  //Wenn Limitschalter anspricht...,
      vMA2State = LOW;             //...dann Ballzuführung stoppen...
      vLastShot = millis();       //...Zeitpunkt letzter Ballschuss speichern...
      fDirection();               //...nächste Servoposition ermitteln...
      vMoveServo = true;          //...Servo anschliessend positionieren lassen...
      vLastMotorstart = 0;        //...und Prüfung auf StopDelay ausschalten.
    }

    if (vMA2State == LOW) {                            //Falls der Motor gestoppt ist...
      if ((millis() - vLastShot) >= vPotIntervall) {  //...und die Wartezeit (Schussintervall) vorbei ist, dann...
        vMA2State = HIGH;                              //...Motor für Ballzuführung einschalten...
        vLastMotorstart = millis();                   //...und Zeitpunkt der Motoreinschaltung speichern.
      }
    }
  }
}



//Ermittelt, welche Position vom Servo aufgrund der Schalterstellung als nächstes angefahren werden soll.
void fDirection() {
  //Wenn alle Richtungsschalter auf HIGH sind, dann wird eine Zufallsrichtung ermittelt.
  if ((digitalRead(cSwitchLeftPin) == HIGH) && (digitalRead(cSwitchCenterPin) == HIGH) && (digitalRead(cSwitchRightPin) == HIGH)) {
    vServoPos = random(cServoPosRight / 10, cServoPosLeft / 10) * 10;   //Damit sich die zufälligen Richtungen klar voneinander unterscheiden, werden nur 10er Schritte ermittelt.
  }
  //Ansonsten werden die eingeschalteten Richtungsvorgaben von links nach rechts durchlaufen
  else
  {
    switch (vServoPos) {
      case cServoPosLeft: //links
        if (digitalRead(cSwitchCenterPin) == LOW) {
          vServoPos = cServoPosCenter;
        } else if (digitalRead(cSwitchRightPin) == LOW) {
          vServoPos = cServoPosRight;
        }
        break;

      case cServoPosCenter: //Mitte
        if (digitalRead(cSwitchRightPin) == LOW) {
          vServoPos = cServoPosRight;
        } else if (digitalRead(cSwitchLeftPin) == LOW) {
          vServoPos = cServoPosLeft;
        }
        break;

      case cServoPosRight: //rechts
        if (digitalRead(cSwitchLeftPin) == LOW) {
          vServoPos = cServoPosLeft;
        } else if (digitalRead(cSwitchCenterPin) == LOW) {
          vServoPos = cServoPosCenter;
        }
        break;

      default:                        //Falls von "Zufall" auf "Richtungsvorgabe" geschaltet wird, entspricht die aktuelle Servoposition keiner der drei vorgegebenen Standardrichtungen.
        vServoPos = cServoPosCenter;  //In diesem Fall soll ein Zwischenschuss in die Mitte abgegeben werden, damit sich die Schussrichtung anschliessend entsprechend den Schalterstellungen verhält.
        break;
    }
  }
}
