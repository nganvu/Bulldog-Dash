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

#define BUTTON_PIN 13

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
#define GREEN (matrix.Color333(3, 7, 0))
#define LIGHT_GREEN (matrix.Color333(2, 7, 2))
#define BLUE (matrix.Color333(0, 0, 7))
#define LIGHT_BLUE (matrix.Color333(2, 2, 7))
#define YELLOW (matrix.Color333(7, 3, 0))
#define PURPLE (matrix.Color333(7, 0, 7))
#define YALE_PURPLE (matrix.Color333(3, 0, 7))

#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_FRONT_LEG_OFFSET (4)
#define BULLDOG_BACK_LEG_OFFSET (2)
#define BULLDOG_TOP_OFFSET (26)

#define CREDITS 4 //(36)

typedef struct gameState {
  bool active;
  int lives;
  int scores;
  int time_step;
  int year;
  int bulldog_altitude;
  int bulldog_max_altitude;
  int bulldog_platform;
  bool bulldog_jumping;
  bool bulldog_jumping_over_gap;
  bool bulldog_jumping_direction;
} gameState;

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

#define BIG_SPRITE_HEIGHT (SPRITE_HEIGHT+4)
#define BIG_SPRITE_WIDTH  (SPRITE_WIDTH*2 + 1)

// 165 bytes
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

