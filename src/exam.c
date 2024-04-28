#include <stdio.h>
#include <stdlib.h>


// creates a square matrix and populates it with some values
// if create_matrix returns, it will return a pointer to 
// a properly contrusted heap allocated 2D array.
int** create_matrix(int size) {
    int** result = malloc(sizeof(int*) * size);
    for(int i=0; i<size; i++) {
        result[i] = malloc(sizeof(int) * size);
        for(int j=0 ; j<size; j++) {
            result[i][j] = i * 10 + j;
        }
    }
    return result;
}

// takes as input a heap allocated 2D array and
// creates a new heap allocated that is the transpose
// of the existing matrix
int** transpose_matrix(int size, int** existing) {
    int** result = create_matrix(size);
    if(result != NULL) {
        for(int i=0; i<size; i++) {
            for(int j=0 ; j<size; j++) {
                result[j][i] = existing[i][j];
            }
        }
    }
    return result;
}

void print_matrix(int size, int** elements) {
    for(int i=0; i<size; i++) {
        for(int j=0 ; j<size; j++) {
            printf("%2d ",elements[i][j]);
        }
        printf("\n");
    }
}

int main() {
    int size = 4;
    int** matrix = create_matrix(size);
    if(matrix != NULL) {
        int** transposed = transpose_matrix(size, matrix);
        if(transposed != NULL) {
            printf("\noriginal matrix:\n");
            print_matrix(size, matrix);
            printf("\nafter transpose:\n");
            print_matrix(size, transposed);
        } else {
            printf("failed to transpose matrix.\n");
        }
    } else {
        printf("failed to create matrix.\n");
    }
}