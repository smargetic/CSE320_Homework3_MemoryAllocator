/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>


#define PREV_BLOCK_ALLOCATED  0x1
#define THIS_BLOCK_ALLOCATED  0x2

static int firstMal = 0;
sf_block* splitBlock(sf_block* block, int size);
void add2Free(sf_block* block, int loc);
sf_block* change2Allo(sf_block* block, int sizeFreeBlock, int sizeNeed, int loc);

int isPrevAlloc(sf_block* block);
int findingPostion(int newSize);
sf_block* coalescing(sf_block* block);

void* expandPage(){
    void* x = sf_mem_grow();
    if(x==NULL){
        sf_errno = ENOMEM;
        return NULL;
    }

    sf_block* ptr2Block =(sf_block*)(x-16);
    ptr2Block->header = 4096;
    int prevAllo = ((ptr2Block->prev_footer)^sf_magic())&THIS_BLOCK_ALLOCATED;
    if(prevAllo==2){
        ptr2Block->header = ptr2Block->header|PREV_BLOCK_ALLOCATED;
    }
    sf_footer* prevFoot = (sf_footer*)((void*)ptr2Block);
    ptr2Block->prev_footer = *prevFoot;

    *(sf_footer*)((void*)ptr2Block + 4096) = ptr2Block->header ^ sf_magic();

    //create new epilogue
    sf_epilogue* ptr2End = (sf_epilogue*)(sf_mem_end()-8);
    ptr2End->header =0;
    ptr2End->header = (ptr2End->header)|(THIS_BLOCK_ALLOCATED); //I make sure the epi is allo


    //I add it to the free list
    add2Free(ptr2Block, 7);

    ptr2Block = coalescing(ptr2Block);

    return ptr2Block;

}

