// All characters are fixed as a 5x7 matrix.
// #include <string.h>
// #include "font5x7.h"

#include <avr/pgmspace.h>
#include <RGBmatrixPanel.h>

#define CLK  8
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

// Timings for scrolling text
#define GAME_MILLIS 150
#define START_MILLIS 75

// Sprites include bulldog and heart.
#define SPRITE_HEIGHT (7)
#define SPRITE_WIDTH (7)

// Chars include letters and digits.
#define CHAR_HEIGHT (7)
#define CHAR_WIDTH (5)

// For jumping direction.
#define UP true
#define DOWN false

// For bulldog platform (where it is jumping from).
#define OBSTACLE_TOP (CHAR_HEIGHT)
#define GROUND (0)

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

uint16_t color;
#define WHITE (matrix.Color333(7, 7, 7))
#define RED (matrix.Color333(7, 0, 0))
#define BLUE (matrix.Color333(0, 0, 7))
#define PURPLE (matrix.Color333(7, 0, 7))

#define DISP_SIZE (32)
#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_FRONT_LEG_OFFSET (4)
#define BULLDOG_BACK_LEG_OFFSET (2)
#define BULLDOG_TOP_OFFSET (26)
#define JUMP_SEQUENCE_LENGTH (19)
#define LAND_ON_OBSTACLE_CHECK (11)
#define MAX_OBSTACLE_CYCLE (6)

//typedef struct gameState {
//  int bulldog_altitude;
//  int bulldog_max_altitude;
//  int bulldog_platform;
//  bool bulldog_jumping;
//  bool jumping_direction;
//  int lives;
//  int scores;
//  bool game_running;
//} gameState;

int bulldog_altitude;
int bulldog_max_altitude;
int bulldog_platform;
bool bulldog_jumping;
bool jumping_direction;
int lives;
int scores;
bool game_running;

// +1 because extra space needed for string null-terminator.
// 56 bytes
const char bulldog[SPRITE_HEIGHT][SPRITE_WIDTH + 1] PROGMEM = {
  "__W___W",
  "___WWW_",
  "___WWW_",
  "WBWBWW_",
  "_WBWW__",
  "_WBWW__",
  "__W_W__"
};

// 56 bytes
const char heart[SPRITE_HEIGHT][SPRITE_WIDTH + 1] PROGMEM = {
  "_RR_RR_",
  "RRRRRRR",
  "RRRRRRR",
  "RRRRRRR",
  "_RRRRR_",
  "__RRR__",
  "___R___",
};

// 130 bytes
const unsigned char LetterFont[] PROGMEM = {
  0x7E, 0x11, 0x11, 0x11, 0x7E,// A
  0x7F, 0x49, 0x49, 0x49, 0x36,// B
  0x3E, 0x41, 0x41, 0x41, 0x22,// C
  0x7F, 0x41, 0x41, 0x22, 0x1C,// D
  0x7F, 0x49, 0x49, 0x49, 0x41,// E
  0x7F, 0x09, 0x09, 0x01, 0x01,// F
  0x3E, 0x41, 0x41, 0x51, 0x32,// G
  0x7F, 0x08, 0x08, 0x08, 0x7F,// H
  0x00, 0x41, 0x7F, 0x41, 0x00,// I
  0x20, 0x40, 0x41, 0x3F, 0x01,// J
  0x7F, 0x08, 0x14, 0x22, 0x41,// K
  0x7F, 0x40, 0x40, 0x40, 0x40,// L
  0x7F, 0x02, 0x04, 0x02, 0x7F,// M
  0x7F, 0x04, 0x08, 0x10, 0x7F,// N
  0x3E, 0x41, 0x41, 0x41, 0x3E,// O
  0x7F, 0x09, 0x09, 0x09, 0x06,// P
  0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
  0x7F, 0x09, 0x19, 0x29, 0x46,// R
  0x46, 0x49, 0x49, 0x49, 0x31,// S
  0x01, 0x01, 0x7F, 0x01, 0x01,// T
  0x3F, 0x40, 0x40, 0x40, 0x3F,// U
  0x1F, 0x20, 0x40, 0x20, 0x1F,// V
  0x7F, 0x20, 0x18, 0x20, 0x7F,// W
  0x63, 0x14, 0x08, 0x14, 0x63,// X
  0x03, 0x04, 0x78, 0x04, 0x03,// Y
  0x61, 0x51, 0x49, 0x45, 0x43,// Z
};

