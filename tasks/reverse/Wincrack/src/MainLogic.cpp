#pragma once
#include "MainLogic.h"

uint64_t* InitCryptoConsts(uint64_t* seeds, HANDLE hHeap) {
    typedef HANDLE(WINAPI* pHeapAlloc) (
        _In_ HANDLE hHeap,
        _In_ DWORD dwFlags,
        _In_ SIZE_T dwBytes
        );

    pHeapAlloc heapAlloc = (pHeapAlloc)FindFunctionByHash(817310536, 663637877);

    uint64_t signature_bytes[1024];
    for (int i = 0; i < 1024; ++i) {
        signature_bytes[i] = (i * 0x7f5e4d) ^ 0x1f2f3f;
    }

	for (auto i = 0; i < 1024; ++i) {
		signature_bytes[i] ^= 0x1384 ^ seeds[i];
	}

	uint64_t seed = 0;
	for (int i = 0; i < 1024; ++i) {
		if (i % 2 == 0) {
			seed += signature_bytes[i] + 0x1337;
		}
		else {
			seed += signature_bytes[i] + 0x7331;
		}
	}

	std::mt19937 rd(seed);

    uint64_t* out_buf = (uint64_t*)heapAlloc(hHeap, HEAP_ZERO_MEMORY, 128 * 8);

	for (auto i = 0; i < 128; ++i){
		out_buf[i] = (rd() % UINT64_MAX);
	}

	return out_buf;
};

class MarkovChain {
private:
    std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> transitions;
    std::mt19937 rng;
    uint32_t current_state;
    
public:
    MarkovChain(uint64_t seed) : rng(seed), current_state(0x1337) {
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
        } else {
            current_state = (current_state * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
        }
        return current_state;
    }
};

class FeistelCipher {
private:
    std::vector<uint32_t> round_keys;
    MarkovChain markov;
    
    uint32_t feistel_function(uint32_t right, uint32_t round_key) {
        uint32_t temp = right;
        temp = ((temp << 13) | (temp >> 19)) ^ round_key;
        temp = ((temp * 0x9E3779B9) ^ 0x85EBCA6B) + 0x1337;
        temp = ((temp << 7) | (temp >> 25)) ^ (round_key >> 8);
        return temp ^ (round_key & 0xFF);
    }
    
public:
    FeistelCipher(const std::vector<uint64_t>& seeds) : markov(seeds[0]) {
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
};

static std::map<uint32_t, std::vector<std::vector<uint32_t>>> encrypted_flag_storage;
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
        encrypted_flag_storage[0].push_back(encrypted);
    };
};

FlagIniter flg;

uint32_t XOR(uint32_t a, uint32_t b) {
    return a ^ b;
}

uint64_t GenerateDynamicSeed(uint64_t base_seed, uint64_t& counter) {
    counter++;
    
    uint64_t deterministic_seed = base_seed + counter;
    uint64_t dynamic_seed = (deterministic_seed ^ 0x9E3779B9) + counter;
    
    dynamic_seed = ((dynamic_seed << 17) | (dynamic_seed >> 47)) ^ 0x48505a4542545950;
    
    return dynamic_seed;
}

