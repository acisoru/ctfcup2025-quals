#pragma once
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

static std::vector<uint64_t> signature_bytes;
uint64_t* InitCryptoConsts(uint64_t* seeds, HANDLE hHeap);
uint64_t RealFlagChecker(std::wstring UserInput);
PVOID FindFunctionByHash(DWORD ModuleHash, DWORD FunctionHash);
static const void* const gPtr = &InitCryptoConsts;
void ExampleUsage();
PVOID FindFunctionByHash(DWORD ModuleHash, DWORD FunctionHash);
DWORD HashStringW(LPCWSTR String);
DWORD HashStringA(LPCSTR String);