// 165 bytes
const char grad_decal[BIG_SPRITE_HEIGHT][BIG_SPRITE_WIDTH] PROGMEM = {
  "P______P_____P",
  "_B_____P____B_",
  "__B____B___B__",
  "___BP___PPB___",
  "___PBBPBPBP__W",
  "PW__BPPBBBP___",
  "___WBWBPPP___",
  "___BWPPB_PB___",
  "__B________B__",
  "_B____B_____B_",
  "P_____B______P"
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

//const char obstacles[] PROGMEM =
//  "______HELLO-WORLD______";
// 55 bytes
const char obstacles[] PROGMEM =
  "______CAMP-YALE_____MIDTERMS____IMPOSTOR-SYNDROME____SCREW___";
int obstacle_index; // Current character at the bottom left corner.
char obstacle_cycle; // Each character takes up 5 columns, so it takes 5 cycles to move 1 character over.
// must be less than 254 chars long, may need to change to int
char MAX_OBSTACLE_INDEX;

#define STATIC_INDEX 0
#define STATIC_CYCLE 0

// 7 bytes each = 35 total
const char congrats[] PROGMEM = "_PASS__";
const char dropout[] PROGMEM = "_FAIL__";
const char new_word[] PROGMEM = "_NEW___";
const char grad[] PROGMEM = "GRAD___";
const char welcome[] PROGMEM = "BOOLA__";
//char welcome_index = 0; // less than 255
//char welcome_cycle = 0; // less than 255
// currently not being reset
//const char welcome[] PROGMEM = "WELCOME_TO_BULLDOG_DASH______";
//const char welcome[] PROGMEM = "YALE__";

//const char instruction[] PROGMEM = "______PRESS_BUTTON_TO_START_____";
//char inst_index; // less than 255
//char inst_cycle; // less than 255
//// tested with strlen in another sketch but idk what the - 6 is for
//#define MAX_INST_INDEX (27)

//const char instruction[] PROGMEM = "______PRESS_BUTTON_TO_START_____";
//char inst_index; // less than 255
//char inst_cycle; // less than 255

unsigned long previous_millis;
unsigned long current_millis;
unsigned long delay_millis = START_MILLIS;

gameState* my_game;
int cur_jumping_idx = 0;
bool game_running = false;

#define BUTTON_DOWN 0
#define BUTTON_UP 1
#define DEBOUNCE 100 

bool buttonPushed = false;
unsigned char newState, prevState;
unsigned long lastSwitchTime; 

/* This function places the current value of the heap and stack pointers in the
   variables. You can call it from any place in your code and save the data for
   outputting or displaying later. This allows you to check at different parts of
   your program flow.
   The stack pointer starts at the top of RAM and grows downwards. The heap pointer
   starts just above the static variables etc. and grows upwards. SP should always
   be larger than HP or you'll be in big trouble! The smaller the gap, the more
   careful you need to be. Julian Gall 6-Feb-2009.
*/
//int * heapptr, * stackptr;
//void check_mem() {
//  stackptr = (int *)malloc(4);          // use stackptr temporarily
//  heapptr = stackptr;                     // save value of heap pointer
//  free(stackptr);      // free up the memory again (sets stackptr to 0)
//  stackptr =  (int *)(SP);           // save value of stack pointer
//
//  Serial.print(F("SP: ")); Serial.print((unsigned int) stackptr); Serial.print(F(" > HP: "));
//  Serial.println((unsigned int) heapptr);
//}

void setup() {

  // For Serial Monitor.
  //Serial.begin(9600);

  // For button.
  PORTB |= (1 << PB7); // passive pull-up
  pinMode(BUTTON_PIN, INPUT); 

   // Initial state of button.
  newState = digitalRead(BUTTON_PIN); //PINB & (1 << PB7);
  // For debouncing.
  lastSwitchTime = 0;

  // For LED Matrix.
  matrix.begin();

  //check_mem();

  // Initialize a new game.
  my_game = (gameState*) malloc(sizeof(gameState));
  restartGame();
  
  MAX_OBSTACLE_INDEX = strlen_P(obstacles) - 6;

  previous_millis = millis();

  matrix.fillScreen(0); // Clear the LED board.
  displayStartScreen();

//  color = YALE_PURPLE;
//  displayLineOfText(welcome, welcome_index, welcome_cycle, matrix.height() - CHAR_HEIGHT, 4);
//  displayStartMessage();
 
}

void restartGame() {
  my_game->active = true;
  my_game->lives = 4;
  my_game->scores = 0;
  my_game->time_step = 0;
  my_game->year = 1;
  my_game->bulldog_altitude = 0;
  my_game->bulldog_max_altitude = 10;
  my_game->bulldog_platform = GROUND;
  my_game->bulldog_jumping = false;
  my_game->bulldog_jumping_over_gap = false;
  my_game->bulldog_jumping_direction = UP;

  obstacle_index = 0;
  obstacle_cycle = 0;
}

void loop() {
  unsigned long current_millis = millis();
  
  prevState = newState;
  newState = digitalRead(BUTTON_PIN); //PINB & (1 << PB7);
  // watch out for holding down?
  if ((current_millis - DEBOUNCE*2) > lastSwitchTime && newState == BUTTON_DOWN){
    buttonPushed = true;// !buttonState;
    lastSwitchTime = current_millis;
  }
  
  if (current_millis - previous_millis >= delay_millis) {
    if(!game_running) {
      if (buttonPushed) {
        buttonPushed = false;
        game_running = true;
        matrix.fillScreen(0);
        displayLives();
        displayScores();
        displayHeart();
        displayBulldog();
        displayObstacles();

        delay_millis = GAME_MILLIS;
      }
//      else{
//        //matrix.fillScreen(0);
//        displayCongrats();
//      }
//      else {
//        displayStartMessage();
//        updateInstCycle();
//        //  used to have welcome message scroll too
//      }
    }
 
    else{
      
      if (my_game->time_step == 0) {
        displayYear();
      } else if (my_game->time_step == 10) {
        clearYear();
      }
  
      if (buttonPushed && my_game->bulldog_jumping == false) {
        buttonPushed = false;
        my_game->bulldog_jumping = true;
        my_game->bulldog_jumping_direction = UP;
      }
      else if (buttonPushed) buttonPushed = false; // reset for next detection
  
      unsigned char front_leg_char = getCharOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_FRONT_LEG_OFFSET);
      unsigned char back_leg_char = getCharOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_BACK_LEG_OFFSET);
      bool both_legs_on_char = (front_leg_char != '_' && front_leg_char != '-') && (back_leg_char != '_' && back_leg_char != '-');
      bool one_leg_on_char = (front_leg_char != '_' && front_leg_char != '-') || (back_leg_char != '_' && back_leg_char != '-');
  
      if (my_game->active && one_leg_on_char && (my_game->bulldog_platform + my_game->bulldog_altitude) < CHAR_HEIGHT) {
        decreaseLives();
        my_game->active = false;
      }
  
      if (!my_game->active && !one_leg_on_char) {
        my_game->active = true;
      }
  
      // Bulldog is jumping.
      if (my_game->bulldog_jumping == true) {
  
        if (front_leg_char == '-' || back_leg_char == '-') {
          my_game->bulldog_jumping_over_gap = true;
        }
  
        // Bulldog is in the upward portion of the jump sequence.
        if (my_game->bulldog_jumping_direction == UP) {
  
          // Clear the bottom row of the bulldog, which will no longer be within the bulldog frame.
          clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude);
  
          my_game->bulldog_altitude++;
  
          if (my_game->bulldog_altitude == my_game->bulldog_max_altitude) {
            my_game->bulldog_jumping_direction = DOWN;
          }
        }
  
        // Bulldog is in the downward portion of the jump sequence.
        else {
  
          // Clear the top row of the bulldog, which will no longer be within the bulldog frame.
          clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude + (SPRITE_HEIGHT - 1));
  
          my_game->bulldog_altitude--;
  
          if (my_game->bulldog_platform == GROUND) {
  
            // The bulldog jumped from ground and landed on an obstacle. Score is incremented.
            if (my_game->bulldog_altitude == OBSTACLE_TOP && both_legs_on_char) {
              my_game->bulldog_jumping = false;
              my_game->bulldog_platform = OBSTACLE_TOP;
              my_game->bulldog_altitude = 0;
              my_game->bulldog_max_altitude = 7;
              if (my_game->active) {
                increaseScores();
              }
            }
  
            // The bulldog jumped from ground and landed back on ground.
            else if (my_game->bulldog_altitude == 0) {
              my_game->bulldog_jumping = false;
            }
          }
  
          // The bulldog jumped from obstacle top and landed back on an obstacle.
          else {
            if (my_game->bulldog_altitude == 0) {
              my_game->bulldog_jumping = false;
              if (my_game->active && my_game->bulldog_jumping_over_gap) {
                increaseScores();
              }
              my_game->bulldog_jumping_over_gap = false;
            }
          }
        }
      }
  
      // Bulldog is not jumping.
      else {
  
        // If the bulldog is on an obstacle and reaches the end of that obstacle, it moves down to ground.
        if (my_game->bulldog_platform == OBSTACLE_TOP && !one_leg_on_char) {
          my_game->bulldog_jumping = true;
          my_game->bulldog_jumping_direction = DOWN;
          my_game->bulldog_platform = GROUND;
          my_game->bulldog_altitude = OBSTACLE_TOP;
          my_game->bulldog_max_altitude = 10;
          clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude + (SPRITE_HEIGHT - 1));
          my_game->bulldog_altitude--;
        }
      }

      // game might have ended since last check
      if(game_running){
        displayObstacles();
        displayBulldog();
      }
        
      obstacle_cycle++;
      if (obstacle_cycle >= CHAR_WIDTH + 1) {
        obstacle_cycle = 0;
        obstacle_index++;
        // For now, restart the game when out of characters.
        if (obstacle_index >= MAX_OBSTACLE_INDEX) {
          obstacle_index = 0;
          my_game->lives = 4;
          my_game->time_step = -1;
          my_game->year++;
          clearLives();
          displayLives();
        }
      }
      my_game->time_step++;
    }
    
    previous_millis = current_millis;
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
    if (current_char == '_' || current_char == '-' || current_char_column == 5) {
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
    if (current_char == '_' || current_char == '-' || current_char_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + current_char_column]));
    }

    for (int row = 0; row < SPRITE_HEIGHT; row++) {
      if (row >= (my_game->bulldog_platform + my_game->bulldog_altitude)) {
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
        color = my_game->active ? WHITE : RED;
      } else if (bulldog_pixel == 'B') {
        color = my_game->active ? BLUE : WHITE;
      }
      matrix.drawPixel((SPRITE_HEIGHT - 1) - row + (my_game->bulldog_platform + my_game->bulldog_altitude), BULLDOG_LEFT_OFFSET + col, color);
    }
  }
}

