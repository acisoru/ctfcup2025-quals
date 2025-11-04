#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <winternl.h>
#include <intrin.h>

namespace AntiPatch {
    static uint32_t CalculateChecksum(void* data, size_t size) {
        uint32_t checksum = 0x12345678;
        uint8_t* bytes = (uint8_t*)data;
        for (size_t i = 0; i < size; i++) {
            checksum = _rotl(checksum, 3) ^ bytes[i];
        }
        return checksum;
    }
    
    static bool DecoyIsDebugged() {
        return false;
    }
    
    static int DecoyCheckDebug() {
        return 0;
    }
    
    static bool (*GetCheckFunction())(void) {
        static bool initialized = false;
        static bool (*checkFunc)(void) = nullptr;
        
        if (!initialized) {
            volatile int selector = 0;
            for (int i = 0; i < 1000; i++) {
                selector += i * 3;
            }
            selector = selector % 2;
            
            if (selector == 0) {
                checkFunc = []() -> bool { return true; };
            } else {
                checkFunc = DecoyIsDebugged;
            }
            initialized = true;
        }
        return checkFunc;
    }
}

class AntiDebug {
private:
    static bool _0x4A7B2C8D() { return CheckRemoteDebuggerPresent(); }
    static bool _0x8E9F1A3B() { return CheckPEB(); }
    static bool _0x2C5D6E7F() { return CheckDebugObject(); }
    static bool _0x9A1B4C8E() { return CheckDebuggerBreakpoints(); }
    static bool _0x3F7E2A9D() { return CheckTiming(); }
    static bool _0x6B8C5E1F() { return CheckSystemDebugger(); }
    
    static bool VerifyIntegrity() {
        static uint32_t expectedChecksum = 0;
        if (expectedChecksum == 0) {
            expectedChecksum = AntiPatch::CalculateChecksum((void*)IsDebugged, 64);
            return true;
        }
        
        uint32_t currentChecksum = AntiPatch::CalculateChecksum((void*)IsDebugged, 64);
        return currentChecksum == expectedChecksum;
    }
    
    static bool RedundantCheck1() {
        BOOL isDebugged = FALSE;
        HANDLE hProcess = GetCurrentProcess();
        isDebugged = CheckRemoteDebuggerPresent();
        return isDebugged != FALSE;
    }
    
    static bool RedundantCheck2() {
        PEB* peb = (PEB*)__readgsqword(0x60);
        return peb->BeingDebugged != 0;
    }
    
    static bool DynamicCheck() {
        static int callCount = 0;
        callCount++;
        
        if (callCount % 3 == 0) {
            return CheckRemoteDebuggerPresent();
        } else if (callCount % 3 == 1) {
            return CheckPEB();
        } else {
            return CheckDebugObject();
        }
    }

public:
    static bool IsDebugged() {
        if (!VerifyIntegrity()) {
            return true;
        }
        
        static bool measuresInstalled = false;
        if (!measuresInstalled) {
            InstallAntiPatchMeasures();
            InstallSelfModifyingCode();
            measuresInstalled = true;
        }
        
        bool result1 = _0x4A7B2C8D() || _0x8E9F1A3B() || _0x2C5D6E7F();
        bool result2 = _0x9A1B4C8E() || _0x3F7E2A9D() || _0x6B8C5E1F();
        
        bool result3 = RedundantCheck1() || RedundantCheck2();
        
        bool result4 = DynamicCheck();
        
        bool result5 = PolymorphicCheck();
        
        AntiPatch::DecoyIsDebugged();
        AntiPatch::DecoyCheckDebug();
 
        return result1 || result2 || result3 || result4 || result5;
    }
    
    static bool IsBeingDebugged() {
        return (
            CheckRemoteDebuggerPresent() ||
            CheckPEB() ||
            CheckDebugObject() ||
            CheckDebuggerBreakpoints() ||
            CheckTiming() ||
            CheckSystemDebugger()
        );
    }

    static void Defend() {
        RemoveDebugPrivileges();
        SelfDebugging();
        InstallIntegrityMonitor();
        ScatterDecoyFunctions();
    }
    
