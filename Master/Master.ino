const int BOTON = 2; 
const int LED = 5;

volatile String robotState;
volatile bool buttonState;
volatile unsigned long buttonPressTime = 0;

bool isReading;
bool isPlaying;

String actions;

void setup(){
  actions = "";
  pinMode(BOTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(BOTON), buttonPressed, CHANGE);
  delay(50);

  robotState = "standby";
  buttonState = false;
  isPlaying = false;
  isReading = false;
}

void loop(){
  if (robotState == "reading" && !isReading) {
    isReading = true;
    Serial.print("start!");
    while (!Serial.available()) {}
    robotState = "playing";
    actions = Serial.readStringUntil('!');

    if (actions == "a")
      ledActionA();
    else 
      ledActionX();
    
    robotState = "standby";
    isReading = false;
    Serial.flush();
    actions = "";
  }
}

void buttonPressed() {
  buttonState = digitalRead(BOTON);
  delay(50);

  if (buttonState){
    if(robotState == "standby"){
      robotState = "reading";
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

void ledActionA(){
  for (int i = 0; i < 3; i++){
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    delay(1000);
  }
}

void ledActionX(){
  for (int i = 0; i < 5; i++){
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(100);
  }
}
