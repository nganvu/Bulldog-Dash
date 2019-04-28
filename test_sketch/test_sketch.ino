
const char instruction[] = 
"______PRESS_BUTTON_TO_START______";
const char welcome[] = 
"_WELCOME_TO_BULLDOG_DASH______";
const char obstacles[] =
"______HELLO_WORLD______";

int MAX_OBSTACLE_INDEX;
char MAX_INST_INDEX;
char MAX_WELCOME_INDEX;

void setup() {
  
  // For Serial Monitor.
  Serial.begin(9600);

  MAX_OBSTACLE_INDEX = strlen(obstacles) - 6;
  MAX_INST_INDEX = strlen(instruction) - 6; 
  MAX_WELCOME_INDEX = strlen(welcome) - 6; 
 
  Serial.println(MAX_OBSTACLE_INDEX);
  Serial.println(int(MAX_INST_INDEX));
  Serial.println(int(MAX_WELCOME_INDEX));
  Serial.flush();
  Serial.println("END");

}

void loop() {
  // put your main code here, to run repeatedly:

}
