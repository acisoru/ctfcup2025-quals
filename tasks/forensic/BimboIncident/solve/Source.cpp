
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

BOOL _DecryptFile(const std::string& filePath) {
    // Read file content
    std::ifstream file(filePath, std::ios::binary);

    char xorConstant[] = { 'S', 'N', 'E', 'Z', 'H', 'A', 'N', 'A',
                            'M', 'A', 'L', 'V', 'A', 'R', 'E', 'V',
                            'A', '\0' };

    if (!file.is_open()) {
        return FALSE;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 16) {
        file.close();
        return FALSE; // File too small for key derivation
    }

    // Read file content
    uint8_t first_16_bytes_of_png[] = { 0x89, 0x50, 0x4E, 0x47, 0xD, 0xA, 0x1A, 0xA, 0x0, 0x0, 0x0, 0xD, 0x49, 0x48, 0x44, 0x52 };

    std::vector<BYTE> fileData(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    file.close();

    // Derive key from first 16 bytes XORed with constant
    BYTE derivedKey[16];
    for (int i = 0; i < 16; i++) {
        derivedKey[i] = first_16_bytes_of_png[i] ^ xorConstant[i];
    }

    // Initialize Crypto API
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHash = 0;
    BOOL result = FALSE;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return FALSE;
    }

    // Create hash object
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return FALSE;
    }

    // Hash the derived key
    if (!CryptHashData(hHash, derivedKey, 16, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        //std::cout << "137" << std::endl;
        return FALSE;
    }

    // Derive AES key from hash
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        //std::cout << "145" << std::endl;
        return FALSE;
    }

    if ((fileSize % 16) != 0) {
        fileSize += 16 - (fileSize % 16);
    }

    DWORD dataSize = fileSize;
    BYTE* dataToEncrypt = new BYTE[dataSize];
    memset(dataToEncrypt, 0x0, dataSize);
    memcpy(dataToEncrypt, fileData.data(), fileData.size());

    if (!CryptDecrypt(hKey, 0, 0, 0, dataToEncrypt, &dataSize)) {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return FALSE;
    }

    // Write encrypted data back to file
    std::ofstream outFile(filePath + "_decrypted.png", std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<char*>(dataToEncrypt), dataSize);
        outFile.close();
        result = TRUE;
    }

    // Cleanup
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    //std::cout << "TRUE!" << std::endl;

    return result;
}

int main(int arc, char** argv) {
    _DecryptFile(std::string(argv[1]));
}