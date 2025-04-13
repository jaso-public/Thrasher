#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

int main() {
    // Number of pages
    const int num_pages = 100;

    // Get the system's page size
    size_t page_size = 0;
#ifdef __APPLE__
    page_size = getpagesize();
#else
    page_size = sysconf(_SC_PAGE_SIZE);
#endif    
    if (page_size == -1) {
        perror("sysconf");
        exit(EXIT_FAILURE);
    }

    printf("page_size = %zu\n", page_size);

    // Create an anonymous memory mapping of one page
    void *base_addr = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (base_addr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Alias this memory 100 times using MAP_FIXED | MAP_SHARED
    for (int i = 1; i <= num_pages; i++) {
        void *new_addr = (char *)base_addr + (i * page_size);
        void *result = mmap(new_addr, page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (result == MAP_FAILED) {
            perror("mmap alias");
            exit(EXIT_FAILURE);
        }
    }

    // Now, base_addr and the subsequent 100 addresses each of size page_size point to the same physical memory

    // Example: Write to base_addr and read from one of the aliased addresses
    for (int i = 1; i <= num_pages; i++) {
        snprintf((char *)base_addr + page_size * i, page_size, "page %d", i);
    }

    for (int i = 1; i <= num_pages; i++) {
        printf("Reading from an aliased address: %s\n", (char *)base_addr + page_size * i);
    }

    // Clean up: Unmap all the mappings
    for (int i = 0; i <= num_pages; i++) {
        if (munmap((char *)base_addr + (i * page_size), page_size) == -1) {
            perror("munmap");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

