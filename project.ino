// All characters are fixed as a 5x7 matrix.
#include <string.h>

#include "font5x7.h"

#define DISP_SIZE (32)
#define BULLDOG_SIZE (7)
#define BULLDOG_FRONT_EAR_OFFSET (2)
#define BULLDOG_LEFT_OFFSET (3)
#define BULLDOG_TOP_OFFSET (26)
#define JUMP_SEQUENCE_LENGTH (19)
#define LAND_ON_OBSTACLE_CHECK (11)
#define MAX_OBSTACLE_CYCLE (6)
int MAX_OBSTACLE_INDEX;

typedef struct gameState {
  char* disp_mat;
  int bulldog_height;
} gameState;

char bulldog[BULLDOG_SIZE][BULLDOG_SIZE + 1] = {
  "__W___W",
  "___WWW_",
  "___WWW_",
  "WBWBWW_",
  "_WBWW__",
  "_WBWW__",
  "__W_W__"
};

int bulldog_jump_heights[JUMP_SEQUENCE_LENGTH] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
char* obstacles = (char*)
"______HELLO______";
int obstacle_index;
int obstacle_cycle; // Each character takes up 5 columns.
bool game_over;

unsigned long previous_millis;
unsigned long current_millis;
unsigned long delay_millis = 250;

gameState* my_game;
bool bulldog_jumping = false;
int cur_jumping_idx = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(500000);
  my_game = (gameState*) malloc(sizeof(gameState));
  my_game->disp_mat = (char*) malloc(DISP_SIZE * (DISP_SIZE + 1) * sizeof(char));
  for (int i = 0; i < DISP_SIZE - 1; i++) {
    my_game->disp_mat[getIndex(i, DISP_SIZE)] = '\n';
  }
  my_game->disp_mat[getIndex(DISP_SIZE - 1, DISP_SIZE)] = '\0';
  my_game->bulldog_height = 0;

  obstacle_index = 0;
  obstacle_cycle = 0;
  MAX_OBSTACLE_INDEX = strlen(obstacles) - 6;
  
  clearDisplayMat();
//  fillBulldog();
  fillLetter();
  printDisplayMat();
  
  previous_millis = millis();
  game_over = false;
}

void loop() {
  unsigned long current_millis = millis();
  if (current_millis - previous_millis >= delay_millis) {
    int read_val = Serial.read();
    clearDisplayMat();
    // When spacebar is pressed.
    if (read_val == 10) {
      bulldog_jumping = true;
    }
    int char_under_bulldog_index = obstacle_index + (int) ((BULLDOG_LEFT_OFFSET + BULLDOG_SIZE - BULLDOG_FRONT_EAR_OFFSET + obstacle_cycle) / 6);
    int char_under_bulldog = obstacles[char_under_bulldog_index];
    if (char_under_bulldog != '_') {
      if (my_game->bulldog_height < 7) {
        game_over = true;
      }
    }
    if (bulldog_jumping) {
      if (cur_jumping_idx == JUMP_SEQUENCE_LENGTH) {
        bulldog_jumping = false;
        cur_jumping_idx = 0;
      } else if (cur_jumping_idx == LAND_ON_OBSTACLE_CHECK) {
        if (char_under_bulldog == '_' && obstacle_cycle == 5) {
          cur_jumping_idx += 1;
        }
      } else {
        cur_jumping_idx += 1;
      }
    }
    my_game->bulldog_height = bulldog_jump_heights[cur_jumping_idx];
    
    fillLetter();
    fillBulldog();
    obstacle_cycle++;
    if (obstacle_cycle >= MAX_OBSTACLE_CYCLE) {
      obstacle_cycle = 0;
      obstacle_index++;
      if (obstacle_index >= MAX_OBSTACLE_INDEX) {
        obstacle_index = 0;
        game_over = false;
      }
    }
    
    Serial.write(12);
    printDisplayMat();
    Serial.println((char)char_under_bulldog);
    Serial.println(game_over);
    previous_millis = current_millis;
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

void printDisplayMat() {
  Serial.println(my_game->disp_mat);
}

void fillBulldog() {
  for (int i = 0; i < BULLDOG_SIZE; i++) {
    for (int j = 0; j < BULLDOG_SIZE; j++) {
      if (bulldog[i][j] != '_') {
        my_game->disp_mat[getIndex(BULLDOG_TOP_OFFSET - my_game->bulldog_height + i, BULLDOG_LEFT_OFFSET + j)] = bulldog[i][j];
      }
    }
  }  
}

void fillLetter() {
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
    column_bitmap = Font5x7[((current_letter - 0x20) * 5) + current_column];
    for (int i = 0; i < 7; i++) {
      current_bit = column_bitmap & 0x1;
      column_bitmap >>= 1;
      if (current_bit == 0x1) {
        my_game->disp_mat[getIndex(obstacle_top_offset + i, j)] = 'X';
      }
    }
  }
}
