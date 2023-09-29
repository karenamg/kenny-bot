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

volatile bool buttonState = false;
volatile bool sensorState = false;
volatile unsigned long buttonPressTime = 0;

enum BotState {
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

enum SystemState {
  RESET,
  WAITING,
  ERROR,
  BUILDING,
  OK
};

volatile BotState robotState = STANDBY;
volatile SerialPortState serialPortState = SLEEPING;
volatile SystemState systemState = RESET;

// Variables para el efecto del arcoíris
unsigned long rainbowStartTime = 0;
unsigned long rainbowDuration = 0;
int rainbowCycles = 0;
float rainbowProgress = 0.0;

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
  

  systemState = RESET;
  serialPortState = SLEEPING;
}

void loop(){
  // Realiza exploracion
  // unsigned long currentTime = millis();
  // if (currentTime - previousExplore >= intervalExplore){
  //   previousExplore = millis();
  // }

  
  // if(robotState == READING){
  //   setColor(255, 0, 255);
  // }
  // Serial.flush();

  int isPower = updateRainbowEffect();
  updateScan();

  if (!isPower){
    if(systemState == ERROR){
      setColor(255, 0, 0);
    } else if(systemState == BUILDING){
      setColor(255, 255, 0);
    } else if(systemState == OK){
      setColor(0, 0, 255);
    }
  }
}

void buttonPressed() {
  buttonState = digitalRead(BUTTON);
  delay(50);

  if (buttonState){
    if(systemState == OK){
      robotState = READING;
    } else if (robotState == PAUSED){
      if ((millis() - buttonPressTime) < 5000){
        robotState = PLAYING;
        // se reanudan las acciones del robot donde se pausaron
      } else {
        robotState = STOPING;
        // se eliminan las acciones en cola
      }
      buttonPressTime = 0;
    }
  } else {
    if (robotState == PLAYING){
      robotState = PAUSED;
      // se envía mensaje de pausado a esclavos
      // se pausan todas las acciones y se guarda el progreso
      return;
    } else if (robotState == PAUSED){
      buttonPressTime = millis();
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

void updateScan(){
  unsigned long currentTime = millis();

  if(systemState == RESET){
    resetScanValues();
    systemState = WAITING;
    serialPortState = START_COUNT;
  } else if(systemState == WAITING){
    if(currentTime - lastScanTime >= intervalScan){
      systemState = ERROR;
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
        systemState = ERROR;
        serialPortState = SLEEPING;
        resetScanValues(); // REVISARRR
        return;
      }

      if(Serial.available() > 0){
        message = Serial.readStringUntil('!');
        wagonsCount = counterToInteger(message[1]);

        if(wagonsCount == 0 || wagonsCount == 1 || wagonsCount == 9){
          systemState = ERROR;
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
        systemState = ERROR;
        serialPortState = SLEEPING;
        resetScanValues();
        return;
      }

      if(Serial.available() > 0){
        scanResult = Serial.readStringUntil('!');

        if(scanResult.length() == 3){
          systemState = ERROR;
          serialPortState = SLEEPING;
          resetScanValues(); // REVISARRR
          return;
        }

        serialPortState = SLEEPING;

        if (scanMessage(scanResult) > 0)
          systemState = BUILDING;
        else 
          systemState = OK;
      }
    break;
  }
}

// void updateSystem(){
//   switch (sysemState){
//     case RESET:
//     break;
//     case ERROR:
//     break;
//     case BUILDING:
//     break;
//     case OK:
//     break;
//   }
// }