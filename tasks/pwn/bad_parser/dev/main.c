#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sys/stat.h>

#define STDIN_FD 0
#define STDOUT_FD 1

// Custom byte swap implementations
static inline uint32_t _bswap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
}

static inline uint64_t _bswap64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

#define MAX_FILE_SIZE 4 * 1024 * 1024 // 4 Mb
#define MAX_PARSING_SIZE 256 * 1024 // 256 Kb

static int total_readed_bytes = 0;

void setup(void)
{
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

ssize_t read_into_buffer(void *buf, uint32_t size) 
{
    if (buf == NULL) {
        puts("[-] invalid buffer pointer");
        return -1;
    }

    if (size == 0) {
        puts("[-] invalid buffer size");
        return -1;
    }

    ssize_t nbytes = read(STDIN_FD, buf, size);

    if (nbytes < 0) {
        puts("[-] failed to read into buffer");
        return -1;
    }

    return nbytes;
}

ssize_t write_from_buffer(const void *buf, uint32_t size) 
{
    if (buf == NULL) {
        puts("[-] invalid buffer pointer");
        return -1;
    }

    if (size == 0) {
        puts("[-] invalid buffer size");
        return -1;
    }

    ssize_t nbytes = write(STDOUT_FD, buf, size);

    if (nbytes < 0) {
        puts("[-] failed to write from buffer");
        return -1;
    }

    return nbytes;
}

int read_integer(void)
{
    const size_t buflen = 8;

    char buf[buflen];
    ssize_t nbytes = read_into_buffer(buf, buflen);

    if (nbytes == -1) {
        puts("[-] failed to read int");
        return -1;
    }

    return atoi(buf);
}

void print_menu() {
    puts("++++ Binary parser ++++");
    puts("1. Upload binary");
    puts("2. Parse");
    puts("3. Exit");
    printf("> ");
}

void sanitize(char* buf, int len) {
    if (buf == NULL) {
        return;
    }
    
    for (int i = 0; i < len; i++) {
        if (buf[i] == '.' || buf[i] == '/' || buf[i] == '\\' || 
            buf[i] == ':' || buf[i] == '*' || buf[i] == '?' || 
            buf[i] == '"' || buf[i] == '<' || buf[i] == '>' || 
            buf[i] == '|' || buf[i] == '\n' || buf[i] == '\r' ||
            buf[i] == '\t' || buf[i] == ' ') {
                buf[i] = '_';
        }
    }
}

int upload_binary() {
    printf("[?] Enter binary name: ");
    
    char name[16];
    memset(name, 0x0, 16);
    int nbytes = read_into_buffer(name, 15);

    if (nbytes == 1 && name[0] == '\n') {
        return -1;
    } else if (name[nbytes - 1] == '\n') {
        name[nbytes - 1] = '\0';
    }

    sanitize(name, strnlen(name, 15));

    char upload_path[32];
    memset(upload_path, 0x0, 32);

    strcpy(upload_path, "/tmp/");
    strcat(upload_path, name);

    FILE *fp = fopen(upload_path, "wb");
    if (fp == NULL) {
        puts("[!] Failed to open file");
        return -1;
    }

    printf("[?] Enter binary size: ");
    uint32_t size = read_integer();
    
    if (size <= 0 || size > MAX_FILE_SIZE) {
        puts("[!] Invalid size");
        return -1;
    }

    void *buf = malloc(size);
    if (buf == NULL) {
        puts("[!] Failed to allocate memory");
        return -1;
    }

    printf("[?] Enter file data (binary): ");

    nbytes = read_into_buffer(buf, size);

    if (nbytes != -1) {
        while (nbytes != size) {
            int tmp = read_into_buffer(&buf[nbytes], size - nbytes);
            if (tmp == -1)
                break;
            else
                nbytes += tmp;
        }
    } else {
        puts("[!] Failed to read into buffer");
        fclose(fp);
        free(buf);
        return -1;
    }

    if (nbytes != size) {
        puts("[!] Failed to read all data");
        fclose(fp);
        free(buf);
        return -1;
    }

    size_t written = fwrite(buf, 1, nbytes, fp);
    if (written != nbytes) {
        puts("[!] Failed to write file");
        fclose(fp);
        free(buf);
        return -1;
    }

    fclose(fp);
    free(buf);

    printf("[+] Binary uploaded! Name: %s\n", name);
    return 0;
}

int check_file_size(const char* filepath) {
    if (filepath == NULL) {
        puts("[-] Invalid file path");
        return -1;
    }
    
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        puts("[-] Failed to open file");
        return -1;
    }
    
    if (fseek(fp, 0, SEEK_END) != 0) {
        puts("[-] Failed to seek to end of file");
        fclose(fp);
        return -1;
    }
    
    long file_size = ftell(fp);
    if (file_size < 0) {
        puts("[-] Failed to get file size");
        fclose(fp);
        return -1;
    }
    
    fclose(fp);
    
    if (file_size > MAX_PARSING_SIZE) {
        puts("[-] File size error!");
        return -1;
    }
    
    return 0;
}

