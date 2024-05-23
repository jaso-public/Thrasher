#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <getopt.h>


uint64_t convert_with_suffix(const char *str) {
    char *end;
    uint64_t value = strtol(str, &end, 10);

    switch (*end) {
        case 'K':
            value *= 1024;
            break;
        case 'k':
            value *= 1e3;
            break;
        case 'M':
            value *= 1024L * 1024L;
            break;
        case 'm':
            value *= 1e6;
            break;
        case 'G':
            value *= 1024L * 1024L * 1024L;
            break;
        case 'g':
            value *= 1e9;
            break;
        case 'T':
            value *= 1024L * 1024L * 1024L * 1024L;
            break;
        case 't':
            value *= 1e12;
            break;
        case '\0': // No suffix
            break;
        default:
            fprintf(stderr, "Invalid suffix: %c\n", *end);
            exit(EXIT_FAILURE);
    }

    return (long long)value;
}

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
static uint8_t code[] = {
                    0x48, 0xff, 0xcf, 
                    0x75, 0x01, 
                    0xc3, 
                    0x48, 0x01, 0xd8,
                    0x48, 0x39, 0xc1,
                    0x75, 0x03,
                    0x48, 0x89, 0xd0,
                    0xff, 0xe0 };


void usage(char* programName) {
    printf("Usage: %s\n", programName);
    printf("\tNEEDS more info\n");

    exit(-1);
}

int main(int argc, char** argv) {

    char* program_name = argv[0];

    uint64_t start_address = 0x200000000000L;
    void* desired_addr = (void*) start_address;
    int page_size = 4096;
    uint64_t stride = page_size*512;
    int num_pages = 10;
    uint64_t iterations = 100000000;


   int c;
    while ((c = getopt(argc, argv, "v:n:p:s:i:a:h:")) != -1) {
        switch (c) {
            case 'n':
                num_pages = convert_with_suffix(optarg);
                break;
            case 'p':
                page_size = convert_with_suffix(optarg);
                break;
            case 'i':
                iterations = convert_with_suffix(optarg);
                break;
            case 's':
                stride = convert_with_suffix(optarg);
                break;
            //  case 'v':
            //     verbosity = convert_with_suffix(optarg);
            //     break;
            case 'a':
                start_address = convert_with_suffix(optarg);
                break;
            default:
                usage(program_name);
        }
    }

    printf("program_name:%s\n", program_name);
    printf("start_address:%p\n", (void*)start_address);
    printf("page_size:%d\n", page_size);
    printf("stride:%lu\n", stride);
    printf("num_pages:%d\n", num_pages);
    printf("iterations:%lu\n", iterations);

  
     for(int i=0; i<num_pages; i++) {
        // Memory map the file
        void *addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANON, -1, 0);
        if (addr == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        if(addr != desired_addr) {
            printf("addr(%p) does not equal desired_addr(%p)\n", addr, desired_addr);
             return EXIT_FAILURE;
        }

         memcpy(addr, code, sizeof(code));

        desired_addr += stride;
    }


    memcpy((void*)start_address, code, sizeof(code));

    uint64_t start = read_tsc();
    uint64_t res = access_memory(start_address, stride, (uint64_t)desired_addr, iterations);
    uint64_t end = read_tsc();


    uint64_t elapsed = end - start;

    double d = elapsed;
    d = d / iterations;
    printf("elapsed: %lu  %f\n", elapsed, d);

    return 0;
}
