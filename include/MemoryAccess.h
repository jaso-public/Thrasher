
#ifndef MEMORYACCESS_H
#define MEMORYACCESS_H

#include <stdint.h>


static void withoutAccess(void* address, uint64_t* start, uint64_t* end, uint64_t* value) {
    asm volatile (
        "  movq %3, %%r8\n"               // the virtual address to read
        "  rdtsc\n"                      // read the clock
        "  movl %%eax, %%esi\n"           // store the values in registers 
        "  movl %%edx, %%edi\n"           // so they don't get clobbered
        "  andq $56, %%rax\n"             // we are going to use the results of the rdtsc to
                                          // compute an address to fetch, that way the processor 
                                          // cannot begin the memory fetch until the tsc is read                      
        // "  movq (%%r8, %%rax, 1), %%r9\n" // this is where we do the actual memory access
        "  rdtsc\n"                      // read the clock again
        "  shlq $32, %%rdx\n"              // shift the hi part of the tsc left by 32 bits
        "  movl %%eax, %%eax\n"           // clear the high bits of rax
        "  orq %%rdx, %%rax\n"            // OR them together so that rax contains end time
        "  movq %%rax, %1\n"             // store the end time
        "  shlq $32, %%rdi\n"             // shift the hi part of the start time
        "  orq %%rdi, %%rsi\n"            // OR the start time parts together (note hibits were cleared when moved)
        "  movq %%rsi, %0\n"              // store the start time
        "  movq %%r9, %2\n"               // store the value
        : "=r"(*start), "=r"(*end), "=r"(*value)
        : "r"(address)
        : "%rax", "%rcx", "%rdx", "%rdi", "%rsi", "r8", "r9"
    );  
}

static void access64bit(void* address, uint64_t* start, uint64_t* end, uint64_t* value) {
    asm volatile (
        "  movq %3, %%r8\n"               // the virtual address to read
        "  rdtscp\n"                      // read the clock
        "  movl %%eax, %%esi\n"           // store the values in registers 
        "  movl %%edx, %%edi\n"           // so they don't get clobbered
        "  andq $56, %%rax\n"             // we are going to use the results of the rdtsc to
                                          // compute an address to fetch, that way the processor 
                                          // cannot begin the memory fetch until the tsc is read                      
        "  movq (%%r8, %%rax, 1), %%r9\n" // this is where we do the actual memory access
        "  rdtscp\n"                      // read the clock again
        "  shlq $32,%%rdx\n"              // shift the hi part of the tsc left by 32 bits
        "  movl %%eax, %%eax\n"           // clear the high bits of rax
        "  orq %%rdx, %%rax\n"            // OR them together so that rax contains end time
        "  movq %%rax, %1\n"             // store the end time
        "  shlq $32, %%rdi\n"             // shift the hi part of the start time
        "  orq %%rdi, %%rsi\n"            // OR the start time parts together (note hibits were cleared when moved)
        "  movq %%rsi, %0\n"              // store the start time
        "  movq %%r9, %2\n"               // store the value
        : "=r"(start), "=r"(end), "=r"(value)
        : "r"(address)
        : "%rax", "%rcx", "%rdx", "%rdi", "%rsi", "r8", "r9"
    );  
}

#endif // MEMORYACCESS_H
