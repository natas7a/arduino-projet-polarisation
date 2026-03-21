#include <LiquidCrystal.h>
#include <string.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <Stepper.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Boutons 
const int pinStart    = 2;
const int pinValider  = 3;
const int pinQuestion = 4;
const int pinPot      = A0;

// Servo 
const int pinServo1 = A1;
Servo servo1;

// Stepper
const int STEPS_PER_REV = 2048;
const int pinIN1 = 5;
const int pinIN2 = 6;
const int pinIN3 = A2;
const int pinIN4 = A4;
Stepper stepperMotor(STEPS_PER_REV, pinIN1, pinIN3, pinIN2, pinIN4);

// LED
const int pinLEDYes = A3;
const int NB_LEDS = 56;
Adafruit_NeoPixel stripYes(NB_LEDS, pinLEDYes, NEO_GRB + NEO_KHZ800);

int etat = 0; // 0=attente, 1=saisie, 2=moyenne

// Questions
const int NB_QUESTIONS = 3;
const char* questions[NB_QUESTIONS] = {
  "Jaune va gagner ?",
  "Guerre en 2026 ?",
  "Cours d'arduino ??"
};
int indexQ = 0;

// Confidentialite
int yesAffiche = 50;
int noAffiche  = 50;
bool potActive = false;
int lastBrut = -1;

// Stockage
long sommeYesQ[NB_QUESTIONS] = {0, 0, 0};
int  nbRepQ[NB_QUESTIONS]    = {0, 0, 0};

// Polarisation
int minYesQ[NB_QUESTIONS] = {100, 100, 100};
int maxYesQ[NB_QUESTIONS] = {0, 0, 0};

// Moyennes mémorisées par question
int moyenneYesQ[NB_QUESTIONS] = {50, 50, 50};
int moyenneNoQ[NB_QUESTIONS]  = {50, 50, 50};

// Moyennes de la question active
int moyenneYes = 50;
int moyenneNo  = 50;

// Anti double clic
bool startLock = false;
bool validerLock = false;
bool questionLock = false;

// Reset total long appui
unsigned long startPressStart = 0;

// fonctionnalités led
void ledsOff() {
  stripYes.clear();
  stripYes.show();
}

void chargerMoyenneQuestionActive() {
  moyenneYes = moyenneYesQ[indexQ];
  moyenneNo  = moyenneNoQ[indexQ];
}

void ledsShowYesNoMix(int yesPercent) {
  int nbYes = (yesPercent * NB_LEDS + 50) / 100;
  nbYes = constrain(nbYes, 0, NB_LEDS);

  stripYes.clear();

  for (int i = 0; i < NB_LEDS; i++) {
    if (i < nbYes) {
      // YES = magenta
      stripYes.setPixelColor(i, stripYes.Color(255, 0, 255));
    } else {
      // NO = cyan
      stripYes.setPixelColor(i, stripYes.Color(0, 255, 255));
    }
  }

  stripYes.show();
}

void afficherLedsQuestionActive() {
  chargerMoyenneQuestionActive();
  ledsShowYesNoMix(moyenneYes);
}

// écran lcd
void afficherAttente() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Appuie Start");
  lcd.setCursor(0,1); lcd.print("pour lancer");
}

void afficherSaisie() {
  lcd.setCursor(0,0);
  lcd.print(questions[indexQ]);
  int len = strlen(questions[indexQ]);
  for (int i = len; i < 16; i++) lcd.print(" ");

  lcd.setCursor(0,1);
  lcd.print("No:");
  lcd.print(noAffiche);
  lcd.print("% Yes:");
  lcd.print(yesAffiche);
  lcd.print("% ");
}

void afficherMoyenneQuestion() {
  lcd.setCursor(0,0);
  lcd.print("Moy Q");
  lcd.print(indexQ + 1);
  lcd.print("=");
  lcd.print(moyenneYes);
  lcd.print("%Yes ");

  lcd.setCursor(0,1);
  lcd.print("et ");
  lcd.print(moyenneNo);
  lcd.print("%No        ");
}

// logique
void nouvellePersonne() {
  yesAffiche = 50;
  noAffiche  = 50;
  potActive = false;
  lastBrut = analogRead(pinPot);
  lcd.clear();
  etat = 1;

  afficherLedsQuestionActive();
}

void calculerMoyenneQuestionActuelle() {
  if (nbRepQ[indexQ] == 0) {
    moyenneYesQ[indexQ] = 50;
    moyenneNoQ[indexQ]  = 50;
  } else {
    moyenneYesQ[indexQ] = (int)((float)sommeYesQ[indexQ] / nbRepQ[indexQ]);
    moyenneNoQ[indexQ]  = 100 - moyenneYesQ[indexQ];
  }

  moyenneYes = moyenneYesQ[indexQ];
  moyenneNo  = moyenneNoQ[indexQ];
}