// 50 bytes
const unsigned char DigitFont[] PROGMEM = {
  0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
  0x00, 0x42, 0x7F, 0x40, 0x00,// 1
  0x42, 0x61, 0x51, 0x49, 0x46,// 2
  0x21, 0x41, 0x45, 0x4B, 0x31,// 3
  0x18, 0x14, 0x12, 0x7F, 0x10,// 4
  0x27, 0x45, 0x45, 0x45, 0x39,// 5
  0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
  0x01, 0x71, 0x09, 0x05, 0x03,// 7
  0x36, 0x49, 0x49, 0x49, 0x36,// 8
  0x06, 0x49, 0x49, 0x29, 0x1E,// 9
};

#define BIG_SPRITE_HEIGHT (SPRITE_HEIGHT+4)
#define BIG_SPRITE_WIDTH  (SPRITE_WIDTH*2 + 1)
// +1 because extra space needed for string null-terminator.
// 112 bytes
const char big_bulldog[BIG_SPRITE_HEIGHT][BIG_SPRITE_WIDTH] PROGMEM = {
  "______WW____WW",
  "_______WWWWWW_",
  "W_______WWWWW_",
  "WWWWWWWWWWWW__",
  "WWWBWWWBWWWW__",
  "_WWWBWBWWWW___",
  "_WWWWBWWW_____",
  "__WWWBWWW_____",
  "__WWWWWWW_____",
  "__WW___WW_____",
  "__WW___WW_____"
};

// PROGMEM memory total: 292 + 112 + 32 + strlen(obstacles) 
//#define LINES 4
//#define MAX_LINE 7
//// 32 bytes
//const char welcome[LINES][MAX_LINE+1] PROGMEM = {
//  "WELCOME",
//  "__TO___",
//  "BULLDOG",
//  "_DASH__"
//};

//const char welcome[] PROGMEM = "WELCOME_TO_BULLDOG_DASH______";
const char welcome[] PROGMEM = "BOOLA__";
//char MAX_WELCOME_INDEX; // less than 255
char welcome_index; // less than 255
char welcome_cycle; // less than 255
// tested with strlen in another sketch
#define MAX_WELCOME_INDEX (24)
//int MAX_OBSTACLE_INDEX;

const char instruction[] PROGMEM = "______PRESS_BUTTON_TO_START_____";
// char MAX_INST_INDEX; // less than 255
char inst_index; // less than 255
char inst_cycle; // less than 255
// tested with strlen in another sketch but idk what the - 6 is for
#define MAX_INST_INDEX (27)
//int MAX_OBSTACLE_INDEX;

// tested with strlen in another sketch
#define MAX_OBSTACLE_INDEX (17)
//int MAX_OBSTACLE_INDEX;

const char obstacles[] PROGMEM =
"______HELLO_WORLD______";
char obstacle_index; // Current character at the bottom left corner.
int obstacle_cycle; // Each character takes up 5 columns, so it takes 5 cycles to move 1 character over.

unsigned long previous_millis;
unsigned long current_millis;
unsigned long delay_millis = START_MILLIS;

//gameState* my_game;
int cur_jumping_idx = 0;

/* This function places the current value of the heap and stack pointers in the
 * variables. You can call it from any place in your code and save the data for
 * outputting or displaying later. This allows you to check at different parts of
 * your program flow.
 * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
 * starts just above the static variables etc. and grows upwards. SP should always
 * be larger than HP or you'll be in big trouble! The smaller the gap, the more
 * careful you need to be. Julian Gall 6-Feb-2009.
 */
