
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

static inline uint64_t access_memory(void* address) {
    uint64_t result;
    asm volatile (
        "movq %1, %%r8\n"       // load the address to be read into r8 

        "rdtscp\n"              // read the clock
        "movl %%eax, %%esi\n"   // store the values in registers 
        "movl %%edx, %%edi\n"   // so that they don't get clobbered
        "andq $31, %%rax\n"     
        "orq %%rax, %%r8\n"
        "movq (%%r8), %%rax\n"    

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


void print_histogram(uint32_t* histogram, int size) {
    for(int i = 0; i < size; i++) {
        if(histogram[i]>0) {
            printf("%d: %d\n", i, histogram[i]);
        }
    }
}



int writeHistogram(char* fileName, uint32_t* histogram, int size) {

    // Find the last non-zero entry in the histogram
    int n = size;
    while(n > 0 && histogram[n-1] == 0)  n--;

    // Declare a FILE pointer
    FILE *file;

    // Open the file for writing
    file = fopen(fileName, "w");

    // Check if the file was successfully opened
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    for(int i = 0; i < n; i++) {
       fprintf(file, "%d\n", histogram[i]);
    }

    // Close the file
    fclose(file);

    return 0;
}



/*
 * Thrasher is a program that trashes a machine in several different ways.
 * At a high level here are the ways it can trash:
 *     1. TLB thrashing with minimal cache misses during page table walk
 *     2. TLB thrashing while causing many cache misses during the page table walk
 *     3. TLB thrashing using huge pages
 *
 */
/* all the threads periodically test this variable to
 * see if the application should continue to run */
volatile int running = 1;

struct thread_info {
    int thread_num;
    int core_id;
    int verbosity;
    pthread_t pthread;

    int iterations;
    int num_pages;
    void** memory;

    uint32_t* histogram;
    int histogram_size;
};

typedef struct thread_info thread_info_t;


// Signal Handler for SIGINT
void sigint_handler(int sig_num) {
    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigint_handler);

    running = 0;
    printf("Ctrl+C -- shutting down\n");
    fflush(stdout);
}

void* threadFunction(void* args) {
    thread_info_t* ti = (thread_info_t*) args;

    // get the number of cpus on this machine.
 	long numCpus = sysconf(_SC_NPROCESSORS_ONLN);

    // make sure the core_id is valid for this machine.
    if(ti->core_id >= numCpus) {
        printf("Error, core_id(%d) is greater than the number of cpus(%ld)\n", ti->core_id, numCpus);
        exit(-1);
    }

    pthread_t thread = pthread_self();

    cpu_set_t *cpusetp;
 
    cpusetp = CPU_ALLOC(numCpus);
    if (cpusetp == NULL) {
        perror("unable to allocate cpusetp");
        exit(EXIT_FAILURE);
    }

    // assign the thread to the core_id
    size_t size = CPU_ALLOC_SIZE(numCpus);
    CPU_ZERO_S(size, cpusetp);    
    CPU_SET_S(ti->core_id, size, cpusetp);
    pthread_setaffinity_np(thread, size, cpusetp);
    CPU_FREE(cpusetp);

    // tell the user what core this thread is running on.
    if(ti->verbosity >= 1) {
        printf("thread:%d core:%d starting\n", ti->thread_num, ti->core_id);
    }

    struct timespec prev;
    if (clock_gettime(CLOCK_REALTIME, &prev) == -1) {
        perror("clock_gettime");
    }

    int index = 0;
    int offset = 4032;
    long sum = 0;
        for(int i=ti->iterations; i>0; i--) {
        int index = (index + 90001) % ti->num_pages;
        if(index == 0) {
            offset -= 64;
            if(offset == 0) {
                offset = 4032;
            }
        }
        void *addr = ti->memory[index] + offset;
        uint64_t elapsed = access_memory(addr);
        if(elapsed > ti->histogram_size) {
            elapsed = ti->histogram_size - 1;
        }
        ti->histogram[elapsed]++;
    }


    struct timespec ts;

    // Get the current high-resolution time
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        perror("clock_gettime");
    }

    int64_t delta = (ts.tv_sec - prev.tv_sec) * 1000000000L;
    delta += (ts.tv_nsec - prev.tv_nsec);
    prev = ts;

    double rate = ti->iterations;
    rate = rate / delta * 1000000000;

    // Print high-resolution time
    printf("Current time: %ld seconds, %09ld nanoseconds delta:%ld rate:%f\n", ts.tv_sec, ts.tv_nsec, delta, rate);
    
    return NULL;
}

uint32_t* makeHistogram(int size) {
    uint32_t* histogram = malloc(sizeof(uint32_t) * size);
    if(histogram == NULL) {
        perror("malloc");
        exit(-1);
    }
    for(int i=0; i<size; i++) {
        histogram[i] = 0;
    }
    return histogram;
}   

void usage(char* programName) {
    printf("Usage: %s\n", programName);
    printf("\t-t [how many threads to run on the cores]\n");
    printf("\t\ti.e. -t 1,2,3,3 will run four threads 1 each on core 1 and 2 and 2 threads on core 3.\n");
    printf("\t-v <verbosity>\n");
    printf("\t\ti.e. -v 0 (silent) -v 1 (some info) -v2 verbose");

    exit(-1);
}

