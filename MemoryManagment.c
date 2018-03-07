#include <stdint.h>
#include <stdbool.h>
#include "MemoryManagment.h"

// Allocation
void* Calloc(int amount, int size);
void* Malloc(int size);
bool findFreeBlock(int allocSize, FreeBlock** foundBlock, bool* hasPrev, FreeBlock* prevBlock);
void handleBlockIsPerfectFit(FreeBlock* fBlock, bool hasPreeBlock, FreeBlock* preFoundBlock);
void updateFreeList(FreeBlock* foundBlock, int size, bool hasPreBlock, FreeBlock* preFoundBlock, int* extraAllocSpace);

// Free
void Free(void* addr);
void runMergeFreeList(FreeBlock* node);
bool findPreBlock(int startAddr, FreeBlock** foundBlock);
bool findPostBlock(int endAddr, FreeBlock** foundBlock);

// Utils
void storeOnHeap(void* from, int byteSize, int addr);
void memCopy(void* from, int byteSize, void* target);
int getFreeListCount(FreeBlock* root);



#define NULL 0x00000000
#define HEAP_SIZE 10000
#define MAGIC_BLOCK_NUMBER 86456231 //Used later in "Free", to check if we actually are freeing a block

FreeBlock* freeList;
uint8_t heap[HEAP_SIZE];
int blockSize, freeBlockSize;
bool memoryManagmentIsInit = false;


//************* Initalization *******************
// Must be called before actuall usage of any other functions
//********************************
void initHeapMemory(){
	FreeBlock firstBlock;
	//Set "Constants" for later use
	blockSize = sizeof(firstBlock.block);
	freeBlockSize = sizeof(firstBlock);


	firstBlock.block.size = HEAP_SIZE - blockSize; //Since if we wish to allocate on the heap, we must start with a headerSection 4 bit long
	firstBlock.block.startIndex = 0;
	firstBlock.hasNext = false;

	//Place first freeBlock on the heap
	storeOnHeap(&firstBlock, freeBlockSize, 0);
	//Make freeList point to the first block, currently heap index 0
	freeList = (FreeBlock*)&heap[0];
	memoryManagmentIsInit = true;
}


//********** Allocation **********
// Takes in the desired allocation size, and returns a pointer to the start of the allocated area
// 
// First we check the size can fit into one sequential area of the heap,
// if not, we return NULL (Throw an error)
//
// The area allocated will start with a header aka the struct "DataBlock".
// This header section will contain information about the allocated area.
//********************************
void* Calloc(int amount, int size){return Malloc(size * amount);}
void* Malloc(int size){
	if(memoryManagmentIsInit == false){
		//RaiseERROR("Not Yet INIT", "In Malloc", "MemoryManagment.c", "Line: 54");
		return NULL; //We did not manage to allocate *Throw Error*
	}

	size += blockSize; //Add overhead size

	FreeBlock* foundFreeBlock;
	FreeBlock* preFoundBlock;
	bool hasPreBlock;

	if(findFreeBlock(size, &foundFreeBlock, &hasPreBlock, preFoundBlock) == false){
		//RaiseERROR("Found no FBlock", "In Malloc", "MemoryManagment.c", "Line: 58");
		return NULL; //We did not manage to allocate *Throw Error*
	}

	int extraAllocSpace = 0;
	updateFreeList(foundFreeBlock, size, hasPreBlock, preFoundBlock, &extraAllocSpace);
	size += extraAllocSpace; // If we get a gap to small for a new freeBlock, we simply allocate all the inbetween space

	//Create the new allocatedBlock
	DataBlock newBlock;
	newBlock.startIndex = foundFreeBlock->block.startIndex;
	newBlock.size = size;
	newBlock.magicNumber = MAGIC_BLOCK_NUMBER;
	storeOnHeap(&newBlock, newBlock.size, newBlock.startIndex);

	return (void*)((&heap[newBlock.startIndex]) + blockSize); //Add offset of block size
}

//After we have allocated something we do the required updates to the free list, such as splitting or shrinking current blocks
void updateFreeList(FreeBlock* foundBlock, int size, bool hasPreBlock, FreeBlock* preFoundBlock, int* extraAllocSpace){
	int currentBlockEndAddr = foundBlock->block.startIndex + foundBlock->block.size;
	int newFreeStartAddr = foundBlock->block.startIndex + size;

	if(newFreeStartAddr < currentBlockEndAddr){ //We need to create a new block
		int sizeLeft = currentBlockEndAddr - newFreeStartAddr;


		if(sizeLeft <= freeBlockSize){ //Gap is to small to create a new Free block
			*extraAllocSpace = sizeLeft;
			handleBlockIsPerfectFit(foundBlock, hasPreBlock, preFoundBlock);
			return;
		}

		//We create a new FreeBlock
		FreeBlock newFree;
		newFree.block.startIndex = newFreeStartAddr;
		newFree.block.size = sizeLeft;
		newFree.hasNext = foundBlock->hasNext;
		newFree.next = foundBlock->next;


		//Put it on the heap
		storeOnHeap(&newFree, freeBlockSize, newFree.block.startIndex);

		if(hasPreBlock == false)//Update free list
			freeList = (void*)&heap[newFree.block.startIndex];
		else//Update prev block
			preFoundBlock->next = (void*)&heap[newFree.block.startIndex];
	}
	else{//We used up the space perfectly
		handleBlockIsPerfectFit(foundBlock, hasPreBlock, preFoundBlock);
	}
		
}