static inline uint32_t read32le(FILE* fp) {
    uint32_t val;
    fread(&val, 1, sizeof(uint32_t), fp);
    total_readed_bytes += sizeof(uint32_t);
    return _bswap32(val);
}

static inline uint64_t read64le(FILE* fp) {
    uint64_t val;
    fread(&val, 1, sizeof(uint64_t), fp);
    total_readed_bytes += sizeof(uint64_t);
    return _bswap64(val);
}

static inline void read_entry(uint64_t* val1, uint32_t* val2, FILE* fp, uint8_t flg) {
    uint32_t t1, t2;
    fread(&t1, 1, sizeof(uint32_t), fp);
    fread(&t2, 1, sizeof(uint32_t), fp);
    total_readed_bytes += 8;

    t1 = _bswap32(t1);
    t2 = _bswap32(t2);
    
    // printf("[DEBUG] read_entry: t1=0x%x, t2=0x%x, flg=%d\n", t1, t2, flg);

    if (t1 == 1) {
        uint64_t val64;
        fread(&val64, 1, sizeof(uint64_t), fp);
        total_readed_bytes += sizeof(uint64_t);
        *val1 = _bswap64(val64);
        *val2 = t2;
        // printf("[DEBUG] read_entry: 64-bit mode, val1=0x%llx, val2=0x%x\n", *val1, *val2);

        if (flg) {
            // printf("[DEBUG] read_entry: seeking forward by %llu bytes\n", *val1 - 16);
            fseek(fp, *val1 - 16, SEEK_CUR);
            total_readed_bytes = total_readed_bytes - 16 + *val1;
        }
    } else {
        *val1 = t1;
        *val2 = t2;
        // printf("[DEBUG] read_entry: 32-bit mode, val1=0x%llx, val2=0x%x\n", *val1, *val2);

        if (flg) {
            // printf("[DEBUG] read_entry: seeking forward by %llu bytes\n", *val1 - 8);
            fseek(fp, *val1 - 8, SEEK_CUR);
            total_readed_bytes = total_readed_bytes - 8 + *val1;
        }
    }
}

