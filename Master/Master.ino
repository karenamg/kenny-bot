#include "LedControl.h"
#include "Servo.h"
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Pin Definitions
#define BUTTON 2
#define SENSOR 3
#define TX_DF 4
#define RX_DF 5
#define BUSSY 6
#define CLK 7
#define CS 8
#define DIN 9
#define SERVO_1 10
#define SERVO_2 11
#define LED_R A0
#define LED_G A1
#define LED_B A2

// Estructura de cada vagón programado
struct Puzzle {
  int slave;
  char eyes; 
  char direction; 
  char color; 
  char sound; 
  unsigned long timeout;
  Puzzle *next;
};

// Variables globales
Puzzle *list = NULL;
Puzzle *currentPuzzle = NULL;

volatile String robotState = "standby";
volatile bool buttonState = false;
volatile bool sensorState = false;
volatile unsigned long buttonPressTime = 0;


String str = "";
int wagons = 0;
int counter = 0;
int index = 0;

// Duración de cada recorrido
const unsigned long timeout = 10000;

// Controladores
LedControl lc = LedControl(DIN, CLK, CS, 2);
Servo servo1;
Servo servo2;
SoftwareSerial portAudio(TX_DF, RX_DF); // RX, TX
DFRobotDFPlayerMini dfPlayer;

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
  
  analogWrite(LED_R, 0);
  analogWrite(LED_G, 0);
  analogWrite(LED_B, 255);
  
  // display setup
  int devices = lc.getDeviceCount();
  for(int address = 0; address < devices; address++) {
    lc.shutdown(address, false);
    lc.setIntensity(address, 1);
    lc.clearDisplay(address);
  }  
  open_eyes();

  servo1.attach(SERVO_1); 
  servo2.attach(SERVO_2); 

  servo1.write(110); 
  servo2.write(70);  
  delay(5000);  
  servo1.write(90); 
  servo2.write(90);  

  Serial.begin(9600);
  // Set the baud rate for the SerialSoftware object
  //portAudio.begin(9600);

  attachInterrupt(digitalPinToInterrupt(SENSOR), sensorInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, CHANGE);
  delay(50);

  robotState = "standby";
}

void loop(){
  if (robotState == "counting") {
    Serial.print("*0!");   // Inicia el contador en 0

    while (!Serial.available()) {
      // agregar condicional de tiempo de espera
    }

    str = Serial.readStringUntil('!');
    wagons = counterToInteger(str[1]);
    str = "";

    if (wagons == 0 || wagons == 9 || wagons == 1){
      robotState = "error";
      return;
    }

    robotState = "reading"; 

  } else if (robotState == "reading") {
    Serial.print("{!");   // Inicia lectura de acciones

    while (!Serial.available()) {
      // agregar condicional de tiempo de espera
    }

    str = Serial.readStringUntil('!');

    if (str.length() == 4){
      robotState = "error";
      return;
    }

    parseMessage(str);

    if (list == NULL){
      robotState = "error";
      return;
    }
    
    robotState = "playing";
    // Se ejecuta el primer vagón

  } else if (robotState == "playing") {

  } else if (robotState == "pause") {

  }
}

void buttonPressed() {
  buttonState = digitalRead(BUTTON);
  delay(50);

  if (buttonState){
    if(robotState == "standby"){
      robotState = "counting";
    } else if (robotState == "paused"){
      if ((millis() - buttonPressTime) < 5000){
        robotState = "playing";
        // se reanudan las acciones del robot donde se pausaron
      } else {
        robotState = "standby";
        // se eliminan las acciones en cola
      }
      buttonPressTime = 0;
    }
  } else {
    if (robotState == "playing"){
      robotState = "paused";
      // se envía mensaje de pausado a esclavos
      // se pausan todas las acciones y se guarda el progreso
      return;
    } else if (robotState == "paused"){
      buttonPressTime = millis();
    }
  }
}

void sensorInterrupt() {
  sensorState = digitalRead(SENSOR);
  delay(50);

  // Insertar logica para pausar al detectar obstaculos
  // if (sensorState == HIGH)
  //   Serial.println("Estamos fuera de la línea oscura");
  // else
  //   Serial.println("Zona oscura");
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

    Puzzle *newNode = new Puzzle{slave, eyes, direction, color, sound, timeout, NULL};

    if (list == NULL)
      list = newNode;
    else
      currentPuzzle->next = newNode;

    currentPuzzle = newNode;
  }
  i++;
  counter = loopToInteger(actions[i]);
}

// Funciones para ojos
void open_eyes(){
  for (int i = 0; i < 8; i++) {
    lc.setRow(0,i,eye_open[i]);
    lc.setRow(1,i,eye_open[i]);
  }
}