// Entradas del encoder rotatorio 
#define CLK 2
#define DT 3
#define LED_PIN_0 4
#define LED_PIN_1 5
#define LED_PIN_2 6
#define LED_PIN_3 7
#define LED_PIN_4 8

int counter = 0;
int id = 9;
int currentStateCLK;
int lastStateCLK;

void setup() {
	// Configurar los pines como entradas
	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);

  // Configurar los led pins como salidas
  pinMode(LED_PIN_0,OUTPUT);
  pinMode(LED_PIN_1,OUTPUT);
  pinMode(LED_PIN_2,OUTPUT);
  pinMode(LED_PIN_3,OUTPUT);
  pinMode(LED_PIN_4,OUTPUT);

	// Configurar el Monitor serial 
	Serial.begin(9600);

	// leer el estado inicial del CLK
	lastStateCLK = digitalRead(CLK);
  powerLed(counter);
	
	// LLamar a updateEncoder() cuando un high o un low haya cambiado 
	// sobre la interrupcion 0 (pin 2), o interrupcion 1 (pin 3)
	attachInterrupt(0, updateEncoder, CHANGE);
	attachInterrupt(1, updateEncoder, CHANGE);
}

void loop() {
    //puede Crear su propia configuración de código aquí
}

void updateEncoder(){
	// leer el estado actual del CLK
	currentStateCLK = digitalRead(CLK);

// Si el ultimo estado actual del CLK es distinto, entonces ocurrió un pulso 
// reacciona solo a un cambio de estado para evitar un conteo doble
	if (currentStateCLK != lastStateCLK && currentStateCLK == 1){

// si el estado del DT es diferente que el estado del CLK 
// entonces el encoder está rotando en sentido antihorario CCW asi que decrementa		
    if (digitalRead(DT) != currentStateCLK) {
      if (counter > 0)
			  counter --;
		} else {
// Encoder está rotando en sentido horario CW así que incrementa
			if (counter < 4)
        counter ++;
		}
    powerLed(counter);
	}

	// guarda el ultimo estado del CLK 
	lastStateCLK = currentStateCLK;
}

// Enciende la led que corresponde
void powerLed(int counter){
  int ledPin = counter + 4;

  for(int i = 4; i < 9; i++){
      if(i!=ledPin){
       digitalWrite(i,LOW);
     }else{
       digitalWrite(i,HIGH);
     }
  }
}

void serialEvent(){
  String message = Serial.readStringUntil('!');
  // Serial.print("Input: ");
  // Serial.println(message);

  if (message[0] == '*'){
    String counter = counterWagon(message[1]);
    Serial.print(counter);
  } else if (message[0] == '{'){
    if (message.length() == 1){
      Serial.print("X!");
    } else {
      String actions = message + generateString();
      Serial.print(actions);
    }
    
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
      id = -1;
      return "*9!";
  }
}

String counterToString(){
  switch (counter){
    case 0:
      return "0";
    case 1:
      return "1";
    case 2:
      return "2";
    case 3:
      return "3";
    case 4:
      return "4";
    default: 
      return "9";
  }
}

String generateString(){
  String loop = "%";
  loop += counterToString();
  loop += "}!";
  return loop;
}