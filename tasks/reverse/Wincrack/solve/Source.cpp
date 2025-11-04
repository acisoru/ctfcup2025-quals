#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <map>
#include <unordered_map>
#include <set>
#include <deque>
#include <string>
#include <fstream>
#include <windows.h>

// Markov Chain State for key generation
class MarkovChain {
private:
    std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> transitions;
    std::mt19937 rng;
    uint32_t current_state;

public:
    MarkovChain(uint64_t seed) : rng(seed), current_state(0x1337) {
        // Initialize transition probabilities based on seed
        for (int i = 0; i < 256; ++i) {
            for (int j = 0; j < 256; ++j) {
                std::vector<uint32_t> next_states;
                for (int k = 0; k < 8; ++k) {
                    next_states.push_back((i * j + k + seed) % 0x10000);
                }
                transitions[{i, j}] = next_states;
            }
        }
    }

    uint32_t getNextState() {
        auto key = std::make_pair(current_state & 0xFF, (current_state >> 8) & 0xFF);
        if (transitions.find(key) != transitions.end()) {
            auto& possible_states = transitions[key];
            current_state = possible_states[rng() % possible_states.size()];
        }
        else {
            current_state = (current_state * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
        }
        return current_state;
    }
};

// Feistel Cipher Implementation
class FeistelCipher {
private:
    std::vector<uint32_t> round_keys;
    MarkovChain markov;

    uint32_t feistel_function(uint32_t right, uint32_t round_key) {
        // Complex Feistel function with multiple operations
        uint32_t temp = right;
        temp = ((temp << 13) | (temp >> 19)) ^ round_key;  // Rotate and XOR
        temp = ((temp * 0x9E3779B9) ^ 0x85EBCA6B) + 0x1337; // Multiply and add
        temp = ((temp << 7) | (temp >> 25)) ^ (round_key >> 8); // Another rotation
        return temp ^ (round_key & 0xFF);
    }

public:
    FeistelCipher(const std::vector<uint64_t>& seeds) : markov(seeds[0]) {
        // Generate round keys using Markov chain
        round_keys.reserve(16);
        for (int i = 0; i < 16; ++i) {
            round_keys.push_back(markov.getNextState());
        }
    }

    std::vector<uint32_t> encrypt(const std::vector<uint32_t>& plaintext) {
        std::vector<uint32_t> ciphertext = plaintext;

        for (int round = 0; round < 16; ++round) {
            for (size_t i = 0; i < ciphertext.size(); i += 2) {
                if (i + 1 < ciphertext.size()) {
                    uint32_t left = ciphertext[i];
                    uint32_t right = ciphertext[i + 1];

                    uint32_t new_left = right;
                    uint32_t new_right = left ^ feistel_function(right, round_keys[round]);

                    ciphertext[i] = new_left;
                    ciphertext[i + 1] = new_right;
                }
            }
        }

        return ciphertext;
    }

    std::vector<uint32_t> decrypt(const std::vector<uint32_t>& ciphertext) {
        std::vector<uint32_t> plaintext = ciphertext;

        for (int round = 15; round >= 0; --round) {
            for (size_t i = 0; i < plaintext.size(); i += 2) {
                if (i + 1 < plaintext.size()) {
                    uint32_t left = plaintext[i];
                    uint32_t right = plaintext[i + 1];

                    uint32_t new_left = right ^ feistel_function(left, round_keys[round]);
                    uint32_t new_right = left;

                    plaintext[i] = new_left;
                    plaintext[i + 1] = new_right;
                }
            }
        }

        return plaintext;
    }
};

// Encrypted flag storage using complex STL containers
static std::map<std::string, std::vector<std::vector<uint32_t>>> encrypted_flag_storage;
static std::unordered_map<std::string, std::deque<uint64_t>> key_derivation_cache;
static std::set<std::string> processed_flags;

class FlagIniter {
public:
    FlagIniter() {
        std::vector<uint32_t> encrypted;
        encrypted.reserve(16);
        encrypted.push_back(1788248053);
        encrypted.push_back(2925656205);
        encrypted.push_back(2277918373);
        encrypted.push_back(944115957);
        encrypted.push_back(1936439195);
        encrypted.push_back(750110178);
        encrypted.push_back(1449301998);
        encrypted.push_back(206726758);
        encrypted.push_back(2202718213);
        encrypted.push_back(3211734886);
        encrypted.push_back(1853377771);
        encrypted.push_back(1625962532);
        encrypted.push_back(1113283355);
        encrypted.push_back(3945480845);
        encrypted.push_back(2521685304);
        encrypted.push_back(777364575);
        encrypted_flag_storage["layer_0"].push_back(encrypted);
    };
};

FlagIniter flg;

// Anti-analysis obfuscation functions
uint32_t Xor(uint32_t a, uint32_t b) {
    return a ^ b;
}

uint64_t GenerateDynamicSeed(uint64_t base_seed, uint64_t& counter) {
    // Generate deterministic seed (no time dependency for consistency)
    counter++;

    // Use deterministic approach instead of system time
    uint64_t deterministic_seed = base_seed + counter;
    uint64_t dynamic_seed = (deterministic_seed ^ 0x9E3779B9) + counter;

    // Additional obfuscation
    dynamic_seed = ((dynamic_seed << 17) | (dynamic_seed >> 47)) ^ 0x48505a4542545950;

    /*LogToFile("GenerateDynamicSeed: base_seed=" + std::to_string(base_seed) +
              ", counter=" + std::to_string(counter) +
              ", result=" + std::to_string(dynamic_seed));*/

    return dynamic_seed;
}


int main() {
    uint64_t base_seed = 1136547294; // Simple fixed seed
    uint64_t counter = 0;

    // Generate same dynamic seed as in initialization
    uint64_t dynamic_seed = GenerateDynamicSeed(base_seed, counter);
    std::vector<uint64_t> layer_seeds = { dynamic_seed };

    FeistelCipher cipher(layer_seeds);
    //input_data = encrypted_flag_storage["layer_0"][0];
    // Single encryption pass to match initialization
    std::vector<uint32_t> decrypted_input = cipher.decrypt(encrypted_flag_storage["layer_0"][0]);

    //LogToFile("Input encrypted:");
    std::string flag;

    for (size_t k = 0; k < decrypted_input.size(); ++k) {
        decrypted_input[k] = Xor(decrypted_input[k], 0x03003003);
        uint32_t val = decrypted_input[k];
        char syms[5];
        memcpy(syms, &val, 4);
        syms[4] = '\0';
        flag += syms;
    }
    std::cout << flag << std::endl;
  
    return -1;
};
