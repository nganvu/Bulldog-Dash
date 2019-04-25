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

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);

uint16_t color;

#define DISP_SIZE (32)
#define BULLDOG_SIZE (7)
#define BULLDOG_FRONT_EAR_OFFSET (2)
#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_TOP_OFFSET (26)
#define JUMP_SEQUENCE_LENGTH (19)
#define LAND_ON_OBSTACLE_CHECK (11)
#define MAX_OBSTACLE_CYCLE (6)
int MAX_OBSTACLE_INDEX;

// 9 bytes
typedef struct gameState {
  char* disp_mat;
  int bulldog_altitude;
  bool bulldog_jumping;
  int lives;
  int scores;
} gameState;

// +1 because extra space needed for string null-terminator.
// 56 bytes
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

// 38 bytes
// calculate this instead
//int bulldog_jump_heights[JUMP_SEQUENCE_LENGTH] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

const char obstacles[] PROGMEM =
"_HELLO______";
int obstacle_index; // Current character at the bottom left corner.
int obstacle_cycle; // Each character takes up 5 columns, so it takes 5 cycles to move 1 character over.

unsigned long previous_millis;
unsigned long current_millis;
unsigned long delay_millis = 200;

gameState* my_game;
int cur_jumping_idx = 0;


//size_t mystrlen(const char *str)
//{
//    size_t len;
//    for (len = 0; str[len]; len++);
//    return len;
//}

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
  
  unsigned char column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[(('A' - 'A') * 5) + 0]));

  Serial.println(column_bitmap, HEX);
  //Serial.println(column_bitmap);
  // Serial.println("hi");

//  // Initialize a new game.
  my_game = (gameState*) malloc(sizeof(gameState));
//  my_game->disp_mat = (char*) malloc(DISP_SIZE * (DISP_SIZE + 1) * sizeof(char));
//  for (int i = 0; i < DISP_SIZE - 1; i++) {
//    my_game->disp_mat[getIndex(i, DISP_SIZE)] = '\n';
//  }
//  my_game->disp_mat[getIndex(DISP_SIZE - 1, DISP_SIZE)] = '\0';
  my_game->bulldog_altitude = 0;
  my_game->bulldog_jumping = false;
  my_game->lives = 4;
  my_game->scores = 0;
  
  obstacle_index = 0;
  obstacle_cycle = 0;
  MAX_OBSTACLE_INDEX = strlen_P(obstacles) - 6;
  
  previous_millis = millis();

  clearLEDBoard();
  matrix.drawPixel(0, 0, matrix.Color333(7, 0, 0));
  matrix.drawPixel(0, matrix.width()-1, matrix.Color333(0, 7, 0));
  matrix.drawPixel(matrix.height()-1, 0, matrix.Color333(0, 0, 7));
  fillLEDLetters();
  fillLEDBulldog();

  check_mem();
}

void loop() {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= delay_millis) {
    int read_val = Serial.read();
    Serial.println(read_val);
//    clearDisplayMat();
    // clearLEDBoard();
    // When spacebar is pressed.
    if (read_val == 'A') {
      my_game->bulldog_jumping = true;
      my_game->bulldog_altitude = 8;
    }
//    int char_under_bulldog_index = obstacle_index + (int) ((BULLDOG_LEFT_OFFSET + BULLDOG_SIZE - BULLDOG_FRONT_EAR_OFFSET + obstacle_cycle) / 6);
//    unsigned char char_under_bulldog = (unsigned char) pgm_read_byte(&(obstacles[char_under_bulldog_index]));
//    if (char_under_bulldog != '_') {
//      if (my_game->bulldog_altitude < 7) {
//        my_game->lives--;
//      }
//    }
//    if (my_game->bulldog_jumping) {
//      if (cur_jumping_idx == JUMP_SEQUENCE_LENGTH) {
//        my_game->bulldog_jumping = false;
//        cur_jumping_idx = 0;
//      } else if (cur_jumping_idx == LAND_ON_OBSTACLE_CHECK) {
//        if (char_under_bulldog == '_' && obstacle_cycle == 5) {
//          cur_jumping_idx += 1;
//        }
//      } else {
//        cur_jumping_idx += 1;
//      }
//    }
//    my_game->bulldog_altitude = bulldog_jump_heights[cur_jumping_idx];
    
    fillLEDLetters();
    fillLEDBulldog();
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
    
//    Serial.write(12);
//    printDisplayMat();
//    Serial.println((char)char_under_bulldog);
//    Serial.println(my_game->bulldog_jumping);
    previous_millis = current_millis;
    check_mem();
  }
}