void resetTotal() {
  for (int i = 0; i < NB_QUESTIONS; i++) {
    sommeYesQ[i] = 0;
    nbRepQ[i] = 0;
    minYesQ[i] = 100;
    maxYesQ[i] = 0;
    moyenneYesQ[i] = 50;
    moyenneNoQ[i] = 50;
  }

  indexQ = 0;
  etat = 0;
  moyenneYes = 50;
  moyenneNo  = 50;

  afficherAttente();

  servo1.write(90);

  afficherLedsQuestionActive();
}

// moteurs
void animationMoteurs7s() {
  unsigned long t0 = millis();

  // Vitesse du stepper
  stepperMotor.setSpeed(12);

  while (millis() - t0 < 7000) {
    int phase = (millis() - t0) % 800;
    int angle1;

    // Servo 1 
    if (phase < 400) angle1 = map(phase, 0, 399, 40, 140);
    else             angle1 = map(phase, 400, 799, 140, 40);

    servo1.write(angle1);

    // Stepper qui tourne en continu dans le meme sens
    stepperMotor.step(4);
  }
}

void stabiliserMoteurServo(int moyYes, int dispersion) {
  int base = map(moyYes, 0, 100, 40, 140);

  const int DISP_FAIBLE = 20;
  const int DISP_FORTE  = 60;

  if (dispersion <= DISP_FAIBLE) {
    servo1.write(base);
    return;
  }

  int ecart;
  if (dispersion >= DISP_FORTE) ecart = 40;
  else ecart = map(dispersion, DISP_FAIBLE + 1, DISP_FORTE - 1, 5, 35);

  int a1 = constrain(base + ecart, 40, 140);
  servo1.write(a1);
}

// setup
void setup() {
  pinMode(pinStart, INPUT_PULLUP);
  pinMode(pinValider, INPUT_PULLUP);
  pinMode(pinQuestion, INPUT_PULLUP);

  lcd.begin(16,2);
  afficherAttente();

  servo1.attach(pinServo1);
  servo1.write(90);

  stripYes.begin();
  stripYes.setBrightness(80);

  // question 1 sans vote = 50/50
  afficherLedsQuestionActive();
}

void loop() {
  int startState    = digitalRead(pinStart);
  int validerState  = digitalRead(pinValider);
  int questionState = digitalRead(pinQuestion);

  // Reset total (appui long 2s sur Start)
  if (startState == LOW) {
    if (startPressStart == 0) startPressStart = millis();
    if (millis() - startPressStart >= 2000) {
      resetTotal();
      delay(300);
      startPressStart = 0;
      return;
    }
  } else {
    startPressStart = 0;
  }

  // changer de question
  if (questionState == LOW && !questionLock) {
    questionLock = true;

    indexQ++;
    if (indexQ >= NB_QUESTIONS) indexQ = 0;

    yesAffiche = 50;
    noAffiche  = 50;
    potActive = false;
    lastBrut = analogRead(pinPot);

    afficherLedsQuestionActive();

    if (etat != 0) {
      etat = 1;
      lcd.clear();
    }

    delay(250);
  }
  if (questionState == HIGH) questionLock = false;


  if (etat == 1) {
    int brut = analogRead(pinPot);

    if (!potActive) {
      if (abs(brut - lastBrut) > 15) potActive = true;
    }

    if (potActive) {
      yesAffiche = map(brut, 0, 1023, 0, 100);
      yesAffiche = constrain(yesAffiche, 0, 100);
      noAffiche = 100 - yesAffiche;
    }
  }

  // ETAT 0
  if (etat == 0) {
    afficherLedsQuestionActive();

    if (startState == LOW && !startLock) {
      startLock = true;
      nouvellePersonne();
      delay(200);
    }
    if (startState == HIGH) startLock = false;
  }

  // ETAT 1
  if (etat == 1) {
    afficherSaisie();

    if (validerState == LOW && !validerLock) {
      validerLock = true;

      sommeYesQ[indexQ] += yesAffiche;
      nbRepQ[indexQ]++;

      if (yesAffiche < minYesQ[indexQ]) minYesQ[indexQ] = yesAffiche;
      if (yesAffiche > maxYesQ[indexQ]) maxYesQ[indexQ] = yesAffiche;

      calculerMoyenneQuestionActuelle();

      animationMoteurs7s();

      int dispersion = maxYesQ[indexQ] - minYesQ[indexQ];
      stabiliserMoteurServo(moyenneYes, dispersion);

      // Mise à jour LED seulement après les moteurs
      ledsShowYesNoMix(moyenneYes);

      etat = 2;
      lcd.clear();
      delay(200);
    }
    if (validerState == HIGH) validerLock = false;
  }

  // ETAT 2
  if (etat == 2) {
    afficherMoyenneQuestion();
    afficherLedsQuestionActive();

    if (startState == LOW && !startLock) {
      startLock = true;
      nouvellePersonne();
      delay(200);
    }
    if (startState == HIGH) startLock = false;
  }

  delay(80);
}