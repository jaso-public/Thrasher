
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>


#include <stdio.h>
#include <stdint.h>

uint64_t read_tsc() {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}



uint64_t access_memory(uint64_t startAddr, uint64_t increment, uint64_t limit, uint64_t count) {
    uint64_t output;
     __asm__ volatile (
        "movq %1, %%rax\n" 
        "movq %2, %%rbx\n" 
        "movq %3, %%rcx\n" 
        "movq %4, %%rdi\n" 
        "movq %%rax, %%rdx\n" 
        "call *%%rax\n"
        "movq %%rdi, %0\n"
        : "=r" (output)
        : "r" (startAddr),  
          "r" (increment),   
          "r" (limit),   
          "r" (count)
        : "rax", "rbx", "rcx", "rdx", "rdi"  // Clobbered registers
    );
    return output;
}


/*

000000000000113a <routine>:
    113a:	48 ff cf             	dec    %rdi
    113d:	75 01                	jne    1140 <label1>
    113f:	c3                   	ret    

0000000000001140 <label1>:
    1140:	48 01 d8             	add    %rbx,%rax
    1143:	48 39 c1             	cmp    %rax,%rcx
    1146:	75 03                	jne    114b <label2>
    1148:	48 89 d0             	mov    %rdx,%rax

000000000000114b <label2>:
    114b:	ff e0                	jmp    *%rax


*/


int main(int argc, char** argv) {

    uint8_t code[] = {0x48, 0xff, 0xcf, 
                      0x75, 0x01, 
                      0xc3, 
                      0x48, 0x01, 0xd8,
                      0x48, 0x39, 0xc1,
                      0x75, 0x03,
                      0x48, 0x89, 0xd0,
                      0xff, 0xe0 };

    uint64_t stride = 4096;

    uint64_t start_address = 0x200000000000L;
    void* desired_addr = (void*) start_address;

    void *addr = mmap(desired_addr, 1024*1024*1024, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANON, -1, 0);
    if (addr == MAP_FAILED) {
        printf("desired_addr: %p, stride: %lu, addr: %p\n", desired_addr, stride, addr);
        perror("mmap");
            return EXIT_FAILURE;
    }

    if(addr != desired_addr) {
        printf("addr(%p) does not equal desired_addr(%p)\n", addr, desired_addr);
            return EXIT_FAILURE;
    }

    uint8_t* memory = desired_addr;
    printf("after %p\n", &memory);
    

    int loops = 1 << (30-6);

    for(int i=0 ; i<loops ; i++) {
        memcpy(memory + i*64, code, sizeof(code));    
    }
    printf("copied\n");


     uint64_t iters = 10000000;
     
    
    for(int i=1; i<loops ; i++) {
        uint64_t desired_addr = (uint64_t) memory + i * stride;
        uint64_t start = read_tsc();
        uint64_t res = access_memory((uint64_t)memory, stride, (uint64_t)desired_addr, iters);

        uint64_t end = read_tsc();

        uint64_t elapsed = end - start;

        double d = elapsed;
        d = d / iters;
        printf("i:%d elapsed: %lu  %f\n", i, elapsed, d);
    }
}


