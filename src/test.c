#include <complex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void print_histogram(uint32_t* histogram, int size) {
    for(int i = 0; i < size; i++) {
        if(histogram[i]>0) {
            printf("%d: %d\n", i, histogram[i]);
        }
    }
}

static inline uint64_t access(void* address) {
    uint64_t result;
    asm volatile (
        "movq %1, %%r8\n"       // load the address to be read into r8 

        "rdtscp\n"              // read the clock
        "movl %%eax, %%esi\n"   // store the values in registers 
        "movl %%edx, %%edi\n"   // so that they don't get clobbered

        "movq (%%r8), %%rax\n"  // do the memory access

        "rdtscp\n"              // read the clock again
        "shlq $32,%%rdx\n"      // shift the hi part of the tsc left by 32 bits
        "movl %%eax, %%eax\n"   // clear the high bits of rax
        "orq %%rdx, %%rax\n"    // OR them together so that rax contains end time
 
        "shlq $32,%%rdi\n"      // shift the hi part of the start time
        "orq %%rdi, %%rsi\n"    // OR the start time parts together 

        "subq %%rsi, %%rax\n"   // subtract start time from end time
        "movq %%rax, %0\n"      // store the elapsed time in result
        : "=r"(result)   
        : "r"(address)
        : "%rax", "%rcx", "%rdx", "%rdi", "%rsi", "r8"
    );
    return result ;     
}


void doTest(uint32_t* histogram, uint32_t* histogram2, void* memory, int size) {

    printf("memory = %p\n", memory);
    uint64_t start = access(memory);
    
    printf("start: %lx\n", start);
}


int main() {
    int size = 16*1024*1024;
    void* memory = malloc(size+64);
    printf("memory1 = %p\n", memory);


    uint32_t histogram[64];
    uint32_t histogram2[128];
    doTest(histogram, histogram2, memory, size);
    print_histogram( histogram, 64);
    printf("----------------\n");
    print_histogram( histogram2, 128);
    printf("Hello World!\n");
    return 0;
}