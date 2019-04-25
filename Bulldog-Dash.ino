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

#define SPRITE_HEIGHT (7)

// For jumping direction.
#define UP true
#define DOWN false

// For bulldog platform (where it is jumping from).
#define OBSTACLE_TOP (SPRITE_HEIGHT)
#define GROUND (0)

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

uint16_t color;
#define WHITE (matrix.Color333(7, 7, 7))
#define BLUE (matrix.Color333(0, 0, 7))
#define PURPLE (matrix.Color333(7, 0, 7))

#define DISP_SIZE (32)
#define BULLDOG_SIZE (7)
#define BULLDOG_FRONT_EAR_OFFSET (2)
#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_FRONT_LEG_OFFSET (4)
#define BULLDOG_BACK_LEG_OFFSET (2)
#define BULLDOG_TOP_OFFSET (26)
#define JUMP_SEQUENCE_LENGTH (19)
#define LAND_ON_OBSTACLE_CHECK (11)
#define MAX_OBSTACLE_CYCLE (6)
int MAX_OBSTACLE_INDEX;

typedef struct gameState {
  char* disp_mat;
  int bulldog_altitude;
  int bulldog_max_altitude;
  int bulldog_platform;
  bool bulldog_jumping;
  bool jumping_direction;
  int lives;
  int scores;
} gameState;

// +1 because extra space needed for string null-terminator.
const char bulldog[BULLDOG_SIZE][BULLDOG_SIZE + 1] PROGMEM = {
  "__W___W",
  "___WWW_",
  "___WWW_",
  "WBWBWW_",
  "_WBWW__",
  "_WBWW__",
  "__W_W__"
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

const char obstacles[] PROGMEM =
"______HELLO_WORLD______";
int obstacle_index; // Current character at the bottom left corner.
int obstacle_cycle; // Each character takes up 5 columns, so it takes 5 cycles to move 1 character over.

unsigned long previous_millis;
unsigned long current_millis;
unsigned long delay_millis = 150;

gameState* my_game;
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

  // Initialize a new game.
  my_game = (gameState*) malloc(sizeof(gameState));
  my_game->bulldog_altitude = 0;
  my_game->bulldog_max_altitude = 10;
  my_game->bulldog_platform = GROUND;
  my_game->bulldog_jumping = false;
  my_game->jumping_direction = UP;
  my_game->lives = 4;
  my_game->scores = 0;
  
  obstacle_index = 0;
  obstacle_cycle = 0;
  MAX_OBSTACLE_INDEX = strlen_P(obstacles) - 6;
  
  previous_millis = millis();

  matrix.fillScreen(0); // Clear the LED board.
  displayLetters();
  displayBulldog();
}

void loop() {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= delay_millis) {
    int read_val = Serial.read();
    
    // When the letter 'A' is sent.
    if (read_val == 'A' && my_game->bulldog_jumping == false) {
      my_game->bulldog_jumping = true;
      my_game->jumping_direction = UP;
    }

    unsigned char front_leg_letter = getLetterOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_FRONT_LEG_OFFSET);
    unsigned char back_leg_letter = getLetterOfColumn(BULLDOG_LEFT_OFFSET + BULLDOG_BACK_LEG_OFFSET);
    bool bulldog_on_letter = (front_leg_letter != '_') && (back_leg_letter != '_');

    // Bulldog is jumping.
    if (my_game->bulldog_jumping == true) {
      if (my_game->jumping_direction == UP) {
        clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude);
        my_game->bulldog_altitude++;
        if (my_game->bulldog_altitude == my_game->bulldog_max_altitude) {
          my_game->jumping_direction = DOWN;
        }
      } else {
        clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude + (SPRITE_HEIGHT - 1));
        my_game->bulldog_altitude--;
        if (my_game->bulldog_platform == GROUND) {
          if (my_game->bulldog_altitude == OBSTACLE_TOP && bulldog_on_letter) {
            my_game->bulldog_jumping = false;
            my_game->bulldog_platform = OBSTACLE_TOP;
            my_game->bulldog_altitude = 0;
            my_game->bulldog_max_altitude = 7;
          } else if (my_game->bulldog_altitude == 0) {
            my_game->bulldog_jumping = false;
          }
        }
        else if (my_game->bulldog_platform == OBSTACLE_TOP) {
          if (my_game->bulldog_altitude == 0) {
            my_game->bulldog_jumping = false;
          }
        }
      }
    }

    // Bulldog is not jumping.
    else {
      if (my_game->bulldog_platform == OBSTACLE_TOP && !bulldog_on_letter) {
        my_game->bulldog_jumping = true;
        my_game->jumping_direction = DOWN;
        my_game->bulldog_platform = GROUND;
        my_game->bulldog_altitude = OBSTACLE_TOP;
        my_game->bulldog_max_altitude = 10;
        clearBulldogRow(my_game->bulldog_platform + my_game->bulldog_altitude + (SPRITE_HEIGHT - 1));
        my_game->bulldog_altitude--;
      }
    }
    
    Serial.print(my_game->bulldog_platform);
    Serial.print(my_game->bulldog_altitude);
    Serial.println(my_game->bulldog_platform + my_game->bulldog_altitude);
    
    displayLetters();
    displayBulldog();
    obstacle_cycle++;
    if (obstacle_cycle >= MAX_OBSTACLE_CYCLE) {
      obstacle_cycle = 0;
      obstacle_index++;
      // For now, restart the game when out of characters.
      if (obstacle_index >= MAX_OBSTACLE_INDEX) {
        obstacle_index = 0;
        my_game->lives = 4;
      }
    }
    
    previous_millis = current_millis;
  }
}