void initializeFree(){
    for(int i=0; i<9; i++){
        //I set the next and prev pointers to themselves
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
}

void add2Free(sf_block* block, int loc){
    block->body.links.next = sf_free_list_heads[loc].body.links.next;
    block->body.links.prev = &sf_free_list_heads[loc];
    //block->body.links.next = sf_free_list_heads[loc].body.links.next;
    sf_free_list_heads[loc].body.links.next->body.links.prev = block;
    sf_free_list_heads[loc].body.links.next= block;

}
void removeFromFree(sf_block* block){

    block->body.links.prev->body.links.next = block->body.links.next;
    block->body.links.next->body.links.prev = block->body.links.prev;

}

sf_block* splitBlock(sf_block* block, int sizeNeed){
    //I check that the size of block allows me to split it


    int blockSize = (block->header)&(BLOCK_SIZE_MASK);
    int prevAlloTag =-1;
    if(((block->header)&PREV_BLOCK_ALLOCATED)==1){
        //debug("PREV BLOCK WAS ALLO");
        prevAlloTag = 0;
    }
    int difSize = blockSize-sizeNeed;
    if(difSize<32){
        //I change current to allo and next to allo
        removeFromFree(block);
        block->header = (block->header)&(~THIS_BLOCK_ALLOCATED);
        *(sf_footer*)((void*)block + blockSize) = block->header ^ sf_magic();

        return block;
    }
    removeFromFree(block);
    //I changed the front block to be allocated
    block->header = sizeNeed;
    if(prevAlloTag==0){
        block->header = block->header|PREV_BLOCK_ALLOCATED;
    }
    block->header = (block->header)|(THIS_BLOCK_ALLOCATED);
    *(sf_footer*)((void*)block + sizeNeed) = block->header ^ sf_magic();
    //create the following freed block
    sf_block* ptr2NextBlock = ((void*)block+sizeNeed);
    ptr2NextBlock->header = difSize;
    ptr2NextBlock->header = ptr2NextBlock->header|PREV_BLOCK_ALLOCATED;
    *(sf_footer*)((void*)ptr2NextBlock + difSize) = ptr2NextBlock->header ^ sf_magic();


    int loc = findingPostion(difSize);
    add2Free(ptr2NextBlock, loc);
;
    return block;

}
sf_block* splitBlock2(sf_block* block, int sizeNeed){

    //I check that the size of block allows me to split it


    int blockSize = (block->header)&(BLOCK_SIZE_MASK);
    int prevAlloTag =-1;
    if(((block->header)&PREV_BLOCK_ALLOCATED)==1){
        prevAlloTag = 0;
    }
    int difSize = blockSize-sizeNeed;
    if(difSize<32){
        return block;
    }
    removeFromFree(block);
    //I changed the front block to be allocated
    block->header = sizeNeed;
    //debug("SIZE NEEDED \t%d", sizeNeed);
    if(prevAlloTag==0){
        block->header = block->header|PREV_BLOCK_ALLOCATED;
    }
    block->header = (block->header)|(THIS_BLOCK_ALLOCATED); //CHANGED
    *(sf_footer*)((void*)block + sizeNeed) = block->header ^ sf_magic();

    //create the following freed block
    sf_block* ptr2NextBlock = ((void*)block+sizeNeed);
    ptr2NextBlock->header = difSize;
    ptr2NextBlock->header = ptr2NextBlock->header|PREV_BLOCK_ALLOCATED;
    *(sf_footer*)((void*)ptr2NextBlock + difSize) = ptr2NextBlock->header ^ sf_magic();


    int loc = findingPostion(difSize);
    add2Free(ptr2NextBlock, loc);

    return block;

}

sf_block* change2Allo(sf_block* block, int sizeFreeBlock, int sizeNeed, int loc){
    //I make sure the current block is free
    //debug("\nCHANGE2ALLO");
    if(((block->header)&THIS_BLOCK_ALLOCATED)== 2){
        //the block is already allocated
        abort();
    }
    //first check if next block is epilogue
    block = splitBlock(block, sizeNeed);

    return block;
}

int isFree(sf_block* block){

    if(((block->header)&THIS_BLOCK_ALLOCATED)==(1)){
        return -1;
    }
    return 0;
}


int isNextPrevFree(sf_block* block, int sizeBlock){
    //the next block should be at the location of the block + sizeFreeBlock
    if((block+sizeBlock)==(sf_mem_end()-16)){
        sf_epilogue* epi = (sf_epilogue*)(sf_mem_end()-8);
        //I check if prev was allocated
        if(((epi->header)&(PREV_BLOCK_ALLOCATED))==1){
            //prev set to allocated
            return -1;
        }
    } else{
        sf_block* nextBlock = (sf_block*)(block+sizeBlock);
        if(((nextBlock->header)&(PREV_BLOCK_ALLOCATED))==1){
            //prev set to allocated
            return -1;
        }
    }
    return 0;
}

sf_block* coalescing(sf_block* block){
    //I find out if the left block was allocated or not
    int blockSize = (block->header)&BLOCK_SIZE_MASK;
    int prevAllo = (block->header)&PREV_BLOCK_ALLOCATED;
    //debug("VALUE OF PREVALLO\t%d", prevAllo);
    if (prevAllo!=1){
        //the free block is located to the left
        //I remove the 2 blocks from the free list
        removeFromFree(block);

        int prevSize = (block->prev_footer^sf_magic())&BLOCK_SIZE_MASK;
        block = (sf_block*)((void*)block-prevSize);
        removeFromFree(block);

        int prevPrevTag= (block->header)&PREV_BLOCK_ALLOCATED;

        // I create a new block with all appropriate fields
        block->header = (blockSize+prevSize);
        //debug("PREV PREV VALUE \t %d", prevPrevTag);
        if(prevPrevTag==1){
            //previous was allocated
            block->header = block->header|PREV_BLOCK_ALLOCATED;
        }
        *(sf_footer*)((void*)block + blockSize+prevSize) = block->header ^ sf_magic();

        //I add to free list
        int loc = findingPostion((blockSize+prevSize));
        add2Free(block, loc);
        //debug("---------------AFTER COAL1");
        //sf_show_heap();
        blockSize = blockSize + prevSize;
    }
    blockSize = (block->header)&BLOCK_SIZE_MASK;
    //I go to the next block to see if it's allocated

    //int nextBlockAllo =2;
    //if((size_t*)((void*)block+blockSize)!=(size_t*)(sf_mem_end()-16)){
    //    nextBlockAllo = *(sf_header*)((void*)block + blockSize+8)&THIS_BLOCK_ALLOCATED;
    //}

    int nextBlockAllo = *(sf_header*)((void*)block+blockSize+8)&THIS_BLOCK_ALLOCATED;
    //debug("VALUE IN NEXTBLOCKALLO: \t%d", nextBlockAllo);
    //debug
    if(nextBlockAllo!=2){
        sf_block* nextBlock = (sf_block*)((void*)block+blockSize);
        int nextSize = (nextBlock->header)&BLOCK_SIZE_MASK;
        removeFromFree(nextBlock);
        removeFromFree(block);
        int blockSize = (block->header)&BLOCK_SIZE_MASK;
        //prev is allocated because was checked in previous loop

        //I edit the new block
        block->header = (nextSize+blockSize);
        block->header = block->header|PREV_BLOCK_ALLOCATED;
        *(sf_footer*)((void*)block + blockSize + nextSize) = block->header ^ sf_magic();

        //add to free list
        int loc = findingPostion((blockSize+nextSize));
        add2Free(block, loc);
    }
    return block;
}



sf_block* traverseAndChange2Allo(int loc, int targetSize){
    sf_block* ptr2Block = sf_free_list_heads[loc].body.links.next;

    while(ptr2Block!=&sf_free_list_heads[loc]){

        int sizeCurrentBlock = ptr2Block->header & BLOCK_SIZE_MASK;

        if(targetSize<=sizeCurrentBlock){
            //I have found the block that I was looking for
            change2Allo(ptr2Block, sizeCurrentBlock, targetSize, loc);
            break;
        }
        ptr2Block = ptr2Block->body.links.next;
    }

    return ptr2Block;
}

int traverse(int targetSize, int loc){
    //I create a pointer to the location in the free list
    sf_block* ptr2Block = sf_free_list_heads[loc].body.links.next;
    int targetFound = -1;
    int tempCount =0;
    while(ptr2Block!=&sf_free_list_heads[loc]){
        int tempSize = ptr2Block->header;
        if(targetSize<=tempSize){
            return 0;
        }
        tempCount++;
        ptr2Block = ptr2Block->body.links.next;
    }
    return targetFound;
}

int findingPostion(int size){
    int m = 32;
    if (size==m){
        return 0;
    } else if ((size>(m))&&(size<=(2*m))){
        return 1;
    } else if ((size>(2*m))&&(size<=(4*m))){
        return 2;
    } else if ((size>(4*m))&&(size<=(8*m))){
        return 3;
    } else if (((size>(8*m))&&(size<=(16*m)))){
        return 4;
    } else if (((size>(16*m))&&(size<=(32*m)))){
        return 5;
    } else if ((size>(32*m))&&(size<=(64*m))){
        return 6;
    } else if((size>(64*m))&&(size<=(128*m))){
        return 7;
    } else if ((size>(128*m))){
        return 8;
    }

    return -1;
}


int isPrevAlloc(sf_block* block){
    sf_footer prevFoot = (block->prev_footer)^sf_magic();
    //I will go through the heap, till I find the block that I need
    int prevAllo = (prevFoot&THIS_BLOCK_ALLOCATED);
    if(prevAllo==2){
        //it is set
        return 0;
    }
    return -1;

}

void *sf_malloc(size_t size) {
    if (firstMal==0){
        //I first grow the heap
        sf_mem_grow();

        //Prologue Initialization
        sf_prologue* ptr2Begin = (sf_prologue*)sf_mem_start();
        ptr2Begin->header = 32;
        ptr2Begin->header = ptr2Begin->header|THIS_BLOCK_ALLOCATED;
        ptr2Begin->header = ptr2Begin->header|PREV_BLOCK_ALLOCATED;
        ptr2Begin->footer = ptr2Begin->header^sf_magic();

        //Block Initialization

        sf_block* block = (sf_block*)(sf_mem_start()+32);
        block->prev_footer = ptr2Begin->footer;
        block->header = 4048;
        block->header = block->header|PREV_BLOCK_ALLOCATED;
        sf_footer* ptr2FOOT = (sf_footer*)(sf_mem_end()-16);
        *(ptr2FOOT) = block->header^sf_magic();


        //Epilogue Initialization
        sf_epilogue* ptr2End = (sf_epilogue*)(sf_mem_end()-8);
        ptr2End->header = 0;
        ptr2End->header = ptr2End->header|THIS_BLOCK_ALLOCATED;
        //ptr2End->header = ptr2End->header|PREV_BLOCK_ALLOCATED;

        firstMal = 1;
        //I then set up my free list
        initializeFree();
        //add to my free list
        add2Free(block, 7);

    }



    if(size==0){
        return NULL;
    }
    //I see what multiple of 16 goes into the size
    if(size<16){
        size = 16;
    }
    size_t fullSize=0;
    fullSize = size + 16; //+ 16 for header and footer


    size_t newSize =0;
    newSize = ((fullSize-1)|15)+1;

    int freeLoc = findingPostion(newSize);
    if(freeLoc==-1){
        abort(); //Some sort of error --> not even possible to have
    }

    for(int i=freeLoc; i<9; i++){
        int found = traverse(newSize, i);
        if (found ==0){
            //the necessary free location is present, and I get the pointer to the new allocated location
            sf_block* alloBlock = traverseAndChange2Allo(i, newSize);
            void* finalBlock = (void*)alloBlock + 16;
            return finalBlock;
            break;
        } else if((found!=0)&&(freeLoc==8)){
            //the last position does not have it, therefore, I must expand the page
            int found2 = -1;
            int count =1;
            while(found2==-1){
                if(expandPage()==NULL){
                    return NULL;
                }

                //I check if the necessary size is in the location
                for(int j=freeLoc; j<9;j++){
                    found2 = traverse(newSize, j);
                    if(found2 ==0){
                        //the necessary block esists
                        sf_block* alloBlock = traverseAndChange2Allo(i, newSize);
                        void* finalBlock = (void*)alloBlock + 16;
                        return finalBlock;
                        break;
                    }
                }
                count++;
            }
            //we have found our free block at loc
        }
    }

    //sf_show_heap();
    return NULL;
}

//WHEN FREE'ING --> MUST CHANGE PREV FLAG OF NEXT BLOCK
//--> check if prev and next are free --> if so, must colesce it all
//--> checking prev --> must account for prologue
//--> checking next --> must account for epilogue
void sf_free(void *pp) {
    //I first check if an invalid pointer has been given
    if(pp == NULL){
        abort();
    }
    //I typecast the pointer to a block
    sf_block* block = (sf_block*)(pp-16);
    int thisAlloc = (block->header)&THIS_BLOCK_ALLOCATED;
    int prevAlloc = (block->header)&PREV_BLOCK_ALLOCATED;
    int size = (block->header)&BLOCK_SIZE_MASK;
    //debug("VALUE OF SIZE %d", size);
    int footAlloc = ((block->prev_footer)^sf_magic())&THIS_BLOCK_ALLOCATED;

    if(thisAlloc!=2){
        //the block is free
        abort();
    }

    if(size<32){
        abort();
    }

    if(*(sf_footer*)((void*)block + size)!=((block->header)^sf_magic())){
        abort();
    }


    if((void*)block<(sf_mem_start()+32)){
        abort();
    }

    if((void*)block>(sf_mem_end()-8)){
        abort();
    }
    //if((sf_mem_start()+40)>)


    if((prevAlloc!=1)&&(footAlloc==2)){
        abort();
    }
    if((prevAlloc==1)&&(footAlloc!=2)){
        abort();
    }

    //I make this bit no longer allocated
    block->header = (block->header)^THIS_BLOCK_ALLOCATED;
    *(sf_footer*)((void*)block+size) = block->header ^ sf_magic();


    //I free the given memory block and store it in the free list
    int loc = findingPostion(size);
    add2Free(block, loc);

    //I change the next block to agree with this
    sf_block* nextBlock = (sf_block*)((void*)block+size);
    int sizeNext = (nextBlock->header)&BLOCK_SIZE_MASK;
    int nextAllo = (nextBlock->header)&THIS_BLOCK_ALLOCATED;


    nextBlock->header = sizeNext;
    //debug("SIZE NEXT: \t%d", sizeNext);
    if(nextAllo==2){
        nextBlock->header = nextBlock->header|THIS_BLOCK_ALLOCATED;
    }
    nextBlock->header = (nextBlock->header)&(~PREV_BLOCK_ALLOCATED);

    //need to change footer
    *(sf_footer*)((void*)nextBlock +sizeNext) = nextBlock->header ^ sf_magic();

    coalescing(block);

    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    //I first check if an invalid pointer has been given
    if(pp == NULL){
        abort();
    }
    //I typecast the pointer to a block
    sf_block* block = (sf_block*)(pp-16);
    int thisAlloc = (block->header)&THIS_BLOCK_ALLOCATED;
    int prevAlloc = (block->header)&PREV_BLOCK_ALLOCATED;
    int size = (block->header)&BLOCK_SIZE_MASK;

    int footAlloc = ((block->prev_footer)^sf_magic())&THIS_BLOCK_ALLOCATED;
    if(rsize==0){
        return NULL;
    }

    if(thisAlloc!=2){

        //the block is free
        abort();
    }

    if(size<32){
        abort();
    }

    if(*(sf_footer*)((void*)block + size)!=((block->header)^sf_magic())){
        abort();
    }

    if((void*)block<(sf_mem_start()+32)){
        abort();
    }

    if((void*)block>(sf_mem_end()-8)){
        abort();
    }


    if((prevAlloc!=1)&&(footAlloc==2)){
        abort();
    }
    if((prevAlloc==1)&&(footAlloc!=2)){
        abort();
    }
    int rsizeTemp = rsize;
    if(rsizeTemp<16){
        rsizeTemp = 32;
    }

    int tempFull = rsizeTemp = 16;
    int tempSize = ((tempFull-1)|15)+1;
    if((tempSize)==size){

        //the sizes are equal
        return pp;
    }

    //I need to make sure that rsize is a multiple of 16

    //I compare the current size with the new size
    if((rsize+16)>size){
        thisAlloc = (block->header)&THIS_BLOCK_ALLOCATED;
        debug("SIZE IS BIGGER");
        //current size is bigger
        sf_block* ptr2Block = sf_malloc(rsize); //malloc function will handle changing size
        if(ptr2Block==NULL){
            return NULL;
        }
        int fullSize = size-16;
        memcpy(ptr2Block, pp, fullSize);
        sf_free(pp);
        sf_show_heap();
        return ptr2Block;
    } else{
        //current size is smaller
        if(rsize<16){
            rsize = 16;
        }
        int fullSize = rsize + 16; //+ 16 for header and footer

        size_t newSize =0;
        newSize = ((fullSize-1)|15)+1;

        sf_block* newBlock = splitBlock2(block, newSize);
        //coalesce might be necessary
        int tempSize = newBlock->header&BLOCK_SIZE_MASK;
        if(((void*)newBlock+tempSize)!=(sf_mem_end()-16)){
            sf_block* nextBlock = (sf_block*) ((void*)newBlock+tempSize);
            coalescing(nextBlock);
        }
        void* finalBlock = ((void*)newBlock + 16);
        return finalBlock;
    }

    return NULL;
}









