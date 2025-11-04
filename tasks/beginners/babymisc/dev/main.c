#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FLAG_COST 13371337

void setup() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    alarm(60);

    FILE *fp = fopen("/dev/urandom", "r");
    unsigned int seed;
    fread(&seed, sizeof(unsigned int), 1, fp);
    srand(seed);
    fclose(fp);
};

int main() {
    setup();

    int money = 1000;
    int bets = 10;

    for (int i = 0; i < bets; i++){
        printf("You have %d money and %d bets left\n", money, bets);
        printf("Enter your bet: ");
        
        int bet;

        scanf("%d", &bet);
        
        if (bet > money) {
            puts("You don't have enough money to bet");
            continue;
        }
        
        money -= bet;

        printf("Enter your guess: ");
        int guess;
        scanf("%d", &guess);

        if (guess < 0 || guess > 100) {
            printf("Invalid guess\n");
            continue;
        }
        if (guess == rand() % 100) {
            printf("You won! You got %d money\n", bet * 2);
            money += bet * 2;
        } else {
            printf("You lost! You lost %d money\n", bet);
            money -= bet;
        }
        bets--;
    }

    if (money >= FLAG_COST) {
        puts("You have enough money to buy the flag");
        FILE *fp = fopen("flag.txt", "r");

        if (fp == NULL) {
            puts("Error: flag.txt not found");
            return 1;
        }
        char flag[100];
        fscanf(fp, "%s", flag);
        printf("Flag: %s\n", flag);
        fclose(fp);
    } else {
        puts("You don't have enough money to buy the flag");
    }
}