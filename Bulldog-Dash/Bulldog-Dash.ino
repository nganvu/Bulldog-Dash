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
#define GAME_MILLIS 120
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
#define TEAL (matrix.Color333(2, 7, 2))
#define BLUE (matrix.Color333(0, 0, 7))
#define LIGHT_BLUE (matrix.Color333(2, 2, 7))
#define YELLOW (matrix.Color333(7, 3, 0))
#define ORANGE (matrix.Color333(7, 1, 0))
#define PURPLE (matrix.Color333(7, 0, 7))
#define PINK (matrix.Color333(7, 0, 3))
#define DARK_PURPLE (matrix.Color333(3, 0, 7))

#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_FRONT_LEG_OFFSET (4)
#define BULLDOG_BACK_LEG_OFFSET (2)
#define BULLDOG_TOP_OFFSET (26)

#define POINTS_PER_OBSTACLE 2
#define CREDITS 36

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

//// 56 bytes
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


// 210 bytes + more
const char freshman[] PROGMEM  = "______2015______CAMP-YALE_____FRISBEE____IMPOSTOR-SYNDROME_______";
const char sophomore[] PROGMEM = "______MAJOR-DILEMMA_____CS223____SLUMP____ENGL120________";
const char junior[] PROGMEM    = "______CS323_____INTERNSHIP____ULAING_____FINALS______";
const char senior[] PROGMEM    = "______INTERVIEWS_____SOCIETY____EXISTENTIAL-CRISIS______THESIS______SENIOR-WEEK_____2019______";

// reuse these since one set of obstacles at a time
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

unsigned long previous_millis;
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


