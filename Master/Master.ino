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

volatile String robotState = "standby";
volatile bool buttonState = false;
volatile bool sensorState = false;
volatile unsigned long buttonPressTime = 0;


// Variables de exploracion
bool ledState = false;
bool loopDetected = false;
String str = "";
int wagons = 0;
int iterations = 0;
const unsigned long intervalExplore = 500; 
unsigned long previousExplore = 0;
const unsigned long timeout = 100;

// Duración de cada recorrido
const unsigned long timeslice = 10000;

// Controladores
LedControl lc = LedControl(DIN, CLK, CS, 3);
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

byte smile[8] = {
  B00000000,
  B00000000,
  B00000000,
  B10000001,
  B01000010,
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
  
  setColor(0, 255, 0);
  
  // display setup
  int devices = lc.getDeviceCount();
  for(int address = 0; address < devices; address++) {
    lc.shutdown(address, false);
    lc.setIntensity(address, 1);
    lc.clearDisplay(address);
  }  
  open_eyes();
  smile_bot();

  servo1.attach(SERVO_1); 
  servo2.attach(SERVO_2); 

  delay(1500);
  turnOff();
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(SENSOR), sensorInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, CHANGE);
  delay(50);

  robotState = "standby";
}

void loop(){
  // Realiza exploracion
  unsigned long currentTime = millis();
  if (currentTime - previousExplore >= intervalExplore){
    previousExplore = currentTime;

    if(!loopDetected){
      setColor(255, 0, 0);

    }

    // str = Serial.readStringUntil('!');
    // wagons = counterToInteger(str[1]);

    // if (wagons == 0 || wagons == 9 || wagons == 1){
    //   robotState = "error";
    //   return;
    // }
  }

  Serial.flush();


  // if (robotState == "counting") {
  //   Serial.print("#0!");   // Inicia el contador en 0

  //   while (!Serial.available()) {
  //     // agregar condicional de tiempo de espera
  //   }

  //   str = Serial.readStringUntil('!');
  //   wagons = counterToInteger(str[1]);

  //   if (wagons == 0 || wagons == 9 || wagons == 1){
  //     robotState = "error";
  //     return;
  //   }

  //   robotState = "reading"; 

  // } else if (robotState == "reading") {
  //   Serial.print("$!");   // Inicia lectura de acciones

  //   while (!Serial.available()) {
  //     // agregar condicional de tiempo de espera
  //   }

  //   str = Serial.readStringUntil('!');

  //   if (str.length() == 3){
  //     robotState = "error";
  //     return;
  //   }

  //   parseMessage(str);

  //   if (list == NULL){
  //     robotState = "error";
  //     return;
  //   }
    
  //   robotState = "playing";

  //   String prueba = "cadena generada: " + str;
  //   Serial.print(prueba);
  //   // Se ejecuta el primer vagón

  // } else if (robotState == "playing") {

  // } else if (robotState == "pause") {

  // }
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

    Puzzle *newNode = new Puzzle{slave, eyes, direction, color, sound, timeslice, NULL};

    if (list == NULL)
      list = newNode;
    else
      currentPuzzle->next = newNode;

    currentPuzzle = newNode;
  }
  i++;
  iterations = loopToInteger(actions[i]);
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

void smile_bot(){
  for (int i = 0; i < 8; i++) {
    lc.setRow(2,i,smile[i]);
  }
}