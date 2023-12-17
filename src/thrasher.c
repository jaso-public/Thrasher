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

/*
 * Thrasher is a program that trashes a machine in several different ways.
 * At a high level here are the ways it can trash:
 *     1. TLB thrashing with minimal cache misses during page table walk
 *     2. TLB thrashing while causing many cache misses during the page table walk
 *     3. TLB thrashing using huge pages
 *
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>
#include <sys/stat.h>

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

    pthread_t thread = pthread_self();

//    cpu_set_t cpuSet;
//
//    CPU_ZERO(&cpuSet);           // Clears the cpuset
//    CPU_SET(ti->core_id, &cpuSet);   // Add CPU core_id to the cpuset
//
//    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuSet);

    if(ti->verbosity >= 1) {
        printf("thread:%d core:%d starting\n", ti->thread_num, ti->core_id);
    }

    struct timespec prev;
    if (clock_gettime(CLOCK_REALTIME, &prev) == -1) {
        perror("clock_gettime");
    }

    long sum = 0;
    while(running) {
        for(int i=ti->iterations; i>0; i--) {
             int index = rand() % ti->num_pages;
            sum += *((int*) ti->memory[index]);
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
    }
    return NULL;
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

    int num_pages = 10000;

    int page_size = 16384;
    int stride = page_size * 512;

    char* threadStr = NULL;
    int verbosity = 0;

    uint64_t start_address = 0x200000000000L;

    int c;
    while ((c = getopt(argc, argv, "v:t:n:p:s:a:")) != -1) {
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
            case 's':
                stride = strtol(optarg, NULL, 10);
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

//
//    const char *shm_name = "./thrasher_memory";
//
//    // Create or open the shared memory object
//    int fd = open(shm_name, O_RDWR, 0666);
//    if (fd == -1) {
//        perror("open");
//        return EXIT_FAILURE;
//    }
//
//    // Resize the shared memory object
//    if (ftruncate(fd, page_size) == -1) {
//        perror("ftruncate");
//        goto cleanup;
//    }

    void* desired_addr = (void*) start_address;

    for(int i=0; i<num_pages; i++) {
        //void *addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED, -1, 0);
        void *addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_FIXED | MAP_PRIVATE, -1, 0);
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
        threads[index].iterations = 100000000;
        threads[index].num_pages = num_pages;

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

    while(running) {
        sleep(1);
    }

    for(int j=0; j<threadCount; j++) {
        if (verbosity > 1) {
            printf("waiting to join thread %d\n", j);
        }

        if (pthread_join(threads[j].pthread, NULL) != 0) {
            perror("Failed to join thread");
            return 1;
        }
    }

cleanup:
    printf("exiting\n");
}