bool isInBulldogFrame(int row, int col) {
  bool colInFrame = (col >= BULLDOG_LEFT_OFFSET && col < BULLDOG_LEFT_OFFSET + SPRITE_WIDTH);
  bool rowInFrame = (row >= my_game->bulldog_platform + my_game->bulldog_altitude && row < my_game->bulldog_platform + my_game->bulldog_altitude + SPRITE_HEIGHT);
  return (colInFrame && rowInFrame);
}

void clearBulldogRow(int row) {
  for (int col = 0; col < SPRITE_WIDTH; col++) {
    matrix.drawPixel(row, BULLDOG_LEFT_OFFSET + col, 0);
  }
}

void displayLives() {
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  for (int col = 0; col < CHAR_WIDTH; col++) {
    current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[(my_game->lives * 5) + col]));
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
}

void displayHeart() {
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

void clearLives() {
  for (int row = matrix.height() - CHAR_HEIGHT; row < matrix.height(); row++) {
    for (int col = 0; col < CHAR_WIDTH; col++) {
      matrix.drawPixel(row, col, 0);
    }
  }
}

void decreaseLives() {
  my_game->lives--;

  if(my_game->lives <= 0) {
     endGame(false); 
  }
  else {
     clearLives();
     displayLives();
  }
}

void displayScores() {
  unsigned char current_column_bitmap;
  unsigned char current_bit;

  // Draw first digit.
  int first_digit = (int) my_game->scores / 10;
  if (first_digit != 0) {
    for (int col = 0; col < CHAR_WIDTH; col++) {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[(first_digit * 5) + col]));
      for (int row = 0; row < CHAR_HEIGHT; row++) {
        current_bit = current_column_bitmap & 0x1;
        current_column_bitmap >>= 1;
        if (current_bit == 0x1) {
          color = GREEN;
        } else {
          color = 0;
        }
        // Draw top to bottom, left to right.
        // Top row of first digit is on the top edge of display.
        // Left column of first digit is 2 chars and 1 space away from right edge of display.
        matrix.drawPixel((matrix.height() - 1) - row, matrix.width() - (CHAR_WIDTH + 1 + CHAR_WIDTH) + col, color);
      }
    }
  }

  // Draw second digit.
  int second_digit = my_game->scores % 10;
  for (int col = 0; col < CHAR_WIDTH; col++) {
    current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[(second_digit * 5) + col]));
    for (int row = 0; row < CHAR_HEIGHT; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (current_bit == 0x1) {
        color = GREEN;
      } else {
        color = 0;
      }
      // Draw top to bottom, left to right.
      // Top row of second digit is on the top edge of display.
      // Left column of second digit is 1 char away from right edge of display.
      matrix.drawPixel((matrix.height() - 1) - row, matrix.width() - CHAR_WIDTH + col, color);
    }
  }
}

