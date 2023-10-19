#include "MD_RobotEyes.h"
#include "Servo.h"
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

// Pin Definitions
#define BUTTON 2    //CHECK
#define SENSOR 4
#define LED_R 3     //CHECK
#define LED_G 5     //CHECK
#define LED_B 6    //CHECK
#define RX 7        //CHECK
#define TX 8        //CHECK
#define SERVO_1 9   //CHECK
#define SERVO_2 10   //CHECK
#define DIN 11      //CHECK
#define CS 12       //CHECK
#define CLK 13      //CHECK
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 2

// Estructura de cada vagón programado
struct Puzzle {
  int slave;
  char eyes; 
  char direction; 
  char color; 
  char sound; 
  unsigned long timeSlice;
  Puzzle *next;
};

// Variables globales
Puzzle *list = NULL;
Puzzle *currentPuzzle = NULL;

volatile bool buttonState = true;
volatile bool paused = false;
volatile bool sensorState = false;
volatile unsigned long buttonPressTime = 0;

enum BotState {
  TURN_ON,
  STANDBY,
  READING,
  PLAYING,
  PAUSED,
  STOPING,
  FINISHED
};

enum SerialPortState {
  SLEEPING,
  START_COUNT,
  WAIT_COUNT_RESPONSE,
  START_SCAN,
  WAIT_SCAN_RESPONSE
};

enum SysState {
  RESET,
  WAITING,
  ERROR,
  BUILDING,
  OK
};

enum AnimationState {
  NEUTRAL,
  BLINK,
  WINK,
  LEFT_RIGHT,
  UP,
  DOWN,
  ANGRY,
  SAD,
  EVIL,
  SQUINT,
  DEAD,
  SCAN_V,
  SCAN_H
};

typedef enum
{
  NORMAL,
  LOOPING,
  RESETING,
} typeOfAnimation_t;

enum AnimControl{
  IDLE,
  IN_PROGRESS,
  FINISHING
};

AnimationState animationState = NEUTRAL;
AnimControl animationControl = IDLE;
bool controlActive = false;

bool isWinking = false;
bool isLeftRight = false;
unsigned long startAnimationTime = 0;

volatile BotState currentRobotState = TURN_ON;
volatile BotState previousRobotState = TURN_ON;
volatile SerialPortState serialPortState = SLEEPING;
volatile SysState currentSystemState = RESET;
volatile SysState previousSystemState = RESET;

// Variables para el efecto del arcoíris
unsigned long rainbowStartTime = 0;
unsigned long rainbowDuration = 0;
int rainbowCycles = 0;
float rainbowProgress = 0.0;

// Variables para paradeo de led
unsigned long blinkStartTime = 0;
unsigned long blinkDuration = 0; // Duración total del parpadeo en milisegundos
unsigned long blinkInterval = 0; // Intervalo de tiempo entre encendido y apagado en milisegundos
bool blinkState = false; // Estado del parpadeo (0: LED apagado, 1: LED encendido)
int red_blink = 0;
int green_blink = 0;
int blue_blink = 0;

// Variables de exploracion
String message = "";
String scanResult = "";
int wagonsCount = 0;
int iterationsCount = 0;
const unsigned long intervalScan = 750; 
unsigned long lastScanTime = 0;

// Duración de cada recorrido
const unsigned long timeSlice = 10000;
unsigned long elapsedTime = 0;
unsigned long playStartTime = 0;

// Controladores
MD_MAX72XX M = MD_MAX72XX(HARDWARE_TYPE, DIN, CLK, CS, MAX_DEVICES);

MD_RobotEyes E;
bool animation = false;

typedef struct
{
  MD_RobotEyes::emotion_t e;
  uint16_t timePause;  // in milliseconds
} sampleItem_t;

const sampleItem_t eSeq[] = {
  { MD_RobotEyes::E_NEUTRAL, 20000 },
  { MD_RobotEyes::E_BLINK, 1000 },
  { MD_RobotEyes::E_WINK, 1000 },
  { MD_RobotEyes::E_LOOK_L, 1000 },
  { MD_RobotEyes::E_LOOK_R, 1000 },
  { MD_RobotEyes::E_LOOK_U, 1000 },
  { MD_RobotEyes::E_LOOK_D, 1000 },
  { MD_RobotEyes::E_ANGRY, 1000 },
  { MD_RobotEyes::E_SAD, 1000 },
  { MD_RobotEyes::E_EVIL, 1000 },
  { MD_RobotEyes::E_EVIL2, 1000 },
  { MD_RobotEyes::E_SQUINT, 1000 },
  { MD_RobotEyes::E_DEAD, 1000 },
  { MD_RobotEyes::E_SCAN_UD, 1000 },
  { MD_RobotEyes::E_SCAN_LR, 1000 },
};