int main(int argc, char** argv) {

    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigint_handler);

    char* programName = argv[0];

    int num_pages = 65500;

    int page_size = 4096;
    int stride = page_size * 512;

    char* threadStr = NULL;
    int verbosity = 0;

    uint64_t iterations = 1000000000L;

    uint64_t start_address = 0x200000000000L;
    uint32_t histogram_size = 1000001;

    int c;
    while ((c = getopt(argc, argv, "v:t:n:p:s:i:a:h:")) != -1) {
        switch (c) {
            case 't':
                threadStr = optarg;
                break;
            case 'n':
                num_pages = strtol(optarg, NULL, 10);
                break;
            case 'p':
                page_size = strtol(optarg, NULL, 10);
                break;
            case 'i':
                iterations = strtol(optarg, NULL, 10);
                break;
            case 's':
                stride = strtol(optarg, NULL, 10);
                break;
            case 'h':
                histogram_size = strtol(optarg, NULL, 10);
                break;
            case 'v':
                verbosity = strtol(optarg, NULL, 10);
                break;
            case 'a':
                start_address = strtol(optarg, NULL, 16);
                break;
            default:
                usage(programName);
        }
    }

    if(threadStr == NULL) {
        printf("\n\nError, you must specify at least one thread\n");
        usage(programName);
    }

    int threadCount = 0;
    char *delimiter = ",";

    char *buffer = malloc(strlen(threadStr) + 1);
    assert(buffer != NULL);
    strcpy(buffer, threadStr);

    char *tmp = NULL;
    char *token = NULL;

    tmp = buffer;
    token = strtok(tmp, delimiter);
    while (token != NULL) {
        threadCount++;
        token = strtok(NULL, delimiter);
    }


    void** addrs = malloc(sizeof(void*) * num_pages);
    assert(addrs != NULL);
    for(int i=0; i<num_pages; i++) {
        addrs[i] = NULL;
    }


    const char *shm_name = "./thrasher_memory";

    // Create or open the shared memory object
    int fd = open(shm_name, O_RDWR, 0666);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    // Resize the shared memory object
    if (ftruncate(fd, page_size) == -1) {
        perror("ftruncate");
        goto cleanup;
    }

    void* desired_addr = (void*) start_address;

    for(int i=0; i<num_pages; i++) {
        void *addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0);
        //void *addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            printf("desired_addr: %p, page_size: %d, addr: %p, index:%i\n", desired_addr, page_size, addr, i);
            perror("mmap");
            goto cleanup;
        }

        if(addr != desired_addr) {
            printf("addr(%p) does not equal desired_addr(%p)\n", addr, desired_addr);
            goto cleanup;
        }

        addrs[i] = addr;
        desired_addr += stride;
    }


    // allocate the arry of thread_info structs
    thread_info_t* threads = malloc(sizeof(thread_info_t) * threadCount);
    strcpy(buffer, threadStr);
    tmp = buffer;
    token = strtok(tmp, delimiter);
    int index = 0;
    while (token != NULL) {
        threads[index].thread_num = index;
        threads[index].core_id = strtol(token, NULL, 10);
        threads[index].verbosity = verbosity;

        threads[index].memory = addrs;
        threads[index].iterations = iterations;
        threads[index].num_pages = num_pages;

        threads[index].histogram_size = histogram_size;
        threads[index].histogram = makeHistogram(histogram_size);
        
        index++;
        token = strtok(NULL, delimiter);
    }

    // free the buffer used to store a copy of the threads argument.
    free(buffer);

    if(verbosity > 0) {
        printf("thread count: %d\n", threadCount);
        printf("threads: %s\n", threadStr);
        printf("num_pages: %d\n", num_pages);
        printf("page_size: 0x%x (%d)\n", page_size, page_size);
        printf("stride: 0x%x (%d)\n", stride, stride);
        printf("first addr: %p\n", addrs[0]);
        printf("last addr: %p\n", addrs[num_pages-1]);
        printf("verbosity: %d\n", verbosity);
        printf("\n");
    }


    // sanity check that we are pointing at the same physical memory
    int val = 56382;
    *((int*) addrs[num_pages-1]) = val - 56; // put some value in the shm
    *((int*) addrs[0]) = val; // change the first buffer
    for(int i=0; i<num_pages; i++) {
        assert(*((int *) addrs[0]) == val); // they should all match the first buffer.
    }

    for(int j=0; j<threadCount; j++) {
        if(verbosity>1) {
            printf("creating thread %d\n", j);
        }

        int result = pthread_create(&(threads[j].pthread), NULL, threadFunction, &(threads[j]));
        if (result != 0) {
            perror("Thread creation failed");
            goto cleanup;
        }
    }

    // while(running) {
    //     sleep(1);
    // }

    if(verbosity > 0) {
        printf("joining threads\n");
    }

    uint32_t* histogram = makeHistogram(histogram_size);
 
     for(int j=0; j<threadCount; j++) {
        if (verbosity > 1) {
            printf("waiting to join thread %d\n", j);
        }

        if (pthread_join(threads[j].pthread, NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }

        for(int i=0; i<histogram_size; i++) {
            histogram[i] += threads[j].histogram[i];
        }

        
    }

    writeHistogram("foo.out", histogram, histogram_size);

cleanup:
    printf("exiting\n");
}