void clearScores() {
  for (int row = matrix.height() - CHAR_HEIGHT; row < matrix.height(); row++) {
    for (int col = matrix.width() - (CHAR_WIDTH + 1 + CHAR_WIDTH); col < matrix.width(); col++) {
      matrix.drawPixel(row, col, 0);
    }
  }
}

void increaseScores() {
  my_game->scores++;
  clearScores();
  displayScores();

  if(my_game->scores == CREDITS) {
    endGame(true);
  }
}

void displayYear() {
  int top_offset = matrix.height() - 10;
  int year_left_offset = 1;
  int digit_left_offset = matrix.width() - 1 - CHAR_WIDTH;

  char current_char;
  unsigned char current_column_bitmap;
  unsigned char current_bit;

  // Display the letters YEAR.
  char year[] = "YEAR";
  for (int c = 0; c < 4; c++) {
    current_char = year[c];
    for (int col = 0; col < CHAR_WIDTH; col++) {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_char - 'A') * 5) + col]));
      for (int row = 0; row < CHAR_HEIGHT; row++) {
        current_bit = current_column_bitmap & 0x1;
        current_column_bitmap >>= 1;
        if (current_bit == 0x1) {
          color = PURPLE;
        } else {
          color = 0;
        }
        matrix.drawPixel(top_offset - row, year_left_offset + c * 6 + col, color);
      }
    }
  }

  // Display year number.
  for (int col = 0; col < CHAR_WIDTH; col++) {
    current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[(my_game->year * 5) + col]));
    for (int row = 0; row < CHAR_HEIGHT; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (current_bit == 0x1) {
        color = LIGHT_BLUE;
      } else {
        color = 0;
      }
      matrix.drawPixel(top_offset - row, digit_left_offset + col, color);
    }
  }
}

void clearYear() {
  int top_offset = matrix.height() - 10;
  for (int col = 0; col < matrix.width(); col++) {
    for (int row = 0; row < CHAR_HEIGHT; row++) {
      matrix.drawPixel(top_offset - row, col, 0);
    }
  }
}