Servo servo1;
Servo servo2;
SoftwareSerial softSerial(RX, TX); // RX, TX
DFRobotDFPlayerMini myDFPlayer;


void setup(){
  // Configuración de pines
  pinMode(BUTTON, INPUT_PULLUP);
  //pinMode(SENSOR, INPUT_PULLUP);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  
  // display setup
  M.begin();
  M.control(MD_MAX72XX::INTENSITY, 0); 
  E.begin(&M);
  E.setBlinkTime(10000);

  Serial.begin(9600);
  softSerial.begin(9600);

  //attachInterrupt(digitalPinToInterrupt(SENSOR), sensorInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, CHANGE);
  delay(50);

  if (myDFPlayer.begin(softSerial)) {  //Use softwareSerial to communicate with mp3.
    myDFPlayer.volume(30);  //Set volume value. From 0 to 30
    myDFPlayer.play(1);  //Play the first mp3
  }

  startRainbowEffect(3000, 8);
  startScanHAnimation();
}

void loop(){
  updateSystem();

  E.runAnimation();

  if (checkStateChange())
    updateRobot();
}

void buttonPressed() {
  buttonState = digitalRead(BUTTON);
  delay(10);

  if (buttonState){
    if(currentRobotState == STANDBY && currentSystemState == OK){
      parseMessage(scanResult);

      if (list == NULL){
        currentRobotState = STOPING;
        currentSystemState = RESET;
        Serial.print("*STANDBY!");
        myDFPlayer.play(10);
        startBlinkEffect(3000, 300, "red");
        startDeadAnimation();
        Serial.flush();
      } else {
        currentRobotState = READING;
        myDFPlayer.play(9);
        startRainbowEffect(5000, 14);
        startScanVAnimation();
        enableServos();
        Serial.print("*READING!");
        Serial.flush();
      }

    } else if (currentRobotState == PAUSED && paused){
      if ((millis() - buttonPressTime) < 4000){
        currentRobotState = PLAYING;
        finishBlinkEffect();
        setColor(0, 255, 0);
        if (!executeActions()){
          currentRobotState = STOPING;
          currentSystemState = RESET;
          Serial.print("*STANDBY!");
          myDFPlayer.play(10);
          Serial.flush();
          startBlinkEffect(3000, 300, "red");
          startDeadAnimation();
          stop();
          disableServos();
          deleteList();
        }
        // se reanudan las acciones del robot donde se pausaron
      } else {
        currentRobotState = STOPING;
        currentSystemState = RESET;
        Serial.print("*STANDBY!");
        myDFPlayer.play(10);
        Serial.flush();
        startBlinkEffect(3000, 300, "red");
        startDeadAnimation();
        stop();
        disableServos();
        deleteList();
        // detener sonido
      }
      buttonPressTime = 0;
    }
    if (currentSystemState == ERROR){
      myDFPlayer.play(2);
    } else if (currentSystemState == BUILDING){
      myDFPlayer.play(8);
    }
  } else {
    if (currentRobotState == PLAYING){
      currentRobotState = PAUSED;
      paused = false;
      startBlinkEffect(0, 750, "yellow");
      pauseActions();
      return;
    } else if (currentRobotState == PAUSED){
      buttonPressTime = millis();
      paused = true;
    }
  }
}

void sensorInterrupt() {
  sensorState = digitalRead(SENSOR);
  delay(50);

  // Insertar logica para pausar al detectar obstaculos
}

int counterToInteger(char c){
  switch (c){
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    default: 
      return 9;
  }
}

int wagonToInteger(char i){
  switch (i){
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    default: 
      return 0;
  }
}

int loopToInteger(char i){
  switch (i){
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    default: 
      return -1;
  }
}