int * heapptr, * stackptr;
void check_mem() {
  stackptr = (int *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (int *)(SP);           // save value of stack pointer

  Serial.print(F("SP: ")); Serial.print((unsigned int) stackptr); Serial.print(F(" > HP: "));
  Serial.println((unsigned int) heapptr);
}

void setup() {
  
  // For Serial Monitor.
  Serial.begin(9600);
 
  // For LED Matrix.
  matrix.begin();

  check_mem();

  bulldog_altitude = 0;
  bulldog_max_altitude = 10;
  bulldog_platform = GROUND;
  bulldog_jumping = false;
  jumping_direction = UP;
  lives = 4;
  scores = 0;
  game_running = false;
  
  obstacle_index = 0;
  obstacle_cycle = 0;
  //MAX_OBSTACLE_INDEX = strlen_P(obstacles) - 6;

  inst_index = 0;
  inst_cycle = 0;
  //MAX_INST_INDEX = strlen_P(instruction) - 6; 

  welcome_index = 0;
  welcome_cycle = 0;
  //MAX_WELCOME_INDEX = strlen_P(welcome) - 6; 
  
  previous_millis = millis();

  check_mem();
  Serial.println(MAX_OBSTACLE_INDEX);
  Serial.println(MAX_INST_INDEX);
  Serial.println(MAX_WELCOME_INDEX);
  Serial.flush();
  Serial.println("END");

  matrix.fillScreen(0); // Clear the LED board.
//  displayStartScreen();
  displayBigBulldog();
//  color = PURPLE;
//  displayLineOfText(welcome, welcome_index, welcome_cycle, matrix.height() - CHAR_HEIGHT, 1);
  color = BLUE;
  displayLineOfText(welcome, welcome_index, welcome_cycle, matrix.height() - CHAR_HEIGHT, 0);
  displayLineOfText(welcome, welcome_index, welcome_cycle, 0, 3);
}

void loop() {
  unsigned long current_millis = millis();
  
// NO MYGAME
  if (current_millis - previous_millis >= delay_millis) {
    
    if(!game_running) {
      int read_val = Serial.read();
    
      // When the char 'A' is sent.
      // Change to be button
      if (read_val == 'A') {
        game_running = true;
        matrix.fillScreen(0);
        displayObstacles();
        displayBulldog();
        delay_millis = GAME_MILLIS;
      }
      else {
//        displayStartScreen();
//        updateInstCycle();
        //updateWelcomeCycle();
      }
    }
 
    else{
      int read_val = Serial.read();
      
      // When the char 'A' is sent.
      // Change to be button
      if (read_val == 'A' && bulldog_jumping == false) {
        bulldog_jumping = true;
        jumping_direction = UP;
      }
  
      unsigned char front_leg_char = getCharOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_FRONT_LEG_OFFSET);
      unsigned char back_leg_char = getCharOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_BACK_LEG_OFFSET);
      bool bulldog_on_char = (front_leg_char != '_') && (back_leg_char != '_');
  
      // Bulldog is jumping.
      if (bulldog_jumping == true) {
        if (jumping_direction == UP) {
          clearBulldogRow(bulldog_platform + bulldog_altitude);
          bulldog_altitude++;
          if (bulldog_altitude == bulldog_max_altitude) {
            jumping_direction = DOWN;
          }
        } else {
          clearBulldogRow(bulldog_platform + bulldog_altitude + (SPRITE_HEIGHT - 1));
          bulldog_altitude--;
          if (bulldog_platform == GROUND) {
            if (bulldog_altitude == OBSTACLE_TOP && bulldog_on_char) {
              bulldog_jumping = false;
              bulldog_platform = OBSTACLE_TOP;
              bulldog_altitude = 0;
              bulldog_max_altitude = 7;
            } else if (bulldog_altitude == 0) {
              bulldog_jumping = false;
            }
          }
          else if (bulldog_platform == OBSTACLE_TOP) {
            if (bulldog_altitude == 0) {
              bulldog_jumping = false;
            }
          }
        }
      }
  
      // Bulldog is not jumping.
      else {
        if (bulldog_platform == OBSTACLE_TOP && !bulldog_on_char) {
          bulldog_jumping = true;
          jumping_direction = DOWN;
          bulldog_platform = GROUND;
          bulldog_altitude = OBSTACLE_TOP;
          bulldog_max_altitude = 10;
          clearBulldogRow(bulldog_platform + bulldog_altitude + (SPRITE_HEIGHT - 1));
          bulldog_altitude--;
        }
      }
      
//      Serial.print(bulldog_platform);
//      Serial.print(bulldog_altitude);
//      Serial.println(bulldog_platform + bulldog_altitude);
      
      displayObstacles();
      displayBulldog();
      displayLives();
      obstacle_cycle++;
      if (obstacle_cycle >= MAX_OBSTACLE_CYCLE) {
        obstacle_cycle = 0;
        obstacle_index++;
        // For now, restart the game when out of characters.
        if (obstacle_index >= MAX_OBSTACLE_INDEX) {
          obstacle_index = 0;
          lives = 4;
        }
      }
    }
  previous_millis = current_millis;
  }
}

//void updateWelcomeCycle(){
//  welcome_cycle++;
//  if (welcome_cycle >= MAX_OBSTACLE_CYCLE) {
//    welcome_cycle = 0;
//    welcome_index++;
//    // keep looping
//    if (welcome_index >= MAX_WELCOME_INDEX) {
//      welcome_index = 0;
//    }
//  }
//}

