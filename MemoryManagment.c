#include <stdint.h>
#include <stdbool.h>
#include "MemoryManagment.h"
#include "MemoryErrorManager.h"

// Allocation
void* Calloc(int amount, int size);
void* Malloc(int size);
bool findFreeBlock(FreeBlock* currentNode, int allocSize, FreeBlock** output);
void updateFreeList(FreeBlock* foundBlock, int size, int* extraAllocSize);

// Free
void Free(void* addr);
void runMergeFreeList(FreeBlock* node);

//Search
bool findPreBlock(FreeBlock* currentNode, int addr, FreeBlock** output);
bool findPostBlock(FreeBlock* currentNode, int addr, FreeBlock** output);
bool searchFreeList(FreeBlock* node, bool (*foundCheck)(FreeBlock* currentNode, int pos, FreeBlock** output), FreeBlock** outputNode, int searchValue);

// Utils
void insertFreeBlock(FreeBlock* newBlock);
void removeFreeBlock(FreeBlock* block);
void createNewFreeBlock(int pos, int size);
void storeOnHeap(void* from, int byteSize, int addr);
void memCopy(void* from, int byteSize, void* target);

#define HEAP_SIZE 1000 //Constant that deceides the size of the heap
#define MAGIC_BLOCK_NUMBER 86456231 //Used later in "Free", to check if we actually are freeing an allocated block
#define MAGIC_BLOCK_NUMBER2 93866231//Used later in "Free", to check so that we don't free an already free block

FreeBlock* freeList;
uint8_t heap[HEAP_SIZE];
int blockSize, freeBlockSize;
bool memoryManagmentIsInit = false;
bool hasFreeList = false;



//************* Initalization *******************
// Must be called before actuall usage of any other functions
//********************************
void initHeapMemory(){
    FreeBlock firstBlock;
    //Set "Constants" for later use
    blockSize = sizeof(firstBlock.block);
    freeBlockSize = sizeof(firstBlock);

    firstBlock.block.size = HEAP_SIZE;
    firstBlock.block.startIndex = 0;
    firstBlock.hasNext = false;
    firstBlock.block.magicNumber = MAGIC_BLOCK_NUMBER;

    //Place first freeBlock on the heap
    storeOnHeap(&firstBlock, freeBlockSize, 0);
    //Make freeList point to the first block, currently heap index 0
    freeList = (FreeBlock*)&heap[0];

    memoryManagmentIsInit = true;
    hasFreeList = true;
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
        throwNotYetInit();
        return NULL; //We did not manage to allocate *Throw Error*
    }

    size += freeBlockSize; //Add overhead size
    FreeBlock* foundFreeBlock;

    if(searchFreeList(freeList,findFreeBlock, &foundFreeBlock, size) == false){
        throwFoundNoMatchingFreeBlock();
        return NULL; //We did not manage to allocate *Throw Error*
    }

    int extraAllocSpace = 0;
    updateFreeList(foundFreeBlock, size, &extraAllocSpace);
    size += extraAllocSpace;

    //Create the new allocatedBlock
    int allocPos = foundFreeBlock->block.startIndex;
    DataBlock newBlock;
    newBlock.startIndex = foundFreeBlock->block.startIndex;
    newBlock.size = size;
    newBlock.magicNumber = MAGIC_BLOCK_NUMBER;

  //SimulateAlloc(newBlock.startIndex, newBlock.size);   Used for the Albot.Online heap simulation
    storeOnHeap(&newBlock, blockSize, newBlock.startIndex);
    return (void*)((&heap[allocPos]) + blockSize); //Add offset of block size
}

//After we have allocated something we do the required updates to the free list, such as splitting or shrinking current blocks
void updateFreeList(FreeBlock* foundBlock, int size, int* extraAllocSize){
    int currentBlockEndAddr = foundBlock->block.startIndex + foundBlock->block.size;
    int newFreeStartAddr = foundBlock->block.startIndex + size;
    removeFreeBlock(foundBlock);

    if(newFreeStartAddr < currentBlockEndAddr){ //We need to create a new free block
        int sizeLeft = currentBlockEndAddr - newFreeStartAddr;
        //If the size that is left is smaller than the overhead block of a free block, then we include that space in the allocation
        if(sizeLeft < freeBlockSize){
            *extraAllocSize += sizeLeft;
            return;
        }
        createNewFreeBlock(newFreeStartAddr, sizeLeft);
    }
}

//Simple implementation of the "First Fit" policy
bool findFreeBlock(FreeBlock* currentNode, int allocSize, FreeBlock** output){
    if(currentNode->block.size >= allocSize){
        *output = currentNode;
        return  true;
    }
    return false;
}