void parseMessage(String actions){
  int i = 1;

  while (actions[i] != '%'){
    int slave;
    char eyes; 
    char direction; 
    char color; 
    char sound; 
    Puzzle *next;

    slave = wagonToInteger(actions[i]);
    i++;
    eyes = actions[i];
    i++;
    direction = actions[i];
    i++;
    color = actions[i];
    i++;
    sound = actions[i];
    i++;

    Puzzle *newNode = new Puzzle{slave, eyes, direction, color, sound, timeSlice, NULL};

    if (list == NULL)
      list = newNode;
    else
      currentPuzzle->next = newNode;

    currentPuzzle = newNode;
  }
  i++;
  iterationsCount = loopToInteger(actions[i]);
  currentPuzzle = list;
}

void deleteList() {
  Puzzle* currentNode = list;
  while (currentNode != NULL) {
    Puzzle* nextNode = currentNode->next;
    delete currentNode;
    currentNode = nextNode;
  }
  list = NULL; // Reiniciar el puntero a la lista
  currentPuzzle = NULL;
}

void setColor(int R, int G, int B) {
  analogWrite(LED_R, R);
  analogWrite(LED_G, G);
  analogWrite(LED_B, B);
}

void turnOff() {
  setColor(0, 0, 0);
}

// Función para iniciar el efecto del arcoíris
void startRainbowEffect(unsigned long duration, int cycles) {
  rainbowStartTime = millis();
  rainbowDuration = duration;
  rainbowCycles = cycles;
  rainbowProgress = 0.0;
}

// Función para iniciar el parpadeo de led
void startBlinkEffect(unsigned long duration, unsigned long interval, String color) {
  blinkStartTime = millis();
  blinkDuration = duration;
  blinkInterval = interval;
  turnOff();
  blinkState = false;

  if (color == "red"){
    red_blink = 255;
    green_blink = 0;
    blue_blink = 0;
  } else if (color == "green"){
    red_blink = 0;
    green_blink = 255;
    blue_blink = 0;
  } else if (color == "blue"){
    red_blink = 0;
    green_blink = 0;
    blue_blink = 255;
  } else if (color == "yellow"){
    red_blink = 255;
    green_blink = 100;
    blue_blink = 0;
  } else if (color == "pink"){
    red_blink = 255;
    green_blink = 0;
    blue_blink = 255;
  } else if (color == "cyan"){
    red_blink = 0;
    green_blink = 255;
    blue_blink = 255;
  } else if (color == "white"){
    red_blink = 255;
    green_blink = 255;
    blue_blink = 255;
  } else {
    red_blink = 0;
    green_blink = 0;
    blue_blink = 0;
  }
}

void finishBlinkEffect() {
  blinkStartTime = 0;
  blinkDuration = 0;
  blinkInterval = 0;
  turnOff();
  blinkState = false;

  red_blink = 0;
  green_blink = 0;
  blue_blink = 0;
}

bool updateRainbowEffect() {
  if (rainbowStartTime == 0) {
    // El efecto del arcoíris no está activo
    return 0;
  }

  unsigned long elapsedTime = millis() - rainbowStartTime;

  if (elapsedTime >= rainbowDuration) {
    // El efecto del arcoíris ha finalizado
    rainbowStartTime = 0;
    turnOff(); // Apagar el LED al finalizar el efecto
    return 0;
  }

  // Calcular el progreso actual del arcoíris
  rainbowProgress = (float)elapsedTime / (float)rainbowDuration;

  // Calcular el valor del componente rojo (R)
  int red = (1 + sin(rainbowProgress * 2 * rainbowCycles * PI + 2 * PI / 3)) * 127;
  // Calcular el valor del componente verde (G)
  int green = (1 + sin(rainbowProgress * 2 * rainbowCycles * PI + 4 * PI / 3)) * 127;
  // Calcular el valor del componente azul (B)
  int blue = (1 + sin(rainbowProgress * 2 * rainbowCycles * PI + 6 * PI / 3)) * 127;

  setColor(red, green, blue);
  return 1;
}

