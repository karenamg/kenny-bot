#define PIN_1 A0
#define PIN_2 A1 
#define PIN_3 A2
#define PIN_4 A3 
#define LED_1 4
#define LED_2 5
#define LED_3 6
#define LED_4 7    
#define LED_R 9    
#define LED_G 10   
#define LED_B 11   

enum BotState {
  STANDBY,
  READING,
  PLAYING,
  CURRENT,
  PAUSED,
  FINISHING
};

BotState robotState = STANDBY;
unsigned long startTime = 0;
unsigned long startReadTime = 0;
const unsigned long readTimeSlice = 750;
bool isReaded = false;
int id = 9;

const float vin = 3.3;
const float ref1 = 2000; // Ref (A0 y A1) = 2KΩ
const float ref2 = 1000; // Ref (A2 y A3) = 1KΩ
float r1 = 0;
float r2 = 0;
float r3 = 0;
float r4 = 0;

bool prevLedState1 = false;
bool prevLedState2 = false;
bool prevLedState3 = false;
bool prevLedState4 = false;

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

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  Serial.begin(9600);
  analogReference(EXTERNAL);
  startTime = millis();
  delay(4);
}

void loop() {
  if (millis() - startTime >= 100){
    refreshLedIndicator();
    startTime = millis();
  }

  updateRGB();
}

float calculateResistance(int pin, float ref){
  float valorAnalog = analogRead(pin);  // Lectura analógica
  if(valorAnalog == 0)
    return 0;

  float vout = (vin / 1024) * valorAnalog; // Calcula el voltaje
  return ((ref * vin) / vout) - ref;  // Calcula la resistencia
}

String addAction(String str, String action){
  str = str + action;
  return str;
}

void serialEvent(){
  String message = Serial.readStringUntil('!');

  if (message[0] == '#'){         // message of count
    String counter = counterWagon(message[1]);
    Serial.print(counter);
  } else if (message[0] == '$'){  // message of resistance's read
    String actions = message + generateString();
    Serial.print(actions);
  } else if (message[0] == '*'){  // message of update robot state
    updateRobotState(message);
    if(robotState == READING){
      startReadTime = millis();
      isReaded = false;
    }
    Serial.print(message + "!");
    setRGB();
  }

  Serial.flush();
}

String counterWagon(char i){
  switch (i){
    case '0':
      id = 1;
      return "#1!";
    case '1':
      id = 2;
      return "#2!";
    case '2':
      id = 3;
      return "#3!";
    case '3':
      id = 4;
      return "#4!";
    case '4':
      id = 5;
      return "#5!";
    case '5':
      id = 6;
      return "#6!";
    default: 
      id = 9;
      return "#9!";
  }
}

String idToString(){
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

String generateString(){
  String wagon = idToString();
  wagon += encoderEyes();
  wagon += encoderDirection();
  wagon += encoderColor();
  wagon += encoderSound();
  wagon += "!";
  return wagon;
}

String encoderEyes(){
  r1 = calculateResistance(PIN_1, ref2);
  return encoderResistance(r1);
}

String encoderDirection(){
  r2 = calculateResistance(PIN_2, ref2);
  return encoderResistance(r2);
}

String encoderColor(){
  r3 = calculateResistance(PIN_3, ref1);
  return encoderResistance(r3);
}

String encoderSound(){
  r4 = calculateResistance(PIN_4, ref1);
  return encoderResistance(r4);
}

String encoderResistance(float r){
  if (r > 50 && r < 500)
    return "A";
  else if (r > 650 && r < 1500)
    return "B";
  else if (r > 1600 && r < 2700)
    return "C";
  else if (r > 4000 && r < 5000)
    return "D";
  else if (r > 5000 && r < 6500)
    return "E";
  else if (r > 9000 && r < 11500)
    return "F";
  else if (r > 11501 && r < 13500)
    return "G";
  else if (r > 18000 && r < 24000)
    return "H";
  else if (r > 30000 && r < 60000)
    return "I";
  else   if (r > 65000 && r < 150000)
    return "J";
  else
    return "X";
}

void setColor(int R, int G, int B) {
  analogWrite(LED_R, R);
  analogWrite(LED_G, G);
  analogWrite(LED_B, B);
}

void setColor(String color) {
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

  analogWrite(LED_R, red_blink);
  analogWrite(LED_G, green_blink);
  analogWrite(LED_B, blue_blink);
}

void turnOff() {
  setColor(0, 0, 0);
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

void finishBlinkEffect(){
  blinkStartTime = 0;
  turnOff(); // Apagar el LED al finalizar el efecto
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

void refreshLedIndicator(){
  bool ledState1, ledState2, ledState3, ledState4;

  if (encoderResistance(calculateResistance(PIN_1, ref2)) != "X") ledState1 = true; else ledState1 = false;

  if (encoderResistance(calculateResistance(PIN_2, ref2)) != "X") ledState2 = true; else ledState2 = false;
  
  if (encoderResistance(calculateResistance(PIN_3, ref1)) != "X") ledState3 = true; else ledState3 = false;

  if (encoderResistance(calculateResistance(PIN_4, ref1)) != "X") ledState4 = true; else ledState4 = false;
  
  if (prevLedState1 != ledState1){
    digitalWrite(LED_1, ledState1);
    prevLedState1 = ledState1;
  }

  if (prevLedState2 != ledState2){
    digitalWrite(LED_2, ledState2);
    prevLedState2 = ledState2;
  }

  if (prevLedState3 != ledState3){
    digitalWrite(LED_3, ledState3);
    prevLedState3 = ledState3;
  }

  if (prevLedState4 != ledState4){
    digitalWrite(LED_4, ledState4);
    prevLedState4 = ledState4;
  }  
}

void setRGB(){
  switch(robotState){
    case PLAYING:
      setColor(findColor(encoderColor()[0]));
    break;
    case CURRENT:
      startBlinkEffect(0,600,findColor(encoderColor()[0]));
    break;
    case FINISHING:
      startRainbowEffect(3000,8);
    break;
    case READING:
    case STANDBY:
    case PAUSED:
      turnOff();
    break;
  }
}

void updateRGB(){
  switch(robotState){
    case READING:
      if (!isReaded)
        if (millis() - startReadTime >= readTimeSlice * id){
          setColor(findColor(encoderColor()[0]));
          startReadTime = 0;
          isReaded = true;
        }
    break;
    case CURRENT:
      updateBlinkEffect(true);
    break;
    case FINISHING:
      updateRainbowEffect();
    break;
  }
}

void updateRobotState(String message){
  if (message == "*STANDBY")
    robotState = STANDBY;
  else if (message == "*READING")
    robotState = READING;
  else if (message == ("*"+idToString()))
    robotState = CURRENT;
  else if (message == "*PAUSED")
    robotState = PAUSED;
  else if (message == "*FINISHING")
    robotState = FINISHING;
  else 
    robotState = PLAYING;
}