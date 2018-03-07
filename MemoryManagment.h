#include <stdbool.h>


typedef struct{ // size is: 8 byte
	int size, startIndex;
	int magicNumber;
}DataBlock;

typedef struct{ // size is: 16 byte
	DataBlock block;
	void* next;
	bool hasNext;
}FreeBlock;




void initHeapMemory();
void* Calloc(int amount, int size);
void* Malloc(int size);
void Free(void* addr);

FreeBlock* getFreeList();
int getFreeListSize();