bool updateBlinkEffect(bool infinity) {
  if (blinkStartTime == 0) // El efecto de parpadeo no está activo
    return 0;

  unsigned long elapsedTime = millis() - blinkStartTime;

  if (!infinity){
    if (elapsedTime >= blinkDuration) {
      finishBlinkEffect();
      return 0;
    }
  }

  if (elapsedTime % blinkInterval < blinkInterval / 2) {
    if (blinkState == 0) {
      // Encender el LED
      setColor(red_blink, green_blink, blue_blink);
      blinkState = 1;
    }
  } else {
    if (blinkState == 1) {
      // Apagar el LED
      turnOff();
      blinkState = 0;
    }
  }

  return 1;
}

void resetScanValues(){
  lastScanTime = millis();
  message = "";
  scanResult = "";
}

int scanMessage(String message){
  int counterX = 0; 

  for (int i = 0; i < message.length(); i++)
    if (message.charAt(i) == 'X') 
      counterX++; 
    
  return counterX;
}

void updateSystem(){
  unsigned long currentTime = millis();

  if(currentSystemState == RESET){
    resetScanValues();
    currentSystemState = WAITING;
    serialPortState = START_COUNT;
  } else if(currentSystemState == WAITING){
    if(currentTime - lastScanTime >= intervalScan){
      currentSystemState = ERROR;
      serialPortState = SLEEPING;
      resetScanValues();
      return;
    }
  }

  switch (serialPortState){
    case SLEEPING:
      if(currentTime - lastScanTime >= intervalScan){
        resetScanValues();
        serialPortState = START_COUNT;
      }
    break;
    case START_COUNT:
      Serial.print("#0!");   // Inicia conteo de vagones
      Serial.flush();
      serialPortState = WAIT_COUNT_RESPONSE;
    break;
    case WAIT_COUNT_RESPONSE:
      if(currentTime - lastScanTime >= intervalScan){
        currentSystemState = ERROR;
        serialPortState = SLEEPING;
        resetScanValues(); // REVISARRR
        return;
      }

      if(Serial.available() > 0){
        message = Serial.readStringUntil('!');
        wagonsCount = counterToInteger(message[1]);

        if(wagonsCount == 0 || wagonsCount == 1 || wagonsCount == 9){
          currentSystemState = ERROR;
          serialPortState = SLEEPING;
          resetScanValues(); // REVISARRR
          return;
        }

        serialPortState = START_SCAN;
      }
    break;
    case START_SCAN:
      Serial.print("$!");   // Inicia scaneo de vagones
      Serial.flush();
      serialPortState = WAIT_SCAN_RESPONSE;
    break;
    case WAIT_SCAN_RESPONSE:
      if(currentTime - lastScanTime >= intervalScan){
        currentSystemState = ERROR;
        serialPortState = SLEEPING;
        resetScanValues();
        return;
      }

      if(Serial.available() > 0){
        scanResult = Serial.readStringUntil('!');

        if(scanResult.length() == 3){
          currentSystemState = ERROR;
          serialPortState = SLEEPING;
          resetScanValues(); // REVISARRR
          return;
        }

        serialPortState = SLEEPING;

        if (scanMessage(scanResult) > 0)
          currentSystemState = BUILDING;
        else 
          currentSystemState = OK;
      }
    break;
  }

  Serial.flush();
}

void updateRobot(){
  switch (currentRobotState){
    case TURN_ON:
      if (!updateRainbowEffect()) {
        currentRobotState = STANDBY;
        finishAnimation();
      }
    break;
    case STANDBY:
      switch (currentSystemState){
        case ERROR:
          setColor(255, 0, 0);
        break;
        case BUILDING:
          setColor(255, 100, 0);
        break;
        case OK:
          setColor(0, 255, 0);
        break;
      }
    break;
    case READING:
      if (!updateRainbowEffect()){
        finishAnimation();
        currentRobotState = PLAYING;
        setColor(0, 255, 0);
        elapsedTime = 0;
        if (!executeActions()){
          currentRobotState = STOPING;
          currentSystemState = RESET;
          Serial.print("*STANDBY!");
          myDFPlayer.play(10);
          Serial.flush();
          startBlinkEffect(3000, 300, "red");
          startDeadAnimation();
          stop();
          disableServos();
          deleteList();
        }
      }
    break;
    case PLAYING:
      updateActions();
    break;
    case PAUSED:
      updateBlinkEffect(true);
    break;
    case STOPING:
      if (!updateBlinkEffect(false)){
        finishAnimation();
        currentRobotState = STANDBY;
      }
    break;
    case FINISHED:
      if (!updateRainbowEffect()){
        currentRobotState = STANDBY;
        finishAnimation();
        Serial.print("*STANDBY!");
        Serial.flush();
      }
    break;
  }
}