//********** Free ***************
// Takes in a pointer to the heap
//
// First we check the pointer is pointing to the a "DataBlock"
// This check is done by dereferencing the adress to a datablock, mathing its Magic Value
//
// Also added an addition check so see if the freed adress matches an already free block Magic number 2.
// This results in an error being thrown if such is the case.
//
// If everything matches, we create a new FreeBlock containing the same size as the block we are freeing.
// We then insert this new block into the FreeList, and perform the required merges of free blocks.
//********************************
void Free(void* addr){
    if(memoryManagmentIsInit == false){
        throwNotYetInit();
        return;
    }

    //It seems some compilers don't like the command:
    //      DataBlock* targetBlock = addr-blockSize;
    //So to counter this we cast it to a char* first

    DataBlock* targetBlock = ((char*)addr)-blockSize;
    FreeBlock* tempBlock = ((char*)addr)-blockSize;
    if(targetBlock->magicNumber != MAGIC_BLOCK_NUMBER || tempBlock->magicNumberFree == MAGIC_BLOCK_NUMBER2){
        throwFreeNonAllocatedSpace();
        return;
    }

   // SimulateFree(targetBlock->startIndex);   Used for the Albot.Online heap simulation
    createNewFreeBlock(targetBlock->startIndex, targetBlock->size);
    runMergeFreeList(freeList);
}

//Lazy Iteration over the FreeList and merges blocks
void runMergeFreeList(FreeBlock* node){
    if(node->hasNext == false)
        return;

    FreeBlock* nextBlock = (FreeBlock*)node->next;
    int endIndex = node->block.startIndex + node->block.size;

    if(endIndex == nextBlock->block.startIndex){//Merge
       // SimulateMerge(node->block.startIndex, nextBlock->block.startIndex);   Used for the Albot.Online heap simulation
        node->block.size += nextBlock->block.size;
        node->hasNext = nextBlock->hasNext;
        node->next = nextBlock->next;
        removeFreeBlock(nextBlock);

        runMergeFreeList(node);//Since we hijacked "nextBlocks", "next" we run again
    }
    else
        runMergeFreeList(nextBlock); //Skip and look at next block
}




//********** Search **************
// Basic search functions for finding blocks in the freeList
//*******************************
bool findPreBlock(FreeBlock* currentNode, int addr, FreeBlock** output){
    if(currentNode->block.startIndex >= addr)
        return false;

    if(currentNode->hasNext){
        int nextAddr = ((FreeBlock*)currentNode->next)->block.startIndex;
        if(nextAddr >= addr){
            *output = currentNode;
            return  true;
        }
    }
    else{
        *output = currentNode;
        return  true;
    }

    return false;
}


bool findPostBlock(FreeBlock* currentNode, int addr, FreeBlock** output){
    if(currentNode->block.startIndex > addr){
        *output = currentNode;
        return  true;
    }

    return false;
}

//Nifty-thrifty search, making use of "foundCheck". Allowing us to re-use the same search function for different searches
bool searchFreeList(FreeBlock* node, bool (*foundCheck)(FreeBlock* currentNode, int pos, FreeBlock** output), FreeBlock** outputNode, int searchValue){
    if(hasFreeList == false)
        return false;
    if(foundCheck(node, searchValue, outputNode))
        return true;
    if(node->hasNext)
        return searchFreeList((FreeBlock*)node->next, foundCheck, outputNode, searchValue);
    return false;
}





//********** Utils **************
// Some basic functions we make use of
//*******************************
void createNewFreeBlock(int pos, int size){
    FreeBlock newFree;
    newFree.block.startIndex = pos;
    newFree.block.size = size;
    newFree.magicNumberFree = MAGIC_BLOCK_NUMBER2;

    storeOnHeap(&newFree, freeBlockSize, pos);
    insertFreeBlock(&heap[pos]);
}


void removeFreeBlock(FreeBlock* block){
    if(hasFreeList == false){
        throwNotYetInit();
        memoryManagmentIsInit = false;
        return;
    }

    FreeBlock* preBlock;
    FreeBlock* postBlock;
    bool hasPreBlock = searchFreeList(freeList, findPreBlock, &preBlock, block->block.startIndex);
    bool hasPostBlock = searchFreeList(freeList, findPostBlock, &postBlock, block->block.startIndex);

    if(hasPreBlock){
        preBlock->hasNext = hasPostBlock;
        preBlock->next = postBlock;
    } else if(hasPostBlock){
        freeList = postBlock;
    } else{
        hasFreeList = false;
    }
}

void insertFreeBlock(FreeBlock* newBlock){
    if(hasFreeList == false){
        freeList = newBlock;
        newBlock->hasNext = false;
        hasFreeList = true;
        return;
    }

    FreeBlock* preBlock;
    FreeBlock* postBlock;
    bool hasPreBlock = searchFreeList(freeList, findPreBlock, &preBlock, newBlock->block.startIndex);
    bool hasPostBlock = searchFreeList(freeList, findPostBlock, &postBlock, newBlock->block.startIndex);

    if(hasPreBlock){
        preBlock->hasNext = true;
        preBlock->next = newBlock;
    }
    else{
        freeList = newBlock;
    }

    newBlock->hasNext = hasPostBlock;
    newBlock->next = postBlock;
    hasFreeList = true;
}




void storeOnHeap(void* from, int byteSize, int addr){memCopy(from, byteSize, (void*)&heap[addr]);}
void memCopy(void* from, int byteSize, void* target){
    bool* tempFrom = (bool*)from;
    bool* tempTarget = (bool*)target;
    for (int i = 0; i < byteSize; i++)
        tempTarget[i] = tempFrom[i];
}


void* getHeapStart(){
    return (void*)heap;
}
