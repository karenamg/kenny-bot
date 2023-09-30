#include "LedControl.h"
#include "Servo.h"

// Pin Definitions
#define BUTTON 2
#define SENSOR 3
#define CLK 4
#define CS 5
#define DIN 6
#define SERVO_1 7
#define SERVO_2 8
#define LED_R 9
#define LED_G 10
#define LED_B 11

// Estructura de cada vagón programado
struct Puzzle {
  int slave;
  char eyes; 
  char direction; 
  char color; 
  char sound; 
  unsigned long timeslice;
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
const unsigned long intervalScan = 500; 
unsigned long lastScanTime = 0;

// Duración de cada recorrido
const unsigned long timeslice = 10000;

// Controladores
LedControl lc = LedControl(DIN, CLK, CS, 2);
Servo servo1;
Servo servo2;
// SoftwareSerial portAudio(TX_DF, RX_DF); // RX, TX
// DFRobotDFPlayerMini dfPlayer;

// Plantilla
byte eye_open[8] = {
  B00111100,
  B01111110,
  B11111111,
  B11100111,
  B11100111,
  B11111111,
  B01111110,
  B00111100
};

byte eye_right[8] = {
  B00111100,
  B01111110,
  B11111111,
  B11001111,
  B11001111,
  B11111111,
  B01111110,
  B00111100
};

byte eye_left[8] = {
  B00111100,
  B01111110,
  B11111111,
  B11110011,
  B11110011,
  B11111111,
  B01111110,
  B00111100
};

byte eye_close[8] = {
  B00000000,
  B00000000,
  B00000000,
  B00111100,
  B11111111,
  B00111100,
  B00000000,
  B00000000
};

void setup(){
  // Configuración de pines
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SENSOR, INPUT_PULLUP);

  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  
  
  // display setup
  int devices = lc.getDeviceCount();
  for(int address = 0; address < devices; address++) {
    lc.shutdown(address, false);
    lc.setIntensity(address, 1);
    lc.clearDisplay(address);
  }  
  open_eyes();
  startRainbowEffect(3000, 8);

  // servo1.attach(SERVO_1); 
  // servo2.attach(SERVO_2); 

  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(SENSOR), sensorInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, CHANGE);
  delay(50);
}

void loop(){
  updateSystem();
  if (checkStateChange())
    updateLed();
}

void buttonPressed() {
  buttonState = digitalRead(BUTTON);
  delay(50);

  if (buttonState){
    if(currentRobotState == STANDBY && currentSystemState == OK){
      parseMessage(scanResult);

      if (list == NULL){
        currentRobotState = STOPING;
        currentSystemState = RESET;
        startBlinkEffect(3000, 300, "red");
      } else {
        currentRobotState = READING;
        startRainbowEffect(3000, 8);
      }

    } else if (currentRobotState == PAUSED && paused){
      if ((millis() - buttonPressTime) < 4000){
        currentRobotState = PLAYING;
        // se reanudan las acciones del robot donde se pausaron
      } else {
        currentRobotState = STOPING;
        currentSystemState = RESET;
        startBlinkEffect(3000, 300, "red");
        deleteList();
      }
      buttonPressTime = 0;
    }
  } else {
    if (currentRobotState == PLAYING){
      currentRobotState = PAUSED;
      paused = false;
      // se envía mensaje de pausado a esclavos
      // se pausan todas las acciones y se guarda el progreso
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

    Puzzle *newNode = new Puzzle{slave, eyes, direction, color, sound, timeslice, NULL};

    if (list == NULL)
      list = newNode;
    else
      currentPuzzle->next = newNode;

    currentPuzzle = newNode;
  }
  i++;
  iterationsCount = loopToInteger(actions[i]);
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

// Funciones para display
void open_eyes(){
  for (int i = 0; i < 8; i++) {
    lc.setRow(0,i,eye_open[i]);
    lc.setRow(1,i,eye_open[i]);
  }
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
    green_blink = 255;
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

bool updateBlinkEffect() {
  if (blinkStartTime == 0) {
    // El efecto de parpadeo no está activo
    return 0;
  }

  unsigned long elapsedTime = millis() - blinkStartTime;

  if (elapsedTime >= blinkDuration) {
    // El efecto de parpadeo ha finalizado
    blinkStartTime = 0;
    turnOff(); // Apagar el LED al finalizar el efecto
    return 0;
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
  wagonsCount = 0;
  iterationsCount = 0;
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
}

void updateLed(){
  switch (currentRobotState){
    case TURN_ON:
      if (!updateRainbowEffect())
        currentRobotState = STANDBY;
    break;
    case STANDBY:
      switch (currentSystemState){
        case ERROR:
          setColor(255, 0, 0);
        break;
        case BUILDING:
          setColor(255, 255, 0);
        break;
        case OK:
          setColor(0, 0, 255);
        break;
      }
    break;
    case READING:
      if (!updateRainbowEffect())
        currentRobotState = PLAYING;
    break;
    case PLAYING:
      setColor(255, 255, 255);
    break;
    case PAUSED:
      setColor(255, 255, 0);
    break;
    case STOPING:
      if (!updateBlinkEffect())
        currentRobotState = STANDBY;
    break;
    case FINISHED:
      if (!updateRainbowEffect())
        currentRobotState = STANDBY;
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
  
  if (changeSystemState)
    previousSystemState = currentSystemState;

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
        currentRobotState = STOPING;
        currentSystemState = RESET;
        startBlinkEffect(3000, 300, "red");
        deleteList();
      }
      return 1;
    break;
  }

  return 0;
}