bool checkStateChange(){
  bool changeRobotState = (currentRobotState != previousRobotState);
  bool changeSystemState = (currentSystemState != previousSystemState);

  if (changeRobotState){
    previousRobotState = currentRobotState;
    return 1;
  }
  
  if (changeSystemState){
    switch (currentSystemState){
      case ERROR:
        if(previousSystemState == BUILDING || previousSystemState == OK)
          myDFPlayer.play(8);
      break;
      case BUILDING:
        if(previousSystemState == ERROR)
          myDFPlayer.play(6);
        if(previousSystemState == OK)
          myDFPlayer.play(10);
      break;
      case OK:
        if(previousSystemState == ERROR || previousSystemState == BUILDING)
          myDFPlayer.play(3);
      break;
    }
    
    previousSystemState = currentSystemState;
  }

  switch(currentRobotState){
    case TURN_ON:
    case FINISHED:
    case STOPING:
      return 1;
    break;
    case STANDBY:
      if(changeSystemState)
        return 1;
    break;
    case READING:
    case PLAYING:
    case PAUSED:
      if(changeSystemState){
        forceFinishAnimation();
        currentRobotState = STOPING;
        currentSystemState = RESET;
        Serial.print("*STANDBY!");
        myDFPlayer.play(10);
        Serial.flush();
        startBlinkEffect(3000, 300, "red");
        startDeadAnimation();
        stop();
        disableServos();
        // detener sonido
        deleteList();
      }
      return 1;
    break;
  }
  return 0;
}

String findColor(char c){
  switch(c){
    case 'A':
      return "blue";
    case 'B':
      return "red";
    case 'C':
      return "green";
    case 'D':
      return "pink";
    case 'F':
      return "yellow";
    case 'H':
      return "cyan";
    default:
      return "white";
  }
}

void startAnimation(char c){
  switch(c){
    case 'A':
      startSadAnimation();
      break;
    case 'B':
      startAngryAnimation();
      break;
    case 'C':
      startLeftRightAnimation();
      break;
    case 'D':
      startWinkBlinkAnimation();
      break;
    case 'E':
      startEvilAnimation();
      break;
    case 'F':
      startNeutralAnimation();
      break;
    default:
      startSquintAnimation();
      break;
  }  

  animationControl = IN_PROGRESS;
  controlActive = false;
}

void startServos(char c){
  switch(c){
    case 'A':
      straight();
    break;
    case 'B':
      left();
    break;
    case 'C':
      right();
    break;
    case 'D':
      stop();
    break;
    case 'E':
      left180();
    break;
    case 'F':
      right180();
    break;
    default:
      stop();
    break;
  }
}

void startMusic(char c){
  switch(c){
    case 'A':
      myDFPlayer.play(12);
    break;
    case 'B':
      myDFPlayer.play(13);
    break;
    case 'C':
      myDFPlayer.play(14);
    break;
    case 'D':
      myDFPlayer.play(15);
    break;
    case 'E':
      myDFPlayer.play(16);
    break;
    case 'F':
      myDFPlayer.play(17);
    break;
    default:
      myDFPlayer.pause();
    break;
  }
}

bool executeActions(){
  if (currentPuzzle == NULL)
    return false;

  playStartTime = millis();
  Serial.print("*" + idToString(currentPuzzle->slave) + "!");
  if(!controlActive){
    startAnimation(currentPuzzle->eyes);
    startServos(currentPuzzle->direction);
    startMusic(currentPuzzle->sound);
  }
  Serial.flush();
  return true;
}