// Optimized helper function to read two 32-bit values and conditionally read 64-bit value
static inline void read_conditional_64bit(uint64_t* t64, uint32_t* t32, FILE* fp) {
    uint32_t val1, val2;
    fread(&val1, 1, sizeof(uint32_t), fp);
    fread(&val2, 1, sizeof(uint32_t), fp);
    total_readed_bytes += 8;

    val1 = _bswap32(val1);
    val2 = _bswap32(val2);
    
    // printf("[DEBUG] read_conditional_64bit: val1=0x%x, val2=0x%x\n", val1, val2);

    if (val1 == 1) {
        uint64_t val64;
        fread(&val64, 1, sizeof(uint64_t), fp);
        total_readed_bytes += sizeof(uint64_t);
        *t64 = _bswap64(val64);
        // printf("[DEBUG] read_conditional_64bit: 64-bit mode, t64=0x%llx\n", *t64);
    } else {
        *t64 = val1;
        // printf("[DEBUG] read_conditional_64bit: 32-bit mode, t64=0x%llx\n", *t64);
    }
    *t32 = val2;
    // printf("[DEBUG] read_conditional_64bit: final t64=0x%llx, t32=0x%x\n", *t64, *t32);
}

// Optimized function to read multiple entries in sequence
static inline void read_multiple_entries(uint64_t* t64, uint32_t* t32, FILE* fp, int count) {
    // printf("[DEBUG] read_multiple_entries: reading %d entries\n", count);
    for (int i = 0; i < count; i++) {
        // printf("[DEBUG] read_multiple_entries: entry %d/%d\n", i+1, count);
        read_entry(t64, t32, fp, 1);
    }
    // printf("[DEBUG] read_multiple_entries: completed %d entries\n", count);
}