//void updateInstCycle(){
//  inst_cycle++;
//  if (inst_cycle >= CHAR_WIDTH + 1) {
//    inst_cycle = 0;
//    inst_index++;
//    // keep looping
//    if (inst_index >= MAX_INST_INDEX) {
//      inst_index = 0;
//    }
//  }
//}

// display start screen
//void displayStartMessage() {
//  color = BLUE;
//  displayLineOfText(instruction, inst_index, inst_cycle, 0, 0);
//}

void displayStartScreen() {
  displayBigBulldog();
  color = BLUE;
  displayLineOfText(welcome, STATIC_INDEX, STATIC_CYCLE, matrix.height() - CHAR_HEIGHT, 0, false);
  displayLineOfText(welcome, STATIC_INDEX, STATIC_CYCLE, 0, 3, false);
//  displayLineOfText(welcome, welcome_index, welcome_cycle, matrix.height() - CHAR_HEIGHT, 0);
//  displayLineOfText(welcome, welcome_index, welcome_cycle, 0, 3);
}

// message must be string in PROGMEM
unsigned char getCharOfColumnGeneral(int column, const char *message, int index, int cycle) {
  return (unsigned char) pgm_read_byte(&(message[index + (int) ((column + cycle) / 6)]));
}

unsigned char getCharColumnOfColumnGeneral(int column, int cycle) {
  return (column + cycle) % 6;
}

// message must be string in PROGMEM with only alphabetic characters
void displayLineOfText(const char *message, int index, int cycle, int row_offset, int col_offset, bool rainbow) {
  unsigned char current_char;
  int current_char_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
//  check_mem();
  for (int col = 0; col < 32; col++) {
    current_char =  getCharOfColumnGeneral(col, message, index, cycle);
    current_char_column = getCharColumnOfColumnGeneral(col, cycle);
    // currently doesn't use the - between words (bc that's for the bulldog to know 
    // if it's jumping over air, not necessary for general display
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

void endGame(bool win) {
  game_running = false;
  delay_millis = START_MILLIS; // make messages scroll faster
  matrix.fillScreen(0);
  if(win)
    displayWinScreen();
  else
    displayLoseScreen();
  // use millis? this is blocking so it wont work if we want to scroll/animate
  delay(3000);
  
  if(win){
    matrix.fillScreen(0);
    displayCongrats();
    delay(5000);
  }
  restartGame();
  matrix.fillScreen(0);
  displayStartScreen();
  
}

void displayWinScreen() {
   color = LIGHT_GREEN;
   displayLineOfText(congrats, STATIC_INDEX, STATIC_CYCLE, (matrix.height() / 2 - CHAR_HEIGHT / 2), -1, false);
}

void displayLoseScreen() {
  color = RED;
  displayLineOfText(dropout, STATIC_INDEX, STATIC_CYCLE, (matrix.height() / 2 - CHAR_HEIGHT / 2), -1, false);
}


void displayCongrats() {
  displayConfetti();
  displayLineOfText(new_word, STATIC_INDEX, STATIC_CYCLE, matrix.height() - CHAR_HEIGHT - 2, 1, true);
  displayLineOfText(grad, STATIC_INDEX, STATIC_CYCLE, 2, 4, true);

}

void displayConfetti() {
  // Draw confetti decal.
  unsigned char pixel;
  for (int col = 0; col < BIG_SPRITE_WIDTH; col++) {
    for (int row = 0; row < BIG_SPRITE_HEIGHT; row++) {
      pixel = (unsigned char) pgm_read_byte(&(grad_decal[row][col]));
      if (pixel == '_') {
        color = 0;
      } else if (pixel == 'P') {
        color = PURPLE;
      } else if (pixel == 'B') {
        color = LIGHT_BLUE;
      } else if (pixel == 'W') {
        color = WHITE;
      }
      matrix.drawPixel((matrix.height() / 2) + (BIG_SPRITE_HEIGHT / 2) - row, ((matrix.width() / 2) - (BIG_SPRITE_WIDTH / 2))  + 1 + col, color);
    }
  }
}

//        // debugging
//        color = LIGHT_BLUE;
//        displayLineOfText(congrats, STATIC_INDEX, STATIC_CYCLE, 0, 0);
        