    static void InstallIntegrityMonitor() {
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            while (true) {
                Sleep(1000 + (rand() % 2000));
                
                if (!VerifyIntegrity()) {
                    ExitProcess(0xDEADBEEF);
                }
                
                if (IsDebuggerPresent()) {
                    ExitProcess(0xDEADBEEF);
                }
            }
            return 0;
        }, nullptr, 0, nullptr);
    }
    
    static void ScatterDecoyFunctions() {
        static auto decoy1 = []() -> bool { return false; };
        static auto decoy2 = []() -> bool { return true; };
        static auto decoy3 = []() -> bool { 
            volatile int x = 0;
            for (int i = 0; i < 1000; i++) x += i;
            return x > 0;
        };
        
        decoy1(); decoy2(); decoy3();
    }

private:
    static bool CheckRemoteDebuggerPresent() {
        BOOL isDebugged = FALSE;
        ::CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebugged);
        return isDebugged != FALSE;
    }

    static bool CheckPEB() {
        PEB* peb = (PEB*)__readgsqword(0x60);
        return peb->BeingDebugged != 0;
    }

    static bool CheckDebugObject() {
        typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
            HANDLE, ULONG, PVOID, ULONG, PULONG);

        auto NtQueryInformationProcess = (_NtQueryInformationProcess)
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

        HANDLE debugObject = nullptr;
        NTSTATUS status = NtQueryInformationProcess(
            GetCurrentProcess(),
            0x1e, // ProcessDebugObjectHandle
            &debugObject,
            sizeof(HANDLE),
            nullptr);

        if (status == 0 && debugObject != nullptr) {
            CloseHandle(debugObject);
            return true;
        }

        return false;
    }


    static bool CheckDebuggerBreakpoints() {
        BYTE* entryPoint = (BYTE*)GetModuleHandle(nullptr);
        IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)entryPoint;
        IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(entryPoint + dosHeader->e_lfanew);
        BYTE* code = entryPoint + ntHeaders->OptionalHeader.AddressOfEntryPoint;

        return *code == 0xCC;
    }

    static bool CheckTiming() {
        auto start = std::chrono::high_resolution_clock::now();
        volatile int64_t counter = 0;
        for (int i = 0; i < 1000000; ++i) counter += i;
        auto end = std::chrono::high_resolution_clock::now();

        return (end - start) > std::chrono::milliseconds(10);
    }

    static bool CheckSystemDebugger() {
        struct SYSTEM_KERNEL_DEBUGGER_INFORMATION {
            BOOLEAN 	KernelDebuggerEnabled;
            BOOLEAN 	KernelDebuggerNotPresent;
        };

        typedef NTSTATUS(WINAPI* pNtQuerySystemInformation)(
            __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
            __inout PVOID SystemInformation,
            __in ULONG SystemInformationLength,
            __out_opt PULONG ReturnLength
            );


        auto _NtQuerySystemInformation = (pNtQuerySystemInformation)
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");

        SYSTEM_KERNEL_DEBUGGER_INFORMATION info;
        NTSTATUS status = _NtQuerySystemInformation(
            (SYSTEM_INFORMATION_CLASS)0x23,
            &info,
            sizeof(info),
            nullptr);
        return info.KernelDebuggerEnabled;
    }

    static void RemoveDebugPrivileges() {
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
            TOKEN_PRIVILEGES tp;
            tp.PrivilegeCount = 1;
            LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid);
            tp.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
            CloseHandle(hToken);
        }
    }

    static void SelfDebugging() {
        if (IsDebuggerPresent()) return;

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        TCHAR path[MAX_PATH];
        GetModuleFileName(nullptr, path, MAX_PATH);

        if (CreateProcess(path, nullptr, nullptr, nullptr, FALSE, DEBUG_PROCESS, nullptr, nullptr, &si, &pi)) {
            DebugActiveProcessStop(GetCurrentProcessId());
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
    
    static void InstallAntiPatchMeasures() {
        static bool (*realIsDebugged)() = IsDebugged;
        static bool (*obfuscatedPtr)() = nullptr;
        
        volatile uintptr_t ptr = (uintptr_t)realIsDebugged;
        ptr ^= 0xDEADBEEF;
        ptr = _rotl(ptr, 13);
        obfuscatedPtr = (bool(*)())(ptr ^ 0xDEADBEEF);
        obfuscatedPtr = (bool(*)())_rotr((uintptr_t)obfuscatedPtr, 13);
        
        AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS pExceptionInfo) -> LONG {
            if (pExceptionInfo->ExceptionRecord->ExceptionAddress >= (PVOID)IsDebugged &&
                pExceptionInfo->ExceptionRecord->ExceptionAddress < (PVOID)((uintptr_t)IsDebugged + 256)) {
                ExitProcess(0);
            }
            return EXCEPTION_CONTINUE_SEARCH;
        });
        
        DWORD oldProtect;
        VirtualProtect((LPVOID)IsDebugged, 256, PAGE_EXECUTE_READ, &oldProtect);
    }
    
    static bool PolymorphicCheck() {
        static int polymorphicCounter = 0;
        polymorphicCounter++;
        
        switch (polymorphicCounter % 4) {
            case 0: {
                BOOL isDebugged = FALSE;
                isDebugged = CheckRemoteDebuggerPresent();
                return isDebugged != FALSE;
            }
            case 1: {
                PEB* peb = (PEB*)__readgsqword(0x60);
                return peb->BeingDebugged != 0;
            }
            case 2: {
                auto start = std::chrono::high_resolution_clock::now();
                volatile int64_t counter = 0;
                for (int i = 0; i < 100000; ++i) counter += i;
                auto end = std::chrono::high_resolution_clock::now();
                return (end - start) > std::chrono::milliseconds(5);
            }
            case 3: {
                typedef NTSTATUS(NTAPI* _NtQueryInformationProcess)(
                    HANDLE, ULONG, PVOID, ULONG, PULONG);
                auto NtQueryInformationProcess = (_NtQueryInformationProcess)
                    GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
                HANDLE debugObject = nullptr;
                NTSTATUS status = NtQueryInformationProcess(
                    GetCurrentProcess(), 0x1e, &debugObject, sizeof(HANDLE), nullptr);
                if (status == 0 && debugObject != nullptr) {
                    CloseHandle(debugObject);
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    
    static void InstallSelfModifyingCode() {
        static bool installed = false;
        if (installed) return;
        
        LPVOID execMem = VirtualAlloc(nullptr, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (execMem) {
            memcpy(execMem, (void*)IsDebugged, 256);
            
            uint8_t* code = (uint8_t*)execMem;
            for (int i = 0; i < 256; i++) {
                if (code[i] == 0x90) { 
                    code[i] = 0xCC; 
                }
            }
            
            DWORD oldProtect;
            VirtualProtect(execMem, 1024, PAGE_EXECUTE_READ, &oldProtect);
        }
        installed = true;
    }
    
    static void InstallDynamicProtection() {
        for (int i = 0; i < 3; i++) {
            CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
                int threadId = (int)(uintptr_t)param;
                
                while (true) {
                    Sleep(250 + (threadId * 200) + (rand() % 100));
                    
                    switch (threadId) {
                        case 0: {
                            if (!VerifyIntegrity()) {
                                ExitProcess(0);
                            }
                            break;
                        }
                        case 1: {
                            if (IsDebuggerPresent() || CheckRemoteDebuggerPresent()) {
                                ExitProcess(0);
                            }
                            break;
                        }
                        case 2: {
                            static uint8_t originalPattern[64];
                            static bool patternSet = false;
                            
                            if (!patternSet) {
                                memcpy(originalPattern, (void*)IsDebugged, 64);
                                patternSet = true;
                            } else {
                                if (memcmp(originalPattern, (void*)IsDebugged, 64) != 0) {
                                    ExitProcess(0);
                                }
                            }
                            break;
                        }
                    }
                }
                return 0;
            }, (LPVOID)(uintptr_t)i, 0, nullptr);
        }
    }

public:
    static void InitializeAllProtections() {
        InstallDynamicProtection();
        InstallIntegrityMonitor();
        ScatterDecoyFunctions();
        InstallAntiPatchMeasures();
        InstallSelfModifyingCode();
    }
};