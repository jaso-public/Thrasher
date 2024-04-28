#include <stdio.h>

// int add ( /* you need specify how the function is called */) {
int add (int len, int array[]) {
    int sum = 0;
    for(int i=0 ; i<len ; i++) {
        sum += array[i];
    }
    return sum;
}

int main() {
    int a[] = {2,3,7,9};
    int sum = add(sizeof(a)/sizeof(int), a);
    printf("sum = %d\n", sum);
}