bool updateActions(){
  if (currentPuzzle == NULL)
    return false;

  unsigned long currentTime = millis();

  if (currentTime - playStartTime >= currentPuzzle->timeSlice - elapsedTime){
    if (currentPuzzle->next == NULL){
      if (iterationsCount > 0){
        if(!controlActive){
          controlActive = true;
          finishAnimation();
          stop();
          animationControl = FINISHING;
        } else {
          if (E.runAnimation()){
            animationControl = IDLE;
          }
        }

        if(animationControl == IDLE){
          controlActive = false;
          myDFPlayer.pause();
          delay(300);
          currentPuzzle = list;
          elapsedTime = 0;
          if (!executeActions()){
            currentRobotState = STOPING;
            currentSystemState = RESET;
            Serial.print("*STANDBY!");
            myDFPlayer.play(10);
            Serial.flush();
            startBlinkEffect(3000, 300, "red");
            startDeadAnimation();
            stop();
            disableServos();
            deleteList();
          }
          iterationsCount--;
        }
      } else {
        // FINALIZAR
        if(!controlActive){
          controlActive = true;
          finishAnimation();
          animationControl = FINISHING;
          stop();
          disableServos();
        } else {
          if (E.runAnimation()){
            animationControl = IDLE;
          }
        }
        if(animationControl == IDLE){
          controlActive = false;
          myDFPlayer.pause();
          elapsedTime = 0;
          delay(300);
          myDFPlayer.play(5);
          currentRobotState = FINISHED;
          currentSystemState = RESET;
          Serial.print("*FINISHING!");
          Serial.flush();
          startRainbowEffect(2000, 6);
          startDeadAnimation();
          deleteList();
          playStartTime = 0;
          return false;
        }
      }
    } else {
      if(!controlActive){
        controlActive = true;
        finishAnimation();
        stop();
        animationControl = FINISHING;
      } else {
        if (E.runAnimation()){
          animationControl = IDLE;
        }
      }
      if(animationControl == IDLE){
        controlActive = false;
        myDFPlayer.pause();
        delay(300);
        currentPuzzle = currentPuzzle->next;
        elapsedTime = 0;
        if (!executeActions()){
          currentRobotState = STOPING;
          currentSystemState = RESET;
          Serial.print("*STANDBY!");
          myDFPlayer.play(10);
          Serial.flush();
          startBlinkEffect(3000, 300, "red");
          startDeadAnimation();
          stop();
          disableServos();
          deleteList();
        }
      }
    }
  }

  updateBlinkEffect(true);
  if (animationState == WINK)
    updateWinkBlinkAnimation();
  if (animationState == LEFT_RIGHT)
    updateLeftRightAnimation();
  return true;
}

void pauseActions(){
  if (currentPuzzle == NULL)
    return;
  
  Serial.print("*PAUSED!");
  Serial.flush();

  elapsedTime = elapsedTime + (millis() - playStartTime);
  forceFinishAnimation();
  if (controlActive)
    animationControl = IDLE;
  stop();
  myDFPlayer.pause();
}

void finishAnimation(){
  animation = E.runAnimation(); 

  switch(animationState){
    case NEUTRAL:
    break;
    case BLINK:
    break;
    case WINK:
    case LEFT_RIGHT:
      finishAnim();
    break;
    case UP:
    break;
    case DOWN:
    break;
    case ANGRY:
      finishAngryAnimation();
    break;
    case SAD:
      finishSadAnimation();
    break;
    case EVIL:
      finishEvilAnimation();
    break;
    case SQUINT:
      finishSquintAnimation();
    break;
    case DEAD:
    case SCAN_V:
    case SCAN_H:
      stopAnimation();
    break;
    }
}

void startScanHAnimation(){
  E.setAnimation(eSeq[14].e, true, false);
  E.setTypeAnimation(MD_RobotEyes::LOOPING);
  E.runAnimation();
  animationState = SCAN_H;
}

void startScanVAnimation(){
  E.setAnimation(eSeq[13].e, true, false);
  E.setTypeAnimation(MD_RobotEyes::LOOPING);
  E.runAnimation();
  animationState = SCAN_V;
}

void startNeutralAnimation(){
  E.setAnimation(eSeq[0].e, false, false);
  E.runAnimation();
  E.setAutoBlink(true);
  E.setBlinkTime(5000);
  animationState = NEUTRAL;
}

void startDeadAnimation(){
  E.setAnimation(eSeq[12].e, true, false);
  E.setTypeAnimation(MD_RobotEyes::LOOPING);
  E.runAnimation();
  animationState = DEAD;
}

void stopAnimation() {
  E.setTypeAnimation(MD_RobotEyes::RESETING);
  finishAnim();
}

void forceFinishAnimation() {
  if (E.getTypeAnimation() == LOOPING)
    E.setTypeAnimation(MD_RobotEyes::RESETING);
  E.setAutoBlink(true);
  E.setBlinkTime(10000);
  finishAnim();
}

