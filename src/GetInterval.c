
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 #include <time.h>

#include <MemoryAccess.h>

uint64_t getTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
}

int main() {
    uint64_t start_time, end_time;


    int num = 1000;
    uint64_t histogram[num];
    uint32_t elapsed[num];
 
    uint64_t start, end, value;
    uint64_t address = 0x7f8c0c000000;

    int count = 0;
    uint64_t total = 0;

    start_time = getTime();

    while(count < num) {
        total++;
        withoutAccess((void*)address, &start, &end, &value);
        uint64_t e = end - start;
        if(e < 30000) continue;    
       
        histogram[count] = start;
        elapsed[count] = end - start;
        count++;
    }

    end_time = getTime();

    for(int i=1 ; i<num ; i++) {
        uint64_t delta = histogram[i] - histogram[i-1];
        printf("%lu %u %lu\n", delta, elapsed[i-1], histogram[i]);
    }


    printf("total: %lu\n", total);
    uint64_t total_elapsed = end - histogram[0];
    printf("total elapsed: %lu\n", total_elapsed);
    printf("average: %lu\n", total_elapsed / total);

     uint64_t elapsed_time = end_time - start_time;
    printf("elapsed time: %lu\n", elapsed_time);

    double et = (double)elapsed_time;
    double tt = (double)total_elapsed;

    printf("ratio: %f\n", tt/et);    
    
    return 0;
 }