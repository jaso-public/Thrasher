#include <stdio.h>
#include <stdlib.h>

// allocates memory and exits with an error code if malloc fails
// you can use this function for allocation.  If this function
// returns, it will return a pointer to newly allocated memory
void* checked_malloc(int size) {
    void* result = malloc(size);
    if(!result) {
        perror("malloc");
        exit(1);
    }
}

// creates a square matrix and populates it with some values
// if create_matrix returns, it will return a pointer to 
// a properly contrusted heap allocated 2D array.
int** create_matrix(int size) {
    // code to create and populate the matrix has been elided
}

// takes a heap allocated 2D array as input and
// creates a new heap allocated 2D array that is 
// the transpose of the existing matrix
int** transpose_matrix(int size, int** existing) {

    // YOU NEED TO WRITE THIS CODE.

}

void print_matrix(int size, int** elements) {
    // code to print the matrix has been elided
}

int main() {
    int size = 4;
    int** matrix = create_matrix(size);
    int** transposed = transpose_matrix(size, matrix);
    
    printf("\noriginal matrix:\n");
    print_matrix(size, matrix);
    printf("\nafter transpose:\n");
    print_matrix(size, transposed);
}