int do_parse(const char* inp_file, const char* out_file) {
    // printf("[DEBUG] Starting parse of file: %s\n", inp_file);
    // printf("[DEBUG] Output file: %s\n", out_file);
    
    FILE* fp = fopen(inp_file, "rb");

    if (fp == NULL) {
        puts("[-] Failed to open input file");
        return -1;
    }

    struct stat _stat;
    stat(inp_file, &_stat);
    uint32_t file_size = _stat.st_size;
    // printf("[DEBUG] File size: %u bytes\n", file_size);

    uint8_t* out_buffer = (uint8_t*) malloc(MAX_PARSING_SIZE);
    uint64_t t64;
    uint32_t t32;
    uint32_t max_size = 0;
    
    // printf("[DEBUG] Allocated output buffer of size: %d bytes\n", MAX_PARSING_SIZE);
    // printf("[DEBUG] Initial total_readed_bytes: %d\n", total_readed_bytes);

    // Initial parsing sequence
    // printf("[DEBUG] Reading initial 2 entries...\n");
    read_multiple_entries(&t64, &t32, fp, 2);
    // printf("[DEBUG] After initial entries - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // Check for "wide" marker (0x77696465 = "wide" in ASCII)
    // printf("[DEBUG] Checking for 'wide' marker (0x77696465)...\n");
    if (t32 == 0x77696465) {
        // printf("[DEBUG] Found 'wide' marker! Reading additional entry...\n");
        read_entry(&t64, &t32, fp, 1);
        // printf("[DEBUG] After 'wide' entry - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    } else {
        // printf("[DEBUG] No 'wide' marker found (t32 = 0x%x)\n", t32);
    }

    // Parse sequence with conditional reads
    // printf("[DEBUG] Starting conditional read sequence 1...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After conditional read 1 - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // printf("[DEBUG] Reading 4 multiple entries...\n");
    read_multiple_entries(&t64, &t32, fp, 4);
    // printf("[DEBUG] After 4 entries - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // printf("[DEBUG] Conditional read sequence 2...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After conditional read 2 - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // printf("[DEBUG] Reading single entry...\n");
    read_entry(&t64, &t32, fp, 1);
    // printf("[DEBUG] After single entry - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // printf("[DEBUG] Conditional read sequence 3...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After conditional read 3 - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // printf("[DEBUG] Reading 2 multiple entries (first set)...\n");
    read_multiple_entries(&t64, &t32, fp, 2);
    // printf("[DEBUG] After 2 entries (first set) - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // printf("[DEBUG] Conditional read sequence 4...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After conditional read 4 - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // printf("[DEBUG] Reading 2 multiple entries (second set)...\n");
    read_multiple_entries(&t64, &t32, fp, 2);
    // printf("[DEBUG] After 2 entries (second set) - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // printf("[DEBUG] Conditional read sequence 5...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After conditional read 5 - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // printf("[DEBUG] Reading 3 multiple entries...\n");
    read_multiple_entries(&t64, &t32, fp, 3);
    // printf("[DEBUG] After 3 entries - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);

    // printf("[DEBUG] Final conditional read...\n");
    read_conditional_64bit(&t64, &t32, fp);
    // printf("[DEBUG] After final conditional read - t64: 0x%llx, t32: 0x%x, total_readed: %d\n", t64, t32, total_readed_bytes);
    
    // Skip 8 bytes
    // printf("[DEBUG] Skipping 8 bytes...\n");
    fseek(fp, 8, SEEK_CUR);
    total_readed_bytes += 8;
    // printf("[DEBUG] After skip - total_readed: %d\n", total_readed_bytes);

    // Read samples with optimized loop
    // printf("[DEBUG] Reading sample count...\n");
    uint32_t samples_cnt = read32le(fp);
    // printf("[DEBUG] samples_cnt: %u (0x%x)\n", samples_cnt, samples_cnt);
    uint32_t* pSamplesBuf32 = NULL;

    if (samples_cnt > 0) {
        // printf("[DEBUG] Processing %u samples...\n", samples_cnt);
        
        // Validate sample count to prevent excessive memory allocation
        if (samples_cnt > MAX_PARSING_SIZE / sizeof(uint32_t)) {
            puts("[-] Too many samples");
            fclose(fp);
            return -1;
        }
        
        // printf("[DEBUG] Allocating memory for %u samples (%u bytes)...\n", samples_cnt, samples_cnt * sizeof(uint32_t));
        pSamplesBuf32 = (uint32_t*) malloc(samples_cnt * sizeof(uint32_t));
        if (pSamplesBuf32 == NULL) {
            puts("[-] Failed to allocate memory for samples");
            fclose(fp);
            return -1;
        }

        // Optimized reading with batch fread
        // printf("[DEBUG] Reading sample data from file...\n");
        if (fread(pSamplesBuf32, sizeof(uint32_t), samples_cnt, fp) != samples_cnt) {
            puts("[-] Failed to read samples");
            free(pSamplesBuf32);
            fclose(fp);
            return -1;
        }
        
        total_readed_bytes += samples_cnt * sizeof(uint32_t);
        // printf("[DEBUG] After reading samples - total_readed: %d\n", total_readed_bytes);
        
        // Find max size with optimized loop
        // printf("[DEBUG] Processing samples and finding max size...\n");
        for (uint32_t i = 0; i < samples_cnt; i++) {
            pSamplesBuf32[i] = _bswap32(pSamplesBuf32[i]);
            // printf("[DEBUG] Sample[%u]: 0x%x\n", i, pSamplesBuf32[i]);
            if (max_size < pSamplesBuf32[i]) {
                max_size = pSamplesBuf32[i];
                // printf("[DEBUG] New max_size: 0x%x\n", max_size);
            }
        }
        // printf("[DEBUG] Final max_size: 0x%x\n", max_size);
    } else {
        // printf("[DEBUG] No samples to process\n");
    }

    // printf("[DEBUG] Reading remaining data from file...\n");
    
    fread(out_buffer, 1, (file_size - total_readed_bytes > MAX_PARSING_SIZE) ? MAX_PARSING_SIZE : file_size - total_readed_bytes, fp);
    uint32_t remaining_size = file_size - total_readed_bytes;
    // printf("[DEBUG] Read %u bytes into output buffer\n", remaining_size);

    // printf("[DEBUG] Appending metadata to output buffer...\n");
    // printf("[DEBUG] Appending max_size: 0x%x\n", max_size);
    memcpy(&out_buffer[remaining_size], &max_size, sizeof(uint32_t));
    remaining_size += sizeof(uint32_t);
    
    // printf("[DEBUG] Appending samples_cnt: %u\n", samples_cnt);
    memcpy(&out_buffer[remaining_size], &samples_cnt, sizeof(uint32_t));
    remaining_size += sizeof(uint32_t);
    
    if (samples_cnt != 0 && pSamplesBuf32 != NULL) {
        // printf("[DEBUG] Appending %u samples to output buffer...\n", samples_cnt);
        memcpy(&out_buffer[remaining_size], pSamplesBuf32, samples_cnt * sizeof(uint32_t));
        remaining_size += samples_cnt * sizeof(uint32_t);
    }
    
    // printf("[DEBUG] Final output buffer size: %u bytes\n", remaining_size);
    // printf("[DEBUG] Opening output file: %s\n", out_file);
    
    FILE* fp_out = fopen(out_file, "wb");
    
    if (fp_out != NULL) {
        // printf("[DEBUG] Writing %u bytes to output file...\n", remaining_size);
        fwrite(out_buffer, 1, remaining_size, fp_out);
        // printf("[DEBUG] Successfully wrote output file\n");
    } else {
        // printf("[DEBUG] Failed to open output file\n");
    }
    
    // Cleanup
    // printf("[DEBUG] Cleaning up resources...\n");
    fclose(fp);
    if (fp_out != NULL) {
        fclose(fp_out);
    }

    puts("[!] Sending file data");

    uint64_t sended_bytes = 0;
    while (sended_bytes < remaining_size) {
        ssize_t tmp = write_from_buffer(out_buffer + sended_bytes, remaining_size - sended_bytes);
        if (tmp == -1) {
            // printf("[DEBUG] Failed to send %llu bytes\n", sended_bytes);
            break;
        }
        sended_bytes += tmp;
    }
   
    // printf("[DEBUG] Sended %llu bytes\n", sended_bytes);

    free(out_buffer);
    if (pSamplesBuf32 != NULL) {
        free(pSamplesBuf32);
    }
    //printf("[DEBUG] Parse completed successfully\n");
    return 0;
}