void setup() {

  // For button.
  PORTB |= (1 << PB7); // passive pull-up
  pinMode(BUTTON_PIN, INPUT); 
   // Initial state of button.
  newState = digitalRead(BUTTON_PIN); //PINB & (1 << PB7);
  // For debouncing.
  lastSwitchTime = 0;

  // For LED Matrix.
  matrix.begin();

  // Initialize a new game.
  my_game = (gameState*) malloc(sizeof(gameState));
  restartGame();

  MAX_OBSTACLE_INDEX = strlen_P(freshman) - 6;
  previous_millis = millis();

  matrix.fillScreen(0); // Clear the LED board.
  displayStartScreen();
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
  newState = digitalRead(BUTTON_PIN); 
  
  if ((current_millis - DEBOUNCE*2) > lastSwitchTime && prevState != newState && newState == BUTTON_DOWN){
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
        
        if (obstacle_index >= MAX_OBSTACLE_INDEX) { //my_game->year - 1
          obstacle_index = 0;
          my_game->time_step = -1;
          my_game->year++;

          delay_millis -= 20;

          if(my_game->year == 2){
            MAX_OBSTACLE_INDEX = strlen_P(sophomore) - 6;
          } else if(my_game->year == 3){
            MAX_OBSTACLE_INDEX = strlen_P(junior) - 6;
          } else if(my_game->year == 4){
            MAX_OBSTACLE_INDEX = strlen_P(senior) - 6;
          } else if(my_game->year >= 5){
            endGame(false); // maybe alternate ending?
          }
          
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
  const char *obstacle;
  if(my_game->year == 1) obstacle = freshman;
  else if(my_game->year == 2) obstacle = sophomore;
  else if(my_game->year == 3) obstacle = junior;
  else if(my_game->year == 4) obstacle = senior;
  
  return (unsigned char) pgm_read_byte(&(obstacle[obstacle_index + (int) ((column + obstacle_cycle) / 6)]));
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
  if(my_game->year == 1) color = YELLOW;
  else if(my_game->year == 2) color = ORANGE;
  else if(my_game->year == 3) color = PINK;
  else if(my_game->year == 4) color = DARK_PURPLE;
  
  for (int col = 0; col < 32; col++) {
    current_char =  getCharOfColumn(col);
    current_char_column = getCharColumnOfColumn(col);
    if (current_char == '_' || current_char == '-' || current_char_column == 5) {
      current_column_bitmap = 0;
    } else if (isdigit(current_char)) {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(DigitFont[((current_char - '0') * 5) + current_char_column]));
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
          if(my_game->year == 1) color = YELLOW;
          else if(my_game->year == 2) color = ORANGE;
          else if(my_game->year == 3) color = PINK;
          else if(my_game->year == 4) color = DARK_PURPLE;
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
  my_game->scores += POINTS_PER_OBSTACLE;
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

void displayStartScreen() {
  displayBigBulldog();
  color = BLUE;
  displayLineOfText(welcome, STATIC_INDEX, STATIC_CYCLE, matrix.height() - CHAR_HEIGHT, 0, false);
  displayLineOfText(welcome, STATIC_INDEX, STATIC_CYCLE, 0, 3, false);
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
    
    if (current_char == '_' || current_char_column == 5) {
      current_column_bitmap = 0;
      // If reached the end of a letter in rainbow mode, change color.
      if (rainbow && current_char_column == 5 && current_char != '_') {
        if (color == TEAL) {
          color = GREEN;
        } else if (color == GREEN) {
          color = YELLOW;
        } else if (color == YELLOW) {
          color = ORANGE;
        } else if (color == ORANGE) {
          color = RED;
        } else if (color == RED) {
          color = PINK;
        } else if (color == PINK) {
          color = PURPLE;
        }
      }
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
  delay(3000); // blocking
  
  if(win){
    matrix.fillScreen(0);
    displayCongrats();
    
    unsigned long current_millis;
    unsigned long prev_millis = millis();
    int elapsed;
    char mode = 0;
    while(true){
      elapsed = millis() - prev_millis;
      
      if(elapsed < 500 && mode == 0) {
        displayBlueConfetti(false);
        displayWhiteConfetti(false);
        mode = 1;
      }
      else if(elapsed >= 500 && elapsed < 1000 && mode == 1) {
        displayBlueConfetti(true);
        mode = 2;
      }
      else if(elapsed >= 1000 && elapsed < 1500 && mode == 2) {
        displayWhiteConfetti(true);
        mode = 3;
      }
      else if(elapsed >= 1500){
        prev_millis = millis(); // start a new loop
        mode = 0;
      }
   
      // check for button
      current_millis = millis();
      prevState = newState;
      newState = digitalRead(BUTTON_PIN); 
      if (prevState != newState && newState == BUTTON_DOWN && (current_millis - DEBOUNCE) > lastSwitchTime) {  
        lastSwitchTime = current_millis;
        break;
      }
    }
  }
  restartGame();
  matrix.fillScreen(0);
  displayStartScreen();
  
}

void displayWinScreen() {
   color = GREEN;
   displayLineOfText(congrats, STATIC_INDEX, STATIC_CYCLE, (matrix.height() / 2 - CHAR_HEIGHT / 2), -1, false);
}

void displayLoseScreen() {
  color = RED;
  displayLineOfText(dropout, STATIC_INDEX, STATIC_CYCLE, (matrix.height() / 2 - CHAR_HEIGHT / 2), -1, false);
}


void displayCongrats() {
  displayGradBulldog();
  color = TEAL;
  displayLineOfText(new_word, STATIC_INDEX, STATIC_CYCLE, matrix.height() - CHAR_HEIGHT, 1, true);
  displayLineOfText(grad, STATIC_INDEX, STATIC_CYCLE, 0, 4, true);
}

void displayGradBulldog() {
  int row;
  int col;
  
  // Draw bulldog.
  unsigned char pixel;
  for (col = 0; col < SPRITE_WIDTH; col++) {
    for (row = 0; row < SPRITE_HEIGHT; row++) {
      pixel = (unsigned char) pgm_read_byte(&(bulldog[row][col]));
      if (pixel == '_') {
        color = 0;
      } else if (pixel == 'B') {
        color = BLUE;
      } else if (pixel == 'W') {
        color = WHITE;
      }
      drawCenteredSpritePixel(row, col, SPRITE_HEIGHT, SPRITE_WIDTH);
    }
  }

  // Draw graduation cap.
  color = BLUE;

  // Top row.
  row = 0;
  for (col = 2; col < 7; col++) {
    drawCenteredSpritePixel(row, col, SPRITE_HEIGHT, SPRITE_WIDTH);
  }

  // Bottom row.
  row = 1;
  for (col = 3; col < 6; col++) {
    drawCenteredSpritePixel(row, col, SPRITE_HEIGHT, SPRITE_WIDTH);
  }
  
  // Tassel.
  color = YELLOW;
  drawCenteredSpritePixel(1, 6, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel(2, 6, SPRITE_HEIGHT, SPRITE_WIDTH);
}

void displayBlueConfetti(bool show_confetti) {
  int row;
  int col;

  color = show_confetti ? BLUE : 0;  
  
  // Horizontal line.
  row = SPRITE_HEIGHT / 2;
  for (col = 0; col < 3; col++) {
    drawCenteredSpritePixel(row, SPRITE_WIDTH + 1 + col, SPRITE_HEIGHT, SPRITE_WIDTH); // Right side.
    drawCenteredSpritePixel(row, -2 - col, SPRITE_HEIGHT, SPRITE_WIDTH); // Left side.
  }
  
  // Vertical line.
  col = SPRITE_WIDTH / 2;
  for (row = 0; row < 3; row++) {
    drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top side.
    drawCenteredSpritePixel(-2 - row, col, SPRITE_HEIGHT, SPRITE_WIDTH); // Bottom side.
  }

  // Diagonal lines, top left to bottom right.
  for (row = 0; row < 2; row++) {
    col = row;
    drawCenteredSpritePixel(-2 - row, -2 - col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top left.
    drawCenteredSpritePixel(-2 - row, SPRITE_WIDTH + 1 + col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top right.
    drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, -2 - col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top left.
    drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, SPRITE_WIDTH + 1 + col, SPRITE_HEIGHT, SPRITE_WIDTH); // Bottom right.
  }
}

void displayWhiteConfetti(bool show_confetti) {
  int row;
  int col;

  color = show_confetti ? WHITE : 0;
  
  // Horizontal line.
  row = SPRITE_HEIGHT / 2;
  col = 3;
  drawCenteredSpritePixel(SPRITE_HEIGHT / 2, SPRITE_WIDTH + 4, SPRITE_HEIGHT, SPRITE_WIDTH); // Right side.
  drawCenteredSpritePixel(SPRITE_HEIGHT / 2, -5, SPRITE_HEIGHT, SPRITE_WIDTH); // Left side.
  
  // Vertical line.
  col = SPRITE_WIDTH / 2;
  row = 3;
  drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top side.
  drawCenteredSpritePixel(-2 - row, col, SPRITE_HEIGHT, SPRITE_WIDTH); // Bottom side.

  // Diagonal lines, top left to bottom right.
  col = 2;
  row = 2;
  drawCenteredSpritePixel(-2 - row, -2 - col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top left.
  drawCenteredSpritePixel(-2 - row, SPRITE_WIDTH + 1 + col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top right.
  drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, -2 - col, SPRITE_HEIGHT, SPRITE_WIDTH); // Top left.
  drawCenteredSpritePixel(SPRITE_HEIGHT + 1 + row, SPRITE_WIDTH + 1 + col, SPRITE_HEIGHT, SPRITE_WIDTH); // Bottom right.

  // In between white pixels.
  drawCenteredSpritePixel(-3,  0, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel(-3,  6, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 0, -3, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 0,  9, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 6, -3, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 6,  9, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 9,  0, SPRITE_HEIGHT, SPRITE_WIDTH);
  drawCenteredSpritePixel( 9,  6, SPRITE_HEIGHT, SPRITE_WIDTH);
}

// Draw a pixel of a sprite given the row and column relative to that pixel.
void drawCenteredSpritePixel(int row, int col, int sprite_height, int sprite_width) {
  matrix.drawPixel((matrix.height() / 2) + (sprite_height / 2) - 1 - row, ((matrix.width() / 2) - (sprite_width / 2)) - 1 + col, color);
}       
