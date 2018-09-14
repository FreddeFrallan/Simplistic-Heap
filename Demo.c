#include <stdio.h>
#include "MemoryManagment.h"

int* allocateNumbersOnHeap();

int main(){
    //Before we can make use of the heap we need to call this function
    //This only needs to be called once!
    initHeapMemory();
    int* p =allocateNumbersOnHeap(0);

    //Print the numbers that are allocated on the heap
    printf("First Loop");
    for(int i = 0; i < 10; i++){
        printf("\nAt pos %p we got number: %i", &p[i], p[i]);
    }

    //Free up the memory used in p so we can make use of it again
    Free(p);

    //Allocate a new pointer and see if it re-uses the freed up memory
    int* p2 =allocateNumbersOnHeap(42);
    //Now the values at the old addresses should be altered
    printf("\n\nSecond Loop");
    for(int i = 0; i < 10; i++){
        printf("\nAt pos %p we got number: %i", &p[i], p[i]);
    }
}

//Function that creates an array on the static heap and returns a pointer
int* allocateNumbersOnHeap(int startNumber){
    int* p = Malloc(sizeof(int) * 10);
    for(int i = 0; i < 10; i++){
        p[i] = startNumber + i;
    }
    return p;
}