int parse_binary() {
    printf("[?] Enter binary name: ");
    
    char name[16];
    memset(name, 0x0, 16);
    int nbytes = read_into_buffer(name, 15);

    if (nbytes <= 0) {
        puts("[-] Error in name enter!");
        return 0;
    }

    if (name[nbytes - 1] == '\n') {
        name[nbytes - 1] = '\0';
    }

    sanitize(name, strnlen(name, 15));

    char filepath[32];
    memset(filepath, 0x0, 32);

    strcpy(filepath, "/tmp/");
    strcat(filepath, name);

    if (check_file_size(filepath) == -1) {
        return 0;
    }

    printf("[?] Enter output name: ");
    char outname[16];
    memset(outname, 0x0, 16);
    nbytes = read_into_buffer(outname, 15);

    if (nbytes == 1 && outname[0] == '\n') {
        return -1;
    } else if (nbytes > 0 && outname[nbytes - 1] == '\n') {
        outname[nbytes - 1] = '\0';
    }

    sanitize(outname, strnlen(outname, 15));

    char outfile_path[32];  
    memset(outfile_path, 0x0, 32);

    strcpy(outfile_path, "/tmp/");
    strcat(outfile_path, outname);

    return do_parse(filepath, outfile_path);
}

// 1. TOCTOU to change file size
// 2. binary praser error when size it too big
// 3. mmap ptr - libc ptr diff is contant
// 4. overwrite GOT inside libc

int main() {
    setup();
    int cycles = 0;

    while (1) {
        cycles++;
        if (cycles > 25 )
            break; // avoid infinite loop

        print_menu();
        int option = read_integer();
        
        switch (option) {
            case 1:
                upload_binary();
                break;
            case 2:
                parse_binary();
                break;
            case 3:
                exit(0);
                break;
            default:
                puts("[-] invalid option");
                break;
        }
    }

    return 0;
}