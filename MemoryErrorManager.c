#include <stdio.h>

void throwNotYetInit(){
    printf("MemoryManager is not yet setup properly.\n");
}

void throwFoundNoMatchingFreeBlock(){
    printf("Found no matching free Block\n");
}

void throwFreeNonAllocatedSpace(){
    printf("Tried to free non-allocated space!");
}