void startAngryAnimation(){
  E.setAnimation(eSeq[7].e, false, false);
  E.setAutoBlink(false);
  E.runAnimation();
  animationState = ANGRY;
}

void finishAngryAnimation() {
  E.setAnimation(eSeq[7].e, false, true);
  E.setAutoBlink(true);
  E.setBlinkTime(10000);
  E.runAnimation();
  animationState = NEUTRAL;
}

void startSadAnimation(){
  E.setAnimation(eSeq[8].e, false, false);
  E.setAutoBlink(false);
  E.runAnimation();
  animationState = SAD;
}

void finishSadAnimation() {
  E.setAnimation(eSeq[8].e, false, true);
  E.setAutoBlink(true);
  E.setBlinkTime(10000);
  E.runAnimation();
  animationState = NEUTRAL;
}

void startEvilAnimation(){
  E.setAnimation(eSeq[9].e, false, false);
  E.setAutoBlink(false);
  E.runAnimation();
  animationState = EVIL;
}

void finishEvilAnimation() {
  E.setAnimation(eSeq[9].e, false, true);
  E.setAutoBlink(true);
  E.setBlinkTime(10000);
  E.runAnimation();
  animationState = NEUTRAL;
}

void startSquintAnimation(){
  E.setAnimation(eSeq[11].e, false, false);
  E.setAutoBlink(false);
  E.runAnimation();
  animationState = SQUINT;
}

void finishSquintAnimation() {
  E.setAnimation(eSeq[11].e, false, true);
  E.setAutoBlink(true);
  E.setBlinkTime(10000);
  E.runAnimation();
  animationState = NEUTRAL;
}

void startWinkBlinkAnimation(){
  isWinking = false;
  E.setAnimation(eSeq[0].e, false, false);
  startAnimationTime = millis();
  animationState = WINK;
  E.runAnimation();
}

void updateWinkBlinkAnimation(){
  if (isWinking && millis() - startAnimationTime >= 3000)
  {
    E.setAnimation(eSeq[1].e, true, false);
    startAnimationTime = millis();
    isWinking = false;
  }
  else if (!isWinking && millis() - startAnimationTime >= 3000)
  {
    E.setAnimation(eSeq[2].e, true, false);
    startAnimationTime = millis();
    isWinking = true;
  }
  E.runAnimation();
}

void startLeftRightAnimation(){
  E.setAnimation(eSeq[0].e, false, false);
  isLeftRight = false;
  animationState = LEFT_RIGHT;
  startAnimationTime = millis();
}

void updateLeftRightAnimation(){
  if (isLeftRight)
  {
    if (E.runAnimation()){
      E.setAnimation(eSeq[4].e, true, false);
      isLeftRight = false;
    }
  }
  else if (millis() - startAnimationTime >= 3500)
  {
    E.setAnimation(eSeq[3].e, true, false);
    isLeftRight = true;
    startAnimationTime = millis();
  }
  E.runAnimation();
}

void finishAnim() {
  E.setAnimation(eSeq[0].e, false, false);
  E.runAnimation();
  animationState = NEUTRAL;
}

String idToString(int id){
  switch (id){
    case 1:
      return "1";
    case 2:
      return "2";
    case 3:
      return "3";
    case 4:
      return "4";
    case 5:
      return "5";
    case 6:
      return "6";
    default: 
      return "9";
  }
}

void enableServos(){
  servo1.attach(SERVO_1); 
  servo2.attach(SERVO_2); 
}

void disableServos(){
  servo1.detach(); 
  servo2.detach(); 
}

void stop(){
  servo1.write(90);
  servo2.write(90);
}

void straight(){
  servo1.write(100);
  servo2.write(80);
}

void straightSlow(){
  servo1.write(105);
  servo2.write(75);
}

void left(){
  servo1.write(100);
  servo2.write(70);
}

void right(){
  servo1.write(110);
  servo2.write(80);
}

void left180(){
  servo1.write(95);
  servo2.write(70);
}

void right180(){
  servo1.write(110);
  servo2.write(85);
}

void left45(){
  servo1.write(95);
  servo2.write(80);
}

void right45(){
  servo1.write(100);
  servo2.write(85);
}