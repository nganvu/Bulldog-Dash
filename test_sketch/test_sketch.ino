
const char freshman[] PROGMEM  = "______CAMP-YALE_____MIDTERMS____IMPOSTOR-SYNDROME____SCREW___";
const char sophomore[] PROGMEM = "______PAPER_____PAPER____SOPHOMORE-SLUMP____PAPER___";
const char junior[] PROGMEM    = "______CPSC-323_____INTERNSHIP____SPRING-FLING____FINALS___";
const char senior[] PROGMEM    = "______INTERVIEWS_____FEB-CLUB____EXISTENTIAL-DREAD____THESIS___";
const char * obstacles[] = {freshman, sophomore, junior, senior};
char MAX_OBSTACLE_INDEX[4];


void setup() {
  
  // For Serial Monitor.
  Serial.begin(9600);

  for(int i = 0; i < 4; i++)
    MAX_OBSTACLE_INDEX[i] = strlen_P(obstacles[i]) - 6;

  for(int i = 0; i < 4; i++)
    Serial.println(int(MAX_OBSTACLE_INDEX[i]));
 
}

void loop() {
  // put your main code here, to run repeatedly:

}
