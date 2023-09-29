int pin1 = A0;
int pin2 = A1; 
int pin3 = A2;
int pin4 = A3; 

const float vin = 3.3;
const float ref1 = 2000; // Ref (A0 y A2) = 2KΩ
const float ref2 = 100000; // Ref (A1 y A3) = 100KΩ
float r1 = 0;
float r2 = 0;
float r3 = 0;
float r4 = 0;

int id = 9;

void setup() {
  Serial.begin(9600);
  analogReference(EXTERNAL);
  delay(500);
}

void loop() {
  // Serial.print("R1 = ");
  // Serial.print(calculateResistance(pin1, ref2));
  // Serial.println("Ω");
  // Serial.print("R2 = ");
  // Serial.print(calculateResistance(pin2, ref2));
  // Serial.println("Ω");
  // Serial.print("R3 = ");
  // Serial.print(calculateResistance(pin3, ref1));
  // Serial.println("Ω");
  // Serial.print("R4 = ");
  // Serial.print(calculateResistance(pin4, ref1));
  // Serial.println("Ω");
  // delay(3000);
}

float calculateResistance(int pin, float ref){
  float valorAnalog = analogRead(pin);  // Lectura analógica
  float vout = (vin / 1024) * valorAnalog; // Calcula el voltaje
  return ((ref * vin) / vout) - ref;  // Calcula la resistencia
}

String codificarResistencia(float valor){
  if (valor > 900 && valor < 1100)
    return "a";
  return "x";
}

String addAction(String str, String action){
  str = str + action;
  return str;
}

void serialEvent(){
  String message = Serial.readStringUntil('!');
  // Serial.print("Input: ");
  // Serial.println(message);

  // if (message == "start"){
  //   String actions = "";
  //   r1 = calcularResistencia(pin1, ref1);
  //   actions = addAction(actions, codificarResistencia(r1));
  //   actions = addAction(actions, "!");
  //   Serial.print(actions);
  // }

  if (message[0] == '#'){
    String counter = counterWagon(message[1]);
    Serial.print(counter);
  } else if (message[0] == '$'){
    String actions = message + generateString();
    Serial.print(actions);
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
  r1 = calculateResistance(pin1, ref2);
  return encoderResistance(r1);
}

String encoderDirection(){
  r2 = calculateResistance(pin2, ref2);
  return encoderResistance(r2);
}

String encoderColor(){
  r3 = calculateResistance(pin3, ref1);
  return encoderResistance(r3);
}

String encoderSound(){
  r4 = calculateResistance(pin4, ref1);
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
  else if (r > 45000 && r < 51000)
    return "I";
  else   if (r > 70000 && r < 120000)
    return "J";
  else
    return "X";
}