unsigned char getLetterOfColumn(int column) {
  return (unsigned char) pgm_read_byte(&(obstacles[obstacle_index + (int) ((column + obstacle_cycle) / 6)]));
}

unsigned char getLetterColumnOfColumn(int column) {
  return (column + obstacle_cycle) % 6;
}

// Display the obstacles, except for things within the square containing the bulldog.
void displayLetters() {
  char current_letter;
  int current_letter_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  color = PURPLE;
  for (int col = 0; col < 32; col++) {
    current_letter =  getLetterOfColumn(col);
    current_letter_column = getLetterColumnOfColumn(col);
    if (current_letter == '_' || current_letter_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_letter - 'A') * 5) + current_letter_column]));
    }
    for (int row = 0; row < 7; row++) {
      current_bit = current_column_bitmap & 0x1;
      current_column_bitmap >>= 1;
      if (isInBulldogFrame((BULLDOG_SIZE - 1) - row, col)) {
        continue;
      }
      if (current_bit == 0x1) {
        matrix.drawPixel((BULLDOG_SIZE - 1) - row, col, color);
      } else {
        matrix.drawPixel((BULLDOG_SIZE - 1) - row, col, 0);
      }
    }
  }
}



// Display the bulldog, as well as anything that is within the square containing the bulldog.
void displayBulldog() {
  char current_letter;
  int current_letter_column;
  unsigned char current_column_bitmap;
  unsigned char current_bit;
  unsigned char bulldog_pixel;
  for (int col = 0; col < BULLDOG_SIZE; col++) {
    current_letter =  getLetterOfColumn(BULLDOG_LEFT_OFFSET + col);
    current_letter_column = getLetterColumnOfColumn(BULLDOG_LEFT_OFFSET + col);
    if (current_letter == '_' || current_letter_column == 5) {
      current_column_bitmap = 0;
    } else {
      current_column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_letter - 'A') * 5) + current_letter_column]));
    }
    
    for (int row = 0; row < BULLDOG_SIZE; row++) {
      if (row >= (my_game->bulldog_platform + my_game->bulldog_altitude)) {
        current_bit = current_column_bitmap & 0x1;
        current_column_bitmap >>= 1;
      } else {
        current_bit = 0;
      }
      bulldog_pixel =  (unsigned char) pgm_read_byte(&(bulldog[row][col]));
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
      matrix.drawPixel((BULLDOG_SIZE - 1) - row + (my_game->bulldog_platform + my_game->bulldog_altitude), BULLDOG_LEFT_OFFSET + col, color);
    }
  }  
}

bool isInBulldogFrame(int row, int col) {
  bool colInFrame = (col >= BULLDOG_LEFT_OFFSET && col < BULLDOG_LEFT_OFFSET + BULLDOG_SIZE);
  bool rowInFrame = (row >= my_game->bulldog_platform + my_game->bulldog_altitude && row < my_game->bulldog_platform + my_game->bulldog_altitude + SPRITE_HEIGHT);
  return (colInFrame && rowInFrame);
}

void clearBulldogRow(int row) {
  for (int col = 0; col < BULLDOG_SIZE; col++) {
    matrix.drawPixel(row, BULLDOG_LEFT_OFFSET + col, 0);
  }
}