void handleBlockIsPerfectFit(FreeBlock* fBlock, bool hasPreeBlock, FreeBlock* preFoundBlock){
	if(hasPreeBlock == false && fBlock->hasNext){
		freeList = (FreeBlock*)fBlock->next;
		return;
	}
	if(hasPreeBlock){
		preFoundBlock->hasNext = false;
		return;
	}
	//RaiseERROR("Used all Memory", "In Malloc", "MemoryManagment.c", "Line: 125");
}



//Simple implementation of the "First Fit" policy
bool findFreeBlock(int allocSize, FreeBlock** foundBlock, bool* hasPrev, FreeBlock* prevBlock){
	FreeBlock* currentBlock = freeList;
	*hasPrev = false;

	while(currentBlock->block.size < allocSize){
		if(currentBlock->hasNext == false)
			return false; //we did not find any block that is big enough

		*hasPrev = true;
		prevBlock = currentBlock;
		currentBlock = ((FreeBlock*)currentBlock->next);
	}

	*foundBlock = currentBlock; //Set output value
	return true;
}





//********** Free ***************
// Takes in a pointer to the heap
// 
// First we check the pointer is pointing to the a "DataBlock"
// This check is done by dereferencing the adress to a datablock, mathing its Magic Value
//
// If everything matches, we create a new FreeBlock containing the same size as the block we are freeing.
// We then insert this new block into the FreeList, and perform the required merges of free blocks.
//********************************
void Free(void* addr){
	if(memoryManagmentIsInit == false){
		//RaiseERROR("Not Yet INIT", "In Malloc", "MemoryManagment.c", "Line: 164");
		return NULL; //We did not manage to allocate *Throw Error*
	}

	DataBlock* targetBlock = addr-blockSize;
	if(targetBlock->magicNumber != MAGIC_BLOCK_NUMBER){
		//RaiseERROR("Unmatching Magic", "In Free", "MemoryManagment.c", "Line: 157");
		return NULL;
	}
	//debugInt(targetBlock->startIndex);
	FreeBlock* preBlock;
	FreeBlock* postBlock;

	//Put the new block on the heap
	FreeBlock* newFreeBlock = (FreeBlock*)(&heap[targetBlock->startIndex]);
	newFreeBlock->block = *targetBlock;

	bool hasPreBlock = findPreBlock(targetBlock->startIndex, &preBlock);
	bool hasPostBlock = findPostBlock(targetBlock->startIndex, &postBlock);

	//Insert into freeList
	if(hasPreBlock){
		preBlock->hasNext = true;
		preBlock->next = newFreeBlock;
	}
	else
		freeList = newFreeBlock; // Put first in free list

	if(hasPostBlock){
		newFreeBlock->hasNext = true;
		newFreeBlock->next = postBlock;
	}
	else
		newFreeBlock->hasNext = false;

	runMergeFreeList(freeList);	
}

//Lazy Iteration over the FreeList and merges blocks
void runMergeFreeList(FreeBlock* node){
	if(node->hasNext == false)
		return;

	FreeBlock* nextBlock = (FreeBlock*)node->next;
	int endIndex = node->block.startIndex + node->block.size;

	if(endIndex == nextBlock->block.startIndex){//Merge
		node->block.size += nextBlock->block.size;
		node->hasNext = nextBlock->hasNext;
		node->next = nextBlock->next;

		runMergeFreeList(node);//Since we hijacked next blocks next, we run again
	}
	else
		runMergeFreeList(nextBlock); //Skip and look at next block
}

bool findPreBlock(int startAddr, FreeBlock** foundBlock){
	FreeBlock* currentBlock = freeList;
	FreeBlock* preBlock;

	if(currentBlock->block.startIndex >= startAddr){
		return false;
	}

	while(currentBlock->block.startIndex <= startAddr){
		if(currentBlock->hasNext == false)
			break;

		preBlock = currentBlock;
		currentBlock = (FreeBlock*)currentBlock->next;
	}

	*foundBlock = preBlock;
	return true;
}
bool findPostBlock(int addr, FreeBlock** foundBlock){
	FreeBlock* currentBlock = freeList;

	if(currentBlock->block.startIndex >= addr){
		*foundBlock = currentBlock;
		return true;
	}


	while(currentBlock->block.startIndex < addr){
		if(currentBlock->hasNext == false) 
			return false;

		currentBlock = (FreeBlock*)currentBlock->next;

		if(currentBlock->block.startIndex >= addr){
			*foundBlock = currentBlock;
			return true;
		}
	}

	return false;
}




//********** Utils **************
// Some basic functions we make use of
//*******************************
void storeOnHeap(void* from, int byteSize, int addr){memCopy(from, byteSize, (void*)&heap[addr]);}
void memCopy(void* from, int byteSize, void* target){
	int i;
	bool* tempFrom = (bool*)from;
	bool* tempTarget = (bool*)target;
	for (i = 0; i < byteSize; i++)
		tempTarget[i] = tempFrom[i];
}

FreeBlock* getFreeList(){return freeList;}
int getFreeListCount(FreeBlock* root){
	if(root->hasNext == false)
		return 1;
	return 1 + getFreeListCount((FreeBlock*)root->next);
}


int getFreeListSize(){return iterateFreeListSize(freeList);}
int iterateFreeListSize(FreeBlock* node){
	if(node->hasNext)
		return node->block.size + iterateFreeListSize((FreeBlock*)node->next);
	else
		return node->block.size;
}
