#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t rax = 0xFFFFFFFFFFFFFFFF; // Example initial value for RAX

    // Inline assembly to zero out the upper 32 bits of RAX
    asm("movl %%eax, %%eax" : "+a" (rax));

    printf("RAX: 0x%lx\n", rax);
    return 0;
}