uint64_t RealFlagChecker(std::wstring UserInput) {
   
    std::string input_str(UserInput.begin(), UserInput.end());
   
    std::vector<uint32_t> input_data;
    input_data.reserve((input_str.length() + 3) / 4);
    
    for (size_t i = 0; i < input_str.length(); i += 4) {
        uint32_t chunk = 0;
        for (int j = 0; j < 4 && i + j < input_str.length(); ++j) {
            chunk |= (static_cast<uint32_t>(input_str[i + j]) << (j * 8));
        }
        chunk = XOR(chunk, 0x3003003);
        input_data.push_back(chunk);
    }
    
    uint8_t* peb = (uint8_t*)__readgsqword(0x60);
    uint64_t* ptr = (uint64_t*)((uint8_t*)peb + 0x400);

    uint64_t base_seed = 0x0;

    for (int i = 0; i < 128; i++) {
        uint64_t val = ptr[128 + i];
        base_seed ^= val;
    }

    uint64_t counter = 0;
    
    uint64_t dynamic_seed = GenerateDynamicSeed(base_seed, counter);
    std::vector<uint64_t> layer_seeds = {dynamic_seed};
    
    FeistelCipher cipher(layer_seeds);
   
    std::vector<uint32_t> encrypted_input = cipher.encrypt(input_data);
    
    auto it = encrypted_flag_storage.find(0);
    if (it != encrypted_flag_storage.end() && !it->second.empty()) {
        const auto& stored_encrypted = it->second[0];      
        
        if (encrypted_input.size() != stored_encrypted.size()) {
            return -1;
        }
        
        bool matches = true;
        for (size_t i = 0; i < encrypted_input.size(); ++i) {
            if (encrypted_input[i] != stored_encrypted[i]) {
                matches = false;
                break;
            }
        }
        
        if (matches) {
            return 1;
        }
    } else {
    }
    
    return -1;
};

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    union {
        LIST_ENTRY HashLinks;
        struct {
            PVOID SectionPointer;
            ULONG CheckSum;
        };
    };
    union {
        ULONG TimeDateStamp;
        PVOID LoadedImports;
    };
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union {
        BOOLEAN BitField;
        struct {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsLegacyProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN SpareBits : 3;
        };
    };
    PVOID Mutant;
    PVOID ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
} PEB, *PPEB;

DWORD HashStringA(LPCSTR String) {
    DWORD Hash = 0;
    for (; *String; String++) {
        Hash = ((Hash << 5) + Hash) + *String;
    }
    return Hash;
}

DWORD HashStringW(LPCWSTR String) {
    DWORD Hash = 0;
    for (; *String; String++) {
        Hash = ((Hash << 5) + Hash) + *String;
    }
    return Hash;
}

PVOID FindFunctionByHash(DWORD ModuleHash, DWORD FunctionHash) {
    PPEB Peb = (PPEB)__readgsqword(0x60);
    PPEB_LDR_DATA Ldr = Peb->Ldr;
    
    if (!Ldr) {
        return NULL;
    }
    
    PLIST_ENTRY ModuleList = &Ldr->InLoadOrderModuleList;
    PLIST_ENTRY CurrentEntry = ModuleList->Flink;
    
    while (CurrentEntry != ModuleList) {
        PLDR_DATA_TABLE_ENTRY CurrentModule = (PLDR_DATA_TABLE_ENTRY)CurrentEntry;
        
        DWORD CurrentModuleHash = HashStringW(CurrentModule->BaseDllName.Buffer);
        
        if (CurrentModuleHash == ModuleHash) {
            PVOID ModuleBase = CurrentModule->DllBase;
            
            if (!ModuleBase) {
                CurrentEntry = CurrentEntry->Flink;
                continue;
            }
            
            PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)ModuleBase;
            if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
                CurrentEntry = CurrentEntry->Flink;
                continue;
            }
            
            PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)((PBYTE)ModuleBase + DosHeader->e_lfanew);
            if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
                CurrentEntry = CurrentEntry->Flink;
                continue;
            }
            
            PIMAGE_EXPORT_DIRECTORY ExportDir = (PIMAGE_EXPORT_DIRECTORY)((PBYTE)ModuleBase + 
                NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
            
            if (!ExportDir) {
                CurrentEntry = CurrentEntry->Flink;
                continue;
            }
            
            PDWORD AddressOfFunctions = (PDWORD)((PBYTE)ModuleBase + ExportDir->AddressOfFunctions);
            PDWORD AddressOfNames = (PDWORD)((PBYTE)ModuleBase + ExportDir->AddressOfNames);
            PWORD AddressOfNameOrdinals = (PWORD)((PBYTE)ModuleBase + ExportDir->AddressOfNameOrdinals);
            
            for (DWORD i = 0; i < ExportDir->NumberOfNames; i++) {
                LPCSTR FunctionName = (LPCSTR)((PBYTE)ModuleBase + AddressOfNames[i]);
                DWORD CurrentFunctionHash = HashStringA(FunctionName);
                
                if (CurrentFunctionHash == FunctionHash) {
                    WORD Ordinal = AddressOfNameOrdinals[i];
                    PVOID FunctionAddress = (PVOID)((PBYTE)ModuleBase + AddressOfFunctions[Ordinal]);
                    return FunctionAddress;
                }
            }
            
            break;
        }
        
        CurrentEntry = CurrentEntry->Flink;
    }
    
    return NULL;
}