void updateInstCycle(){
  inst_cycle++;
  if (inst_cycle >= MAX_OBSTACLE_CYCLE) {
    inst_cycle = 0;
    inst_index++;
    // keep looping
    if (inst_index >= MAX_INST_INDEX) {
      inst_index = 0;
    }
  }
}

unsigned char getCharOfColumn(int column) {
  return (unsigned char) pgm_read_byte(&(obstacles[obstacle_index + (int) ((column + obstacle_cycle) / 6)]));
}

unsigned char getCharColumnOfColumn(int column) {
  return (column + obstacle_cycle) % 6;
}

// Display the obstacles, except for things within the square containing the bulldog.
void displayObstacles() {
  unsigned char current_char;
  int current_char_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  color = PURPLE;
  for (int col = 0; col < 32; col++) {
    current_char =  getCharOfColumn(col);
    current_char_column = getCharColumnOfColumn(col);
    if (current_char == '_' || current_char_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + current_char_column]));
    }
    for (int row = 0; row < 7; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (isInBulldogFrame((CHAR_HEIGHT - 1) - row, col)) {
        continue;
      }
      if (current_bit == 0x1) {
        matrix.drawPixel((CHAR_HEIGHT - 1) - row, col, color);
      } else {
        matrix.drawPixel((CHAR_HEIGHT - 1) - row, col, 0);
      }
    }
  }
}


// Display the bulldog, as well as anything that is within the square containing the bulldog.
void displayBulldog() {
  unsigned char current_char;
  int current_char_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  unsigned char bulldog_pixel;
  for (int col = 0; col < SPRITE_WIDTH; col++) {
    current_char =  getCharOfColumn(BULLDOG_LEFT_OFFSET + col);
    current_char_column = getCharColumnOfColumn(BULLDOG_LEFT_OFFSET + col);
    if (current_char == '_' || current_char_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + current_char_column]));
    }
    
    for (int row = 0; row < SPRITE_HEIGHT; row++) {
      if (row >= (bulldog_platform + bulldog_altitude)) {
        current_bit = current_column_bitmap & 0x1;
        current_column_bitmap >>= 1;
      } else {
        current_bit = 0;
      }
      bulldog_pixel = (unsigned char) pgm_read_byte(&(bulldog[row][col]));
      if (bulldog_pixel == '_') {
        if (current_bit == 0x1) {
          color = PURPLE;
        } else {
          color = 0;
        }
      } else if (bulldog_pixel == 'W') {
        color = WHITE;
      } else if (bulldog_pixel == 'B') {
        color = BLUE;
      }
      matrix.drawPixel((SPRITE_HEIGHT - 1) - row + (bulldog_platform + bulldog_altitude), BULLDOG_LEFT_OFFSET + col, color);
    }
  }  
}

bool isInBulldogFrame(int row, int col) {
  bool colInFrame = (col >= BULLDOG_LEFT_OFFSET && col < BULLDOG_LEFT_OFFSET + SPRITE_WIDTH);
  bool rowInFrame = (row >= bulldog_platform + bulldog_altitude && row < bulldog_platform + bulldog_altitude + SPRITE_HEIGHT);
  return (colInFrame && rowInFrame);
}

void clearBulldogRow(int row) {
  for (int col = 0; col < SPRITE_WIDTH; col++) {
    matrix.drawPixel(row, BULLDOG_LEFT_OFFSET + col, 0);
  }
}

void displayLives() {

  // Draw number of lives.
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  for (int col = 0; col < CHAR_WIDTH; col++) {
    current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[(lives * 5) + col]));
    for (int row = 0; row < CHAR_HEIGHT; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (current_bit == 0x1) {
        color = RED;
      } else {
        color = 0;
      }
      matrix.drawPixel((matrix.height() - 1) - row, col, color);
    }
  }

  // Draw heart.
  unsigned char heart_pixel;
  for (int col = 0; col < SPRITE_WIDTH; col++) {
    for (int row = 0; row < SPRITE_HEIGHT; row++) {
      heart_pixel = (unsigned char) pgm_read_byte(&(heart[row][col]));
      if (heart_pixel == '_') {
        color = 0;
      } else if (heart_pixel == 'R') {
        color = RED;
      }
      matrix.drawPixel((matrix.height() - 1) - row, CHAR_WIDTH + 1 + col, color);
    }
  }
}

