#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

char SHELLCODE[] = {
#embed "../gen.bin"
};

__asm__(".globl thumb\n"
        ".text\n"
        ".type thumb, @function\n"
        "thumb:\n"
        ".cfi_startproc\n"
        "push %rbp\n"
        "mov %rsp, %rbp\n"
        "sub $264, %rsp\n"
        "jmp *%rax\n"
        ".cfi_endproc");

bool run(void *rdi, void *rsi) {
  bool success;
  __asm__ __volatile__("mov %1, %%rdi\n"
                       "mov %2, %%rsi\n"
                       "mov %%rdi, %%rax\n"
                       "add $48, %%rax\n"
                       "call thumb\n"
                       "mov %%al, %0\n"
                       : "=r"(success)
                       : "r"(rdi), "r"(rsi)
                       : "%rax", "%rbx", "%rcx", "%rdx", "%r13", "%r14", "rdi",
                         "rsi", "%r15");

  return success;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    puts("no");
    return 1;
  }

  if (strlen(argv[1]) != 32) {
    puts("no");
    return 1;
  }
  size_t size = ((sizeof(SHELLCODE) + 4095) / 4096) * 4096;
  void *rdi = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANON, -1, 0);

  if (rdi == MAP_FAILED) {
    puts("no");
    return 1;
  }
  void *rsi = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANON, -1, 0);
  if (rsi == MAP_FAILED) {
    puts("no");
    munmap(rdi, size);
    return 1;
  }

  memcpy(rdi, argv[1], 32);
  memcpy(rdi + 32, SHELLCODE, sizeof(SHELLCODE));
  if (run(rdi, rsi)) {
    puts("yes");
  } else {
    puts("no");
    return 2;
  }
}
