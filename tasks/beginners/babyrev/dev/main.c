#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

unsigned char shuffled_map[256] = {
    147, 111, 196, 102, 201, 12, 222, 43, 1, 62, 49, 178, 151, 198, 166, 112, 10, 27, 6, 128, 180, 120, 142, 187, 2, 65, 88, 39, 
    225, 186, 159, 115, 157, 243, 255, 0, 52, 119, 164, 32, 138, 253, 123, 137, 66, 177, 56, 249, 143, 7, 85, 103, 8, 248, 133, 
    51, 223, 60, 99, 98, 59, 146, 25, 247, 209, 246, 179, 242, 236, 40, 183, 47, 67, 227, 193, 174, 192, 165, 97, 77, 162, 76, 
    94, 217, 71, 226, 72, 139, 20, 44, 79, 109, 203, 125, 215, 214, 154, 131, 105, 89, 73, 184, 63, 245, 175, 22, 173, 145, 239, 
    23, 84, 83, 202, 9, 80, 152, 237, 200, 69, 161, 16, 195, 124, 36, 127, 41, 24, 81, 26, 116, 213, 207, 101, 160, 130, 190, 
    218, 199, 11, 185, 15, 212, 90, 87, 167, 155, 100, 232, 229, 42, 205, 191, 106, 70, 228, 172, 91, 168, 211, 104, 182, 3, 
    96, 235, 129, 107, 208, 34, 171, 238, 141, 92, 45, 114, 122, 61, 21, 110, 231, 204, 126, 230, 140, 5, 181, 210, 18, 216, 
    33, 240, 136, 206, 234, 28, 75, 220, 132, 113, 78, 135, 17, 153, 50, 144, 95, 38, 170, 64, 57, 221, 149, 37, 244, 169, 224, 
    86, 194, 252, 250, 68, 30, 254, 14, 55, 35, 197, 46, 233, 93, 82, 176, 108, 13, 118, 251, 134, 4, 74, 19, 241, 29, 54, 53, 
    150, 58, 189, 121, 148, 117, 31, 158, 188, 48, 219, 163, 156
};

// ctfcup{y0ur_f1rst_r3v3rs3_ch4ll3ng3_1s_d0n3_c0ngratulations}
unsigned char target_array[] = {
    0x02,0xd8,0x71,0x02,0xd4,0x7b,0x26,0xab,0xe5,0xd4,0x51,0xa0,0x71,0x9e,
    0x51,0x63,0xd8,0xa0,0x51,0x1b,0x27,0x1b,0x51,0x63,0x1b,0xa0,0x02,0x43,
    0xbe,0x7c,0x7c,0x1b,0x20,0xf7,0x1b,0xa0,0x9e,0x63,0xa0,0x16,0xe5,0x20,
    0x1b,0xa0,0x02,0xe5,0x20,0xf7,0x51,0x31,0xd8,0xd4,0x7c,0x31,0xd8,0x90,
    0x24,0x20,0x63,0xd9
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <flag>\nFlag format: ctfcup{<data>}", argv[0]);
        return 1;
    }
    
    char *input = argv[1];
    int input_len = strlen(input);
    int target_len = sizeof(target_array);
    
    if (input_len != target_len) {
        puts("[-] Invalid flag!");
        return 1;
    }
    
    unsigned char processed[256];
    uint32_t seed = (uint32_t)input[0] << 24 | (uint32_t)input[1] << 16 | (uint32_t)input[2] << 8 | (uint32_t)input[3];
    
    srand(seed);
    int used_indices[256] = {0};
    
    for (int i = 0; i < 256; i++) {
        int new_index;
        do {
            new_index = rand() % 256;
        } while (new_index == i || used_indices[new_index]);
        used_indices[new_index] = 1;
        unsigned char temp = shuffled_map[i];
        shuffled_map[i] = shuffled_map[new_index];
        shuffled_map[new_index] = temp;
    }

    for (int i = 0; i < input_len; i++) {
        unsigned char xored = input[i] ^ 0x42;
        processed[i] = shuffled_map[xored];    
    }

    for (int i = 0; i < target_len; i++) {
        if (processed[i] != target_array[i]) {
            puts("[-] Invalid flag!");
            return 1;
        }
    }
    
    puts("[+] Correct flag!");
    return 0;
}