int getIndex(int i, int j) {
  return i * (DISP_SIZE + 1) + j;
}

void clearDisplayMat() {
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 32; j++) {
      my_game->disp_mat[getIndex(i, j)] = '-';
    }
  }
}

void clearLEDBoard() {
  for (int i = 0; i < matrix.height(); i++) {
    for (int j = 0; j < matrix.width(); j++) {
      matrix.drawPixel(i, j, 0);
    }  
  } 
}

void printDisplayMat() {
  Serial.println(my_game->disp_mat);
}

void fillBulldog() {
  for (int i = 0; i < BULLDOG_SIZE; i++) {
    for (int j = 0; j < BULLDOG_SIZE; j++) {
      if (bulldog[i][j] != '_') {
        my_game->disp_mat[getIndex(BULLDOG_TOP_OFFSET - my_game->bulldog_altitude + i, BULLDOG_LEFT_OFFSET + j)] = bulldog[i][j];
      }
    }
  }  
}

void fillLEDBulldog() {
  unsigned char bulldog_char;
  for (int i = 0; i < BULLDOG_SIZE; i++) {
    for (int j = 0; j < BULLDOG_SIZE; j++) {
      bulldog_char =  (unsigned char) pgm_read_byte(&(bulldog[i][j]));
      if ( bulldog_char == '_') {
        color = 0;
      } else if ( bulldog_char == 'W') {
        color = matrix.Color333(7, 7, 7);
      } else if ( bulldog_char == 'B') {
        color = matrix.Color333(0, 0, 7);
      }
      matrix.drawPixel((BULLDOG_SIZE - 1) - i + my_game->bulldog_altitude, BULLDOG_LEFT_OFFSET + j, color);
    }
  }  
}

void fillLetters() {
  char current_letter;
  int current_column;
  unsigned char column_bitmap;
  unsigned char current_bit;
  int obstacle_top_offset = 25;
  for (int j = 0; j < 32; j++) {
    current_letter = obstacles[obstacle_index + (int) ((j + obstacle_cycle) / 6)];
    if (current_letter == '_') {
      continue;
    }
    current_column = (j + obstacle_cycle) % 6;
    if (current_column == 5) {
      continue;
    }
    column_bitmap = 0;//Font5x7[((current_letter - 0x20) * 5) + current_column];
    for (int i = 0; i < 7; i++) {
      current_bit = column_bitmap & 0x1;
      column_bitmap >>= 1;
      if (current_bit == 0x1) {
        my_game->disp_mat[getIndex(obstacle_top_offset + i, j)] = 'X';
      }
    }
  }
}

void fillLEDLetters() {
  char current_letter;
  int current_column;
  unsigned char column_bitmap;
  unsigned char current_bit;
  int obstacle_top_offset = 25;
  color = matrix.Color333(7, 0, 7);
  for (int j = 0; j < 32; j++) {
   // (unsigned char) pgm_read_byte(&(LetterFont[(('A' - 'A') * 5) + 0]));
    current_letter =  (unsigned char) pgm_read_byte(&(obstacles[obstacle_index + (int) ((j + obstacle_cycle) / 6)]));
    //current_letter = 'H';
    current_column = (j + obstacle_cycle) % 6;
    if (current_letter == '_' || current_column == 5) {
      column_bitmap = 0;
    } else {
      column_bitmap = (unsigned char) pgm_read_byte(&(LetterFont[((current_letter - 'A') * 5) + current_column]));
    }
    //Serial.print(column_bitmap);
    for (int i = 0; i < 7; i++) {
      current_bit = column_bitmap & 0x1;
      column_bitmap >>= 1;
      if (current_bit == 0x1) {
        matrix.drawPixel((BULLDOG_SIZE - 1) - i, j, color);
      } else {
        matrix.drawPixel((BULLDOG_SIZE - 1) - i, j, 0);
      }
    }
  }
}
