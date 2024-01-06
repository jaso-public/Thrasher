
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void print_histogram(uint32_t* histogram, int size) {
    int n = 0;
    for(int i = 0; i < size; i++) {
        if(histogram[i]>0) n = i;
    }

    for(int i = 0; i <= n; i++) {
        printf("%i %lu: %d\n", i, ((uint64_t)1)<<i, histogram[i]);
    }
}

static void access(uint32_t* histogram, size_t histogram_size, uint64_t count) {
    uint32_t histo[65];
    memset(histo, 0, sizeof(histo));

    uint64_t foo;
    asm volatile (
        "movq %1, %%r8\n"            // load the address of the histogram
        "movq %2, %%r9\n"            // count iterations
        "loop:\n"
//        "  cli\n"
        "  rdtscp\n"                  // read the clock
        "  movl %%eax, %%esi\n"      // store the values in registers 
        "  movl %%edx, %%edi\n"      // so they don't get clobbered
        "  rdtscp\n"                  // read the clock again
//        "  sti\n"                    
        "  shlq $32,%%rdx\n"         // shift the hi part of the tsc left by 32 bits
        "  movl %%eax, %%eax\n"      // clear the high bits of rax
        "  orq %%rdx, %%rax\n"       // OR them together so that rax contains end time
        "  shlq $32,%%rdi\n"         // shift the hi part of the start time
        "  orq %%rdi, %%rsi\n"       // OR the start time parts together 
        "  subq %%rsi, %%rax\n"      // subtract start time from end time
        "  bsrq %%rax, %%rax\n"      // find the highest bit set in the elapsed time
        "  incl (%%r8, %%rax, 4)\n"  // increment the bucket in the histogram
        "  decq %%r9\n"              // decrement the loop counter
        "  jnz loop\n"               // and loop until the counter is zero
        : "=r"(foo)
        : "r"(histo), "r"(count)
        : "%rax", "%rcx", "%rdx", "%rdi", "%rsi", "r8", "r9"
    );  

    int num = 65;
    if(histogram_size < num) num = histogram_size;
    for(int i=0; i<num; i++) {
        histogram[i] = histo[i];
    }
}

int main() {
    uint32_t histogram[64];
   
    access(histogram, 64, 1000000000);
    print_histogram( histogram, 63);
    access(histogram, 64, 1000000000);
    print_histogram( histogram, 63);
    return 0;
}
