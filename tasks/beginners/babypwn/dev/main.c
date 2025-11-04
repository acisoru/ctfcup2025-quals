#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#define BOARD_SIZE 10
#define SHIP_COUNT 5

typedef struct {
    int size;
    int hits;
    char name[20];
} Ship;

#define EMPTY 0
#define SHIP 1
#define HIT 2
#define MISS 3

void win();

void init_board(uint8_t board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = EMPTY;
        }
    }
}

void display_board(uint8_t board[BOARD_SIZE][BOARD_SIZE], int show_ships, uint8_t hit_mark) {
    printf("\n   ");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", i);
    }
    printf("\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%2d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == EMPTY) {
                printf(" . ");
            } else if (board[i][j] == SHIP) {
                if (show_ships) {
                    printf(" S ");
                } else {
                    printf(" . ");
                }
            } else if (board[i][j] == MISS) {
                printf(" O ");
            } else {
                printf(" %c ", board[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

int can_place_ship(uint8_t board[BOARD_SIZE][BOARD_SIZE], int row, int col, int size, int direction) {
    if (direction == 0) {
        if (col + size > BOARD_SIZE) return 0;
        for (int i = 0; i < size; i++) {
            if (board[row][col + i] != EMPTY) return 0;
        }
    } else {
        if (row + size > BOARD_SIZE) return 0;
        for (int i = 0; i < size; i++) {
            if (board[row + i][col] != EMPTY) return 0;
        }
    }
    return 1;
}

void place_ship(uint8_t board[BOARD_SIZE][BOARD_SIZE], int row, int col, int size, int direction) {
    if (direction == 0) {
        for (int i = 0; i < size; i++) {
            board[row][col + i] = SHIP;
        }
    } else {
        for (int i = 0; i < size; i++) {
            board[row + i][col] = SHIP;
        }
    }
}

void place_ships_randomly(uint8_t board[BOARD_SIZE][BOARD_SIZE], Ship ships[SHIP_COUNT]) {
    srand(time(NULL));
    
    int ship_sizes[] = {5, 4, 3, 3, 2};
    char ship_names[][20] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};
    
    for (int i = 0; i < SHIP_COUNT; i++) {
        ships[i].size = ship_sizes[i];
        ships[i].hits = 0;
        strcpy(ships[i].name, ship_names[i]);
        
        int placed = 0;
        while (!placed) {
            int row = rand() % BOARD_SIZE;
            int col = rand() % BOARD_SIZE;
            int direction = rand() % 2;

            if (can_place_ship(board, row, col, ships[i].size, direction)) {
                place_ship(board, row, col, ships[i].size, direction);
                placed = 1;
            }
        }
    }
}

int all_ships_sunk(Ship ships[SHIP_COUNT]) {
    for (int i = 0; i < SHIP_COUNT; i++) {
        if (ships[i].hits < ships[i].size) {
            return 0;
        }
    }
    return 1;
}

int process_shot(uint8_t board[BOARD_SIZE][BOARD_SIZE], Ship ships[SHIP_COUNT], int row, int col, uint8_t hit_mark) {
    if (board[row][col] == hit_mark || board[row][col] == MISS) {
        printf("You already shot at this position!\n");
        return 0;
    }
    
    if (board[row][col] != EMPTY && board[row][col] != hit_mark) {
        board[row][col] = hit_mark;
        printf("HIT! You hit a ship!\n");
        
        for (int i = 0; i < SHIP_COUNT; i++) {
            if (ships[i].hits < ships[i].size) {
                ships[i].hits++;
                if (ships[i].hits == ships[i].size) {
                    printf("You sunk the %s!\n", ships[i].name);
                }
                break;
            }
        }
        return 1;
    } else {
        board[row][col] = MISS;
        printf("MISS! You missed.\n");
        return 1;
    }
}

void game() {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
    Ship ships[SHIP_COUNT];
    int shots = 0;
    int max_shots = 5;
    uint8_t hit_mark;
    
    printf("=== BATTLESHIP GAME ===\n");
    printf("Welcome to Battleship! Try to sink all 5 ships.\n");
    printf("Ships: Carrier(5), Battleship(4), Cruiser(3), Submarine(3), Destroyer(2)\n");
    printf("Enter coordinates as 'row col' (0-9). You have %d shots.\n", max_shots);
    printf("Enter your custom HIT mark (0-255): ");
    
    int temp_hit_mark;
    if (scanf("%d", &temp_hit_mark) != 1) {
        printf("Invalid input! Using default 'X' mark.\n");
        hit_mark = 'X';
    } else if (temp_hit_mark < 0 || temp_hit_mark > 255) {
        printf("Invalid range! Using default 'X' mark.\n");
        hit_mark = 'X';
    } else {
        hit_mark = (uint8_t)temp_hit_mark;
    }
    
    printf("Using HIT mark: %c (ASCII %d)\n\n", hit_mark, hit_mark);
    
    init_board(board);
    place_ships_randomly(board, ships);
    
    while (shots < max_shots && !all_ships_sunk(ships)) {
        display_board(board, 0, hit_mark);
        
        printf("Shots remaining: %d\n", max_shots - shots);
        printf("Enter coordinates (row col): ");
        
        int row, col;
        if (scanf("%d %d", &row, &col) != 2) {
            printf("Invalid input! Please enter two numbers.\n");
            while (getchar() != '\n');
            continue;
        }
        
        if (process_shot(board, ships, row, col, hit_mark)) {
            shots++;
        }
        
        printf("\n");
    }
    
    if (all_ships_sunk(ships)) {
        printf("🎉 CONGRATULATIONS! You sunk all ships!\n");
        printf("You won in %d shots!\n", shots);
        win();
    } else {
        printf("💥 GAME OVER! You ran out of shots.\n");
        printf("Here's where the ships were:\n");
        display_board(board, 1, hit_mark);
    }
}

int main() {
    game();
    return 0;
}

void win() {
    system("cat ./flag.txt");
};
