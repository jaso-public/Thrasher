#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    size_t page_size = 2 * 1024 * 1024;  // 2MB, size of one huge page

    uint64_t start_address = 0x200000000000L;
    void* desired_addr = (void*) start_address;
    uint64_t stride = page_size;


    void* original_addr = mmap(desired_addr, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_FIXED | MAP_PRIVATE | MAP_HUGETLB , -1, 0);
    if (original_addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if(original_addr != desired_addr) {
        printf("original_addr(%p) does not equal desired_addr(%p)\n", original_addr, desired_addr);
        return EXIT_FAILURE;
    }

    printf("Original memory allocated with huge pages at %p\n", original_addr);

    // Example usage of memory
    int *arr = (int *)original_addr;
    arr[0] = 10;  // Just an example of writing to this memory
    printf("Value at arr[0]: %d\n", arr[0]);

    // Define the new address, ensuring it doesn't overlap with the original
    void* new_addr = (void *)start_address + page_size * 1024;
    printf("new_addr:%p\n", new_addr);

    // Remap the memory to the new address
    printf("before remap at arr[0]: %d\n", arr[0]);

    void *result = mremap(original_addr, page_size, page_size, MREMAP_DONTUNMAP | MREMAP_MAYMOVE | MREMAP_FIXED, new_addr);
    if (result == MAP_FAILED) {
        perror("mremap");
        exit(EXIT_FAILURE);
    }

    printf("after remap at arr[0]: %d\n", arr[0]);

    printf("Memory remapped with huge pages to new address %p\n", new_addr);

    // The original memory is still accessible and unchanged
    printf("Original address still valid, value at arr[0]: %d\n", arr[0]);
    int *new_arr = (int *)new_addr;
    new_arr[0] = 20;  // Write a different value in the new mapping
    printf("Value at new_arr[0] at new address: %d\n", new_arr[0]);
    printf("Original address still valid, value at arr[0]: %d\n", arr[0]);

    // Clean up
    munmap(original_addr, page_size);
    munmap(new_addr, page_size);

    return 0;
}