// message must be string in PROGMEM
unsigned char getCharOfColumnGeneral(int column, const char *message, int index, int cycle) {
  return (unsigned char) pgm_read_byte(&(message[index + (int) ((column + cycle) / 6)]));
}

unsigned char getCharColumnOfColumnGeneral(int column, int cycle) {
  return (column + cycle) % 6;
}

// message must be string in PROGMEM with only alphabetic characters
void displayLineOfText(const char *message, int index, int cycle, int row_offset, int col_offset) {
  unsigned char current_char;
  int current_char_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  check_mem();
  for (int col = 0; col < 32; col++) {
    current_char =  getCharOfColumnGeneral(col, message, index, cycle);//getCharOfColumnGeneral(col, message, index, cycle);
    current_char_column = getCharColumnOfColumnGeneral(col, cycle);
    if (current_char == '_' || current_char_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + current_char_column]));
    }
    for (int row = 0; row < 7; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (current_bit == 0x1) {
        matrix.drawPixel(row_offset + (CHAR_HEIGHT - 1) - row, col + col_offset, color);
      } else {
        matrix.drawPixel(row_offset + (CHAR_HEIGHT - 1) - row, col + col_offset, 0);
      }
    }
  }
}

void displayBigBulldog() {
  // Draw big bulldog.
  unsigned char pixel;
  for (int col = 0; col < BIG_SPRITE_WIDTH; col++) {
    for (int row = 0; row < BIG_SPRITE_HEIGHT; row++) {
      pixel = (unsigned char) pgm_read_byte(&(big_bulldog[row][col]));
      if (pixel == '_') {
        color = 0;
      } else if (pixel == 'W') {
        color = WHITE;
      } else if (pixel == 'B') {
        color = BLUE;
      }
      matrix.drawPixel((matrix.height() / 2) + (BIG_SPRITE_HEIGHT / 2) - row, ((matrix.width() / 2) - (BIG_SPRITE_WIDTH / 2))  + 1 + col, color);
    }
  }
}

// display start screen
void displayStartScreen() {

  color = BLUE;
  displayLineOfText(instruction, inst_index, inst_cycle, 0, 0);

}
//
////  color = PURPLE;
////  displayLineOfText(welcome, welcome_index, welcome_cycle, matrix.height() - CHAR_HEIGHT);
//
////  color = BLUE;
////  displayLineOfText(instruction, inst_index, inst_cycle, 0);
//
//  // Draw big bulldog.
//  unsigned char pixel;
//  for (int col = 0; col < BIG_SPRITE_WIDTH; col++) {
//    for (int row = 0; row < BIG_SPRITE_HEIGHT; row++) {
//      pixel = (unsigned char) pgm_read_byte(&(big_bulldog[row][col]));
//      if (pixel == '_') {
//        color = 0;
//      } else if (pixel == 'W') {
//        color = WHITE;
//      } else if (pixel == 'B') {
//        color = BLUE;
//      }
//      matrix.drawPixel((matrix.height() / 2) + (BIG_SPRITE_HEIGHT / 2) - row, ((matrix.width() / 2) - (BIG_SPRITE_WIDTH / 2))  + 1 + col, color);
//    }
//  }
//}
//
//// Display the program start message.
//unsigned char getCharOfColumnInst(int column) {
//  return (unsigned char) pgm_read_byte(&(instruction[inst_index + (int) ((column + inst_cycle) / 6)]));
//}
//
//unsigned char getCharColumnOfColumnInst(int column) {
//  return (column + inst_cycle) % 6;
//}
//
//void displayInstructions() {
//  unsigned char current_char;
//  int current_char_column;
//  unsigned char current_column_bitmap;
//  unsigned char current_bit;
//  color = BLUE;
//  for (int col = 0; col < 32; col++) {
//    current_char =  getCharOfColumn(col);
//    current_char_column = getCharColumnOfColumn(col);
//    if (current_char == '_' || current_char_column == 5) {
//      current_column_bitmap = 0;
//    } else {
//      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + current_char_column]));
//    }
//    for (int row = 0; row < 7; row++) {
//      current_bit = current_column_bitmap & 0x1;
//      current_column_bitmap >>= 1;
//      if (current_bit == 0x1) {
//        matrix.drawPixel((CHAR_HEIGHT - 1) - row, col, color);
//      } else {
//        matrix.drawPixel((CHAR_HEIGHT - 1) - row, col, 0);
//      }
//    }
//  }
//}
