#include <stdio.h>

int main() {
    for(int i=0; i<=30; i++) {
        printf("%d\t%d\n", i, (1<<i));
    }
}