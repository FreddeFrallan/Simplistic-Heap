#include <stdbool.h>


typedef struct{
    int size, startIndex;
    int magicNumber;
}DataBlock;

typedef struct{
    int magicNumberFree;
    DataBlock block;
    void* next;
    bool hasNext;
}FreeBlock;


void initHeapMemory();
void* Calloc(int amount, int size);
void* Malloc(int size);
void Free(void* addr);


void* getHeapStart();