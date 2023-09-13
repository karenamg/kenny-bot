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

int id;

void setup() {
  Serial.begin(9600);
  analogReference(EXTERNAL);
  delay(500);

  id = 9;
  r1 = 0;
  r2 = 0;
  r3 = 0;
  r4 = 0;
}

void loop() {
  // Serial.print("R1 = ");
  // Serial.print(calculateResistance(pin1, ref1));
  // Serial.println("Ω");
  // Serial.print("R2 = ");
  // Serial.print(calculateResistance(pin2, ref2));
  // Serial.println("Ω");
  // Serial.print("R3 = ");
  // Serial.print(calculateResistance(pin3, ref1));
  // Serial.println("Ω");
  // Serial.print("R4 = ");
  // Serial.print(calculateResistance(pin4, ref2));
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

  if (message[0] == '*'){
    String counter = counterWagon(message[1]);
    Serial.print(counter);
  } else if (message[0] == '{'){
    String actions = message + generateString();
    Serial.print(actions);
  }

  Serial.flush();
}

String counterWagon(char i){
  switch (i){
    case '0':
      id = 1;
      return "*1!";
    case '1':
      id = 2;
      return "*2!";
    case '2':
      id = 3;
      return "*3!";
    case '3':
      id = 4;
      return "*4!";
    case '4':
      id = 5;
      return "*5!";
    case '5':
      id = 6;
      return "*6!";
    default: 
      id = 9;
      return "*9!";
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
  wagon += "[";
  wagon += encoderEyes();
  wagon += ",";
  wagon += encoderAnimal();
  wagon += ",";
  wagon += encoderDirection();
  wagon += "]!";
  return wagon;
}

String encoderEyes(){
  r1 = calculateResistance(pin1, ref2);

  if (r1 > 70000 && r1 < 120000)
    return "A";
  else if (r1 > 45000 && r1 < 51000)
    return "B";
  else if (r1 > 18000 && r1 < 24000)
    return "C";
  else
    return "X";
}

String encoderDirection(){
  r2 = calculateResistance(pin2, ref2);

  if (r2 > 70000 && r2 < 120000)
    return "J";
  else if (r2 > 45000 && r2 < 51000)
    return "K";
  else if (r2 > 18000 && r2 < 24000)
    return "L";
  else if (r2 > 9000 && r2 < 15000)
    return "M";
  else if (r2 > 3500 && r2 < 7000)
    return "N";
  else
    return "X";
}

String encoderColor(){
  r3 = calculateResistance(pin3, ref1);

  if (r3 > 50 && r3 < 500)
    return "O";
  else if (r3 > 650 && r3 < 1500)
    return "P";
  else if (r3 > 1600 && r3 < 2700)
    return "Q";
  else if (r3 > 3700 && r3 < 5500)
    return "R";
  else if (r3 > 9000 && r3 < 13000)
    return "S";
  else if (r3 > 18000 && r3 < 24000)
    return "P";
  else
    return "X";
}

String encoderAnimal(){
  r4 = calculateResistance(pin4, ref1);

  if (r4 > 50 && r4 < 500)
    return "D";
  else if (r4 > 650 && r4 < 1500)
    return "E";
  else if (r4 > 1600 && r4 < 2700)
    return "F";
  else if (r4 > 3700 && r4 < 5500)
    return "G";
  else if (r4 > 9000 && r4 < 13000)
    return "H";
  else if (r4 > 18000 && r4 < 24000)
    return "I";
  else
    return "X";
}
