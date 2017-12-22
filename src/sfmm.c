/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

void createHeader(sf_free_header *free1, int allocated, int padding_required, int block_size, sf_free_header *next, sf_free_header *prev){
    //CREATE AND SET THE HEADER TO ALLOCATED WITH ALL OTHER APPROPRIATE FIELDS.
    sf_header *header = ((sf_header*)free1);
    header->allocated = allocated;
    if(padding_required != 0){
        header->padded = 1;
    }
    else{
        header->padded = 0;
    }
    header->two_zeroes = 0x0;
    header->block_size = block_size >> 4;
    header->unused = 0x0;

    printf("Header address: %p\n", header);

    free1->header = *header;
    free1->next = next;
    free1->prev = prev;
}

void createFooter(sf_free_header *free1, int allocated,  int padding_required, int block_size, int requested_size){
    sf_header *header = ((sf_header*)free1);
    sf_footer *footer = (sf_footer*)((char *)header + block_size - 8);//STARTING ADDRESS OF FOOTER
    // CAST TO CHAR POINTER BECAUSE YOU WANT TO INCREMENT THE POINTER BYTE BY BYTE AND NOT BY THE SIZE OF FOOTER/FOOTER POINTER, WHICH IS 8 BYTES.

    footer->allocated = allocated;
    if(padding_required != 0){
        footer->padded = 1;
    }
    else{
        footer->padded = 0;
    }
    footer->two_zeroes = 0;
    footer->block_size= block_size >> 4;
    footer->requested_size = requested_size;

    printf("Footer address: %p\n", footer);
}

int ceiling(float x){
    int int_x = (int)x;
    if(x == (float) int_x){
        return int_x;
    }
    return int_x + 1;
}

/**
 * You should store the heads of your free lists in these variables.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */

//ARRAY OF FREELIST TYPEDEF STRUCTS
free_list seg_free_list[FREE_LIST_COUNT] = {
    {NULL, LIST_1_MIN, LIST_1_MAX},
    {NULL, LIST_2_MIN, LIST_2_MAX},
    {NULL, LIST_3_MIN, LIST_3_MAX},
    {NULL, LIST_4_MIN, LIST_4_MAX}
};


//PAGE COUNT
int page_count = 0;

size_t list_1_min = LIST_1_MIN;
size_t list_1_max = LIST_1_MAX;
size_t list_2_min = LIST_2_MIN;
size_t list_2_max = LIST_2_MAX;
size_t list_3_min = LIST_3_MIN;
size_t list_3_max = LIST_3_MAX;
size_t list_4_min = LIST_4_MIN;
size_t list_4_max = LIST_4_MAX;


void *sf_malloc(size_t size) {

    if(size <= 0 || size > 16384){
        sf_errno = EINVAL;
        return NULL;
    }

    int padding_required = 0;
    if((size % 16) != 0){
        padding_required = 16-(size % 16);
    }
    // int total_free_size = size + 32;
    int total_block_size = size + 16 + padding_required;
    // printf("Total Block Size: %d\n", total_block_size);

    if(total_block_size > 16384){
        sf_errno = ENOMEM;
        return NULL;
    }


    // //FIRST CALL TO MALLOC
    // if(seg_free_list[3].head == NULL){
    //     sf_free_header *free1;
    //     free1 = (sf_free_header *)sf_sbrk();
    //     page_count++;
    //     int remaining_size = PAGE_SZ - total_block_size;

    //     //ALLOCATED BLOCK
    //     createHeader(free1, 1, padding_required, total_block_size, NULL, NULL);
    //     createFooter(free1, 1, padding_required, total_block_size, size);

    //     sf_free_header *next_free1 = (sf_free_header*)((char *)free1 + total_block_size + 8);

    //     //REMAINING FREE BLOCK
    //     createHeader(next_free1, 0, 0, remaining_size, NULL, NULL);
    //     createFooter(next_free1, 0, 0, remaining_size, 0);

    //     sf_blockprint(next_free1);

    //     if(next_free1->header.block_size << 4 > LIST_4_MIN){
    //         seg_free_list[3].head = next_free1;
    //     }
    //     if(next_free1->header.block_size << 4 > LIST_3_MIN && next_free1->header.block_size << 4 < LIST_3_MAX){
    //         seg_free_list[2].head = next_free1;
    //     }
    //     if(next_free1->header.block_size << 4 > LIST_2_MIN && next_free1->header.block_size << 4 < LIST_2_MAX){
    //         seg_free_list[1].head = next_free1;
    //     }
    //     if(next_free1->header.block_size << 4 > LIST_1_MIN && next_free1->header.block_size << 4 < LIST_1_MAX){
    //         seg_free_list[0].head = next_free1;
    //     }
    //     sf_snapshot();

    //     return (sf_free_header*)((char*) free1 + 8);
    // }

	for(int i = 0; i < FREE_LIST_COUNT; i++){

        //IF THE SIZE FALLS BETWEEN THE MIN AND MAX SIZES OF THE CURRENT FREE LIST IN THE ARRAY, SEARCH THIS FREE LIST.

        //SEARCH UP TO 3RD ELEMENT IN THE ARRAY. 4TH HAS NO MAX, SO CANT COMPARE IF LESS THAN MAX (VALUE IS -1).
        if(total_block_size <= seg_free_list[i].max && total_block_size >= seg_free_list[i].min){
            //SEARCH THE ARRAY STARTING AT GIVEN ELEMENT INDEX. IF THE CURRENT FREE LIST IN SAID ELEMENT IS FILLED, i IS INCREMENTED TO SEARCH THE NEXT ELEMENT IN THE ARRAY.
            while(i < 4){


                sf_free_header *current_header = seg_free_list[i].head;


                while(current_header != NULL){
                    if((current_header -> header.block_size << 4)>= total_block_size){
                        /*
                        -ALLOC THE BLOCK
                        -SPLIT THE BLOCK FROM THE REST OF THE FREE BLOCK IF THE FREE BLOCK IS
                        GREATER THAN REQUESTED BLOCK SIZE
                        -CREATE HEADER FOR SPLIT FREE BLOCK

                        -IF THE SPLIT BLOCK DOES NOT FIT SIZE REQUIREMENT OF CURRENT FREE LIST,
                        ADD IT AS THE HEAD OF THE PROPER FREE LIST AND MAKE ITS NEXT POINTER POINT TO THE PREVIOUS HEAD.

                        -OTHERWISE, SET THE NEXT POINTER OF THE PREVIOUS BLOCK TO THAT NEW FREE BLOCK
                        -SET THE NEXT POINTER OF THE NEW FREE BLOCK TO THE NEXT BLOCK OF THE CURRENT BLOCK
                        -RETURN THE POINTER TO THE ALLOC BLOCK
                         */


                        //ALLOCATED BLOCK
                        int remaining_size = (current_header -> header.block_size << 4) - total_block_size;

                        if(remaining_size < 32){
                            //IF THE FREE LIST HEAD POINTS TO THIS BLOCK, IT SHOULDN'T ANYMORE BECAUSE IT IS GOING TO BE ALLOCATED AND SHOULD POINT TO THE ONE AFTER.
                            if(seg_free_list[i].head == current_header){
                                seg_free_list[i].head = current_header->next;
                                if(current_header->next != NULL){
                                    current_header->next->prev = NULL; //THIS IS BECAUSE THE NEXT FREE BLOCK'S (IF THERE IS ONE) PREV BLOCK JUST GOT ALLOCATED.
                                }
                            }
                            else{
                                if(current_header->prev != NULL){
                                    current_header->prev->next = current_header->next;
                                }
                                if(current_header->next != NULL){
                                    current_header->next->prev = current_header->prev;
                                }
                            }
                            //DO NOT SPLIT THE BLOCK, OTHERWISE THE REMAINING FREE BLOCK IS TOO SMALL TO ALLOCATE FOR ANYTHING. A SPLINTER WILL EXIST.
                            //ALLOCATE THE BLOCK
                            if(remaining_size != 0){
                                //OVERALLOCATE FOR SPLINTERS SPLINTERS NEED PADDING
                                createHeader(current_header, 1, 1, total_block_size + remaining_size, NULL, NULL);
                                createFooter(current_header, 1, 1, total_block_size + remaining_size, size);
                            }
                            else{
                                createHeader(current_header, 1, padding_required, total_block_size + remaining_size, NULL, NULL);
                                createFooter(current_header, 1, padding_required, total_block_size + remaining_size, size);
                            }

                            sf_blockprint(current_header);
                        }
                        //IF THE REMAINING FREE BLOCK IS GREATER THAN 32, SPLIT AND MAKE A FREE BLOCK AND PUT INTO HEAD OF APPROPRIATE FREE LIST.
                        //OTHERWISE, DONT SPLIT THE BLOCK.
                        if(remaining_size >= 32){
                            //IF THE FREE LIST HEAD POINTS TO THIS BLOCK, IT SHOULDN'T ANYMORE BECAUSE IT IS GOING TO BE ALLOCATED AND SHOULD POINT TO THE ONE AFTER.
                            if(seg_free_list[i].head == current_header){
                                seg_free_list[i].head = current_header->next;
                                if(current_header->next != NULL){
                                    current_header->next->prev = NULL; //THIS IS BECAUSE THE FREE BLOCK'S PREV BLOCK JUST GOT ALLOCATED.
                                }
                            }
                            else{
                                if(current_header->prev != NULL){
                                    current_header->prev->next = current_header->next;
                                }
                                if(current_header->next != NULL){
                                    current_header->next->prev = current_header->prev;
                                }
                            }
                            //ALLOCATE THE BLOCK
                            createHeader(current_header, 1, padding_required, total_block_size, NULL, NULL);
                            createFooter(current_header, 1, padding_required, total_block_size, size);

                            sf_blockprint(current_header);

                            sf_free_header *next_free1 = (sf_free_header*)((char *)current_header + total_block_size);

                            //REMAINING FREE BLOCK

                            if(remaining_size >= list_4_min && remaining_size <= list_4_max){
                                createHeader(next_free1, 0, 0, remaining_size, seg_free_list[3].head, NULL); //REMAINDER FREE BLOCK GETS PUSHED TO THE FRONT AS NEW HEAD. NEXT POINTS TO THE PREVIOUS HEAD.
                                createFooter(next_free1, 0, 0, remaining_size, 0);

                                //THIS ASSUMES THAT THE HEAD POINTS TO A FREE BLOCK, AS IT SHOULDDD!!!!
                                if(seg_free_list[3].head != NULL){
                                    next_free1->next = seg_free_list[3].head;
                                    seg_free_list[3].head->prev = next_free1;
                                }
                                seg_free_list[3].head = next_free1;

                                sf_blockprint(next_free1);
                            }
                            if(remaining_size >= list_3_min && remaining_size <= list_3_max){
                                createHeader(next_free1, 0, 0, remaining_size, seg_free_list[2].head, NULL); //REMAINDER FREE BLOCK GETS PUSHED TO THE FRONT AS NEW HEAD. NEXT POINTS TO THE PREVIOUS HEAD.
                                createFooter(next_free1, 0, 0, remaining_size, 0);

                                if(seg_free_list[2].head != NULL){
                                    next_free1->next = seg_free_list[2].head;
                                    seg_free_list[2].head->prev = next_free1;
                                }
                                seg_free_list[2].head = next_free1;

                                sf_blockprint(next_free1);
                            }
                            if(remaining_size >= list_2_min && remaining_size <= list_2_max){
                                createHeader(next_free1, 0, 0, remaining_size, seg_free_list[1].head, NULL); //REMAINDER FREE BLOCK GETS PUSHED TO THE FRONT AS NEW HEAD. NEXT POINTS TO THE PREVIOUS HEAD.
                                createFooter(next_free1, 0, 0, remaining_size, 0);

                                if(seg_free_list[1].head != NULL){
                                    next_free1->next = seg_free_list[1].head;
                                    seg_free_list[1].head->prev = next_free1;
                                }
                                seg_free_list[1].head = next_free1;

                                sf_blockprint(next_free1);
                            }
                            if(remaining_size >= list_1_min && remaining_size <= list_1_max){
                                createHeader(next_free1, 0, 0, remaining_size, seg_free_list[0].head, NULL); //REMAINDER FREE BLOCK GETS PUSHED TO THE FRONT AS NEW HEAD. NEXT POINTS TO THE PREVIOUS HEAD.
                                createFooter(next_free1, 0, 0, remaining_size, 0);

                                if(seg_free_list[0].head != NULL){
                                    next_free1->next = seg_free_list[0].head;
                                    seg_free_list[0].head->prev = next_free1;
                                }
                                seg_free_list[0].head = next_free1;

                                sf_blockprint(next_free1);
                            }
                        }
                        sf_snapshot();

                        // current_header -> prev -> next = current_header -> next;
                        // current_header -> next -> prev = current_header -> prev;
                        return (void *)(sf_free_header *)((char*)current_header + 8);
                    }
                    else{
                        //SEARCH FOR THE NEXT FREE BLOCK IN THE LIST
                        current_header = current_header -> next;
                    }
                }


                //IF THE LAST FREE LIST IN THE ARRAY IS SEARCHED AND THERE IS NO FREE BLOCK, THEN THERE IS NO MEMORY AVAILABLE AND REQUEST MORE MEMORY FROM THE KERNEL ON THE HEAP.
                if(i == FREE_LIST_COUNT - 1){
                    //IF sfbrk HAS BEEN CALLED 4 TIMES (MAX 4 PAGES TO ALLOCATE ON THE HEAP), THEN THERES NOT ENOUGH MEMORY TO ALLOCATE.
                    if(page_count == 4){
                        sf_errno = ENOMEM;
                        return NULL;
                    }

                    //REQUEST MORE MEMORY USING sbrk();
                    int number_to_sbrk = ceiling(((float)total_block_size) / ((float)PAGE_SZ));
                    int j = 0;
                    sf_free_header *free1;
                    // free1 = (sf_free_header *)sf_sbrk(); //GET ONE GIANT FREE BLOCK ON THE HEAP FROM sbrk().
                    // page_count++;

                    // // //CREATE FREE BLOCK FROM THIS
                    // printf("Free block from sbrk:");
                    // createHeader(free1, 0, 0, PAGE_SZ, NULL, NULL);
                    // createFooter(free1, 0, 0, PAGE_SZ, 0);

                    // sf_free_header *free1_original = free1;

                    // //IF THERE EXISTS A FREE BLOCK PREVIOUS TO THIS 4096 BYTE BLOCK OBTAINED, COALESCE THESE BLOCKS.
                    // if(page_count > 1){
                    //     sf_footer *prevFooter = (sf_footer *)((char*)free1-8);
                    //     if(prevFooter->allocated == 0){
                    //         //COALESCE IF THE PREVIOUS BLOCK IS FREE
                    //         int previousBlockSize = prevFooter->block_size << 4;
                    //         sf_header *prevHeader = (sf_header *)((char*)free1 - previousBlockSize);
                    //         prevHeader->block_size = (previousBlockSize + PAGE_SZ) >> 4;
                    //         free1 = (sf_free_header *)prevHeader;
                    //         free1->header = *prevHeader;

                    //         //CREATE THE NEW FOOTER FOR THE NEW sbrk() BLOCK
                    //         createFooter(free1, 0, 0, prevHeader->block_size, 0);
                    //     }
                    // }


                    //IF WE NEED TO sbrk MORE THAN ONCE BECAUSE REQUESTED SIZE IS LARGER THAN A PAGE AND FREE BLOCK IS STILL SMALLER AFTER COALESCING
                    do{
                        free1 = (sf_free_header *)sf_sbrk(); //GET ONE GIANT FREE BLOCK ON THE HEAP FROM sbrk().
                        page_count++;

                        //CREATE FREE BLOCK FROM THIS
                        printf("Free block from sbrk:");
                        createHeader(free1, 0, 0, PAGE_SZ, NULL, NULL);
                        createFooter(free1, 0, 0, PAGE_SZ, 0);

                        //ON THE FIRST ITERATION OF THE LOOP, TAKE OUT THE BLOCK FROM WHERE ITS POINTED AT, THEN COALESCE
                        if(j == 0){
                            //ONLY AFTER FIRST sbrk BECAUSE IF YOU TRY TO REMOVE DURING FIRST sbrk, THERE IS NO PREVIOUS BLOCK TO REMOVE....
                            if(page_count > 1){
                                sf_footer *prevFooter = (sf_footer *)((char*)free1-8);
                                int previousBlockSize = prevFooter->block_size << 4;
                                sf_header *prevHeader = (sf_header *)((char*)free1 - previousBlockSize);
                                sf_free_header *prevFreeHeader = (sf_free_header *)prevHeader;

                                if(previousBlockSize >= list_4_min && previousBlockSize <= list_4_max){
                                    if(seg_free_list[3].head == prevFreeHeader){
                                        seg_free_list[3].head = prevFreeHeader->next;
                                    }
                                    else{
                                        prevFreeHeader->prev->next = prevFreeHeader->next;
                                    }
                                }
                                if(previousBlockSize >= list_3_min && previousBlockSize <= list_3_max){
                                    if(seg_free_list[2].head == prevFreeHeader){
                                        seg_free_list[2].head = prevFreeHeader->next;
                                    }
                                    else{
                                        prevFreeHeader->prev->next = prevFreeHeader->next;
                                    }
                                }
                                if(previousBlockSize >= list_2_min && previousBlockSize <= list_2_max){
                                    if(seg_free_list[1].head == prevFreeHeader){
                                        seg_free_list[1].head = prevFreeHeader->next;
                                    }
                                    else{
                                        prevFreeHeader->prev->next = prevFreeHeader->next;
                                    }
                                }
                                if(previousBlockSize >= list_1_min && previousBlockSize <= list_1_max){
                                    if(seg_free_list[0].head == prevFreeHeader){
                                        seg_free_list[0].head = prevFreeHeader->next;
                                    }
                                    else{
                                        prevFreeHeader->prev->next = prevFreeHeader->next;
                                    }
                                }
                            }

                        }

                        //IF THERE EXISTS A FREE BLOCK PREVIOUS TO THIS 4096 BYTE BLOCK OBTAINED, COALESCE THESE BLOCKS.
                        if(page_count > 1){
                            sf_footer *prevFooter = (sf_footer *)((char*)free1-8);
                            if(prevFooter->allocated == 0){
                                //COALESCE
                                int previousBlockSize = prevFooter->block_size << 4;
                                sf_header *prevHeader = (sf_header *)((char*)free1 - previousBlockSize);
                                prevHeader->block_size = (previousBlockSize + PAGE_SZ) >> 4;
                                free1 = (sf_free_header *)prevHeader;
                                free1->header = *prevHeader;

                                //CREATE THE NEW FOOTER FOR THE NEW sbrk() BLOCK
                                createFooter(free1, 0, 0, prevHeader->block_size << 4, 0);
                            }
                        }
                        j++;
                    }while (j < number_to_sbrk && (free1->header.block_size << 4) < total_block_size);

                    //AFTER COALESCING, PUT THE BIG BLOCK INTO THE APPROPRIATE ARRAY INDEX.
                    if(free1->header.block_size << 4 >= list_4_min && free1->header.block_size << 4 <= list_4_max){

                        if(seg_free_list[3].head != NULL){
                            free1->next = seg_free_list[3].head;
                            seg_free_list[3].head->prev = free1;
                        }

                        seg_free_list[3].head = free1;
                    }
                    if(free1->header.block_size << 4 >= list_3_min && free1->header.block_size << 4 <= list_3_max){

                        if(seg_free_list[2].head != NULL){
                            free1->next = seg_free_list[2].head;
                            seg_free_list[2].head->prev = free1;
                        }
                        seg_free_list[2].head = free1;
                    }
                    if(free1->header.block_size << 4 >= list_2_min && free1->header.block_size << 4 <= list_2_max){

                        if(seg_free_list[1].head != NULL){
                            free1->next = seg_free_list[1].head;
                            seg_free_list[1].head->prev = free1;
                        }
                        seg_free_list[1].head = free1;
                    }
                    if(free1->header.block_size << 4 >= list_1_min && free1->header.block_size << 4 <= list_1_max){

                        if(seg_free_list[0].head != NULL){
                            free1->next = seg_free_list[0].head;
                            seg_free_list[0].head->prev = free1;
                        }
                        seg_free_list[0].head = free1;
                    }


                    // free1 = free1_original;

                    int remaining_size = (free1->header.block_size << 4) - total_block_size;

                    //IF THE FREE LIST HEAD POINTS TO THIS BLOCK, IT SHOULDN'T ANYMORE BECAUSE IT IS GOING TO BE ALLOCATED AND SHOULD POINT TO THE ONE AFTER.
                    if(seg_free_list[3].head == free1){
                        seg_free_list[3].head = free1->next;
                    }
                    if(seg_free_list[2].head == free1){
                        seg_free_list[2].head = free1->next;
                    }
                    if(seg_free_list[1].head == free1){
                        seg_free_list[1].head = free1->next;
                    }
                    if(seg_free_list[0].head == free1){
                        seg_free_list[0].head = free1->next;
                    }

                    if(remaining_size < 32){
                        //ALLOCATED BLOCK
                        createHeader(free1, 1, padding_required, total_block_size + remaining_size, NULL, NULL);
                        createFooter(free1, 1, padding_required, total_block_size + remaining_size, size);

                        printf("Allocated block: %p\n", free1);
                        sf_blockprint(free1);
                    }

                    if(remaining_size >= 32){
                        //ALLOCATED BLOCK
                        createHeader(free1, 1, padding_required, total_block_size, NULL, NULL);
                        createFooter(free1, 1, padding_required, total_block_size, size);

                        printf("Allocated block: %p\n", free1);
                        sf_blockprint(free1);

                        sf_free_header *next_free1 = (sf_free_header*)((char *)free1 + total_block_size);


                        //REMAINING FREE BLOCK
                        createHeader(next_free1, 0, 0, remaining_size, NULL, NULL);
                        createFooter(next_free1, 0, 0, remaining_size, 0);

                        printf("Remaining free block: %p\n", next_free1);
                        sf_blockprint(next_free1);

                        if(next_free1->header.block_size << 4 >= list_4_min && next_free1->header.block_size << 4 <= list_4_max){

                            //THIS ASSUMES THAT THE HEAD POINTS TO A FREE BLOCK, AS IT SHOULDDD!!!!
                            if(seg_free_list[3].head != NULL){
                                next_free1->next = seg_free_list[3].head;
                                seg_free_list[3].head->prev = next_free1;
                            }

                            seg_free_list[3].head = next_free1;
                        }
                        if(next_free1->header.block_size << 4 >= list_3_min && next_free1->header.block_size << 4 <= list_3_max){

                            if(seg_free_list[2].head != NULL){
                                next_free1->next = seg_free_list[2].head;
                                seg_free_list[2].head->prev = next_free1;
                            }
                            seg_free_list[2].head = next_free1;
                        }
                        if(next_free1->header.block_size << 4 >= list_2_min && next_free1->header.block_size << 4 <= list_2_max){

                            if(seg_free_list[1].head != NULL){
                                next_free1->next = seg_free_list[1].head;
                                seg_free_list[1].head->prev = next_free1;
                            }
                            seg_free_list[1].head = next_free1;
                        }
                        if(next_free1->header.block_size << 4 >= list_1_min && next_free1->header.block_size << 4 <= list_1_max){

                            if(seg_free_list[0].head != NULL){
                                next_free1->next = seg_free_list[0].head;
                                seg_free_list[0].head->prev = next_free1;
                            }
                            seg_free_list[0].head = next_free1;
                        }
                        sf_snapshot();
                    }

                    return (void *)(sf_free_header*)((char*) free1 + 8);
                }

                //IF THE CURRENT FREE LIST IN THE ARRAY DOES NOT HAVE A FREE BLOCK OF ENOUGH SIZE, SEARCH THE NEXT FREE LIST IN THE ARRAY.
                i++;

            }
        }

    }
    return NULL;
}

void *sf_realloc(void *ptr, size_t size) {
    //IF THE POINTER IS NULL, ABORT
    if(ptr == NULL){
        abort();
    }

    sf_header *headToResize = (sf_header *)((char *)ptr - 8);
    sf_footer *currentFooter = (sf_footer *)((char *)headToResize + (headToResize->block_size << 4) - 8);

    int padding_required = 0;
    if((size % 16) != 0){
        padding_required = 16-(size % 16);
    }
    int new_total_block_size = size + 16 + padding_required;

    //IF THE ALLOC BIT IN THE HEADER OR FOOTER IS 0, ABORT
    if(headToResize->allocated == 0 || currentFooter->allocated == 0){
        abort();
    }

    //IF THE HEADER OF THE BLOCK IS BEFORE heap_start OR BLOCK ENDS AFTER heap_end, ABORT
    void *heapStartAddress = get_heap_start();
    void *heapEndAddress = get_heap_end();

    if((void *)headToResize < heapStartAddress){
        abort();
    }

    if((void *)currentFooter > heapEndAddress){
        abort();
    }

    //IF THE SIZE CONTAINS PADDING BUT PADDED BIT IS NOT SET, ABORT
    //THIS IS BECAUSE requested_size IS THE SIZE WITHOUT PADDING AND ADDING 16 IS ONLY HEADER AND FOOTER.
    if((currentFooter->requested_size + 16 != headToResize->block_size << 4) && currentFooter->padded == 0){
        abort();
    }

    //IF THE PADDED AND ALLOC BITS IN THE HEADER AND FOOTER ARE INCONSISTENT, ABORT
    if((headToResize->padded == 1 && currentFooter->padded == 0) ||
        (headToResize->padded == 0 && currentFooter->padded == 1)){
        abort();
    }
    if((headToResize->allocated == 1 && currentFooter->allocated == 0) ||
        (headToResize->allocated == 0 && currentFooter->allocated == 1)){
        abort();
    }

    //IF THE SIZE TO REALLOC TO IS 0, FREE THE BLOCK AND RETURN NULL
    if(size == 0){
        sf_free(ptr);
        return NULL;
    }

    if(headToResize->block_size << 4 < new_total_block_size){
        //REALLOC TO A LARGER BLOCK OF MEMORY
        void *realloc_ptr = sf_malloc(size);

        if(realloc_ptr != NULL){
            realloc_ptr = memcpy(realloc_ptr, ptr, headToResize->block_size);
            sf_free(ptr);
        }

        return realloc_ptr;
    }
    else{
        //REALLOC TO A SMALLER BLOCK OF MEMORY
        int currentBlockSize = headToResize->block_size << 4;
        int remaining_size = currentBlockSize - new_total_block_size;
        if(remaining_size < 32){
            //IT WILL CREATE A SPLINTER IF WE SPLIT. SO DO NOT SPLIT. SIMPLY RESIZE THE PAYLOAD, NOT THE BLOCK. USE THE REST OF THE BLOCK AS PADDING. BLOCK SIZE SAME.
            sf_free_header *headToResize_F = (sf_free_header *)headToResize;
            if(remaining_size != 0){
                //OVERALLOCATE FOR SPLINTERS SPLINTERS NEED PADDING
                createHeader(headToResize_F, 1, 1, currentBlockSize, NULL, NULL);
                createFooter(headToResize_F, 1, 1, currentBlockSize, size);
            }
            else{
                createHeader(headToResize_F, 1, padding_required, currentBlockSize, NULL, NULL);
                createFooter(headToResize_F, 1, padding_required, currentBlockSize, size);
            }

            sf_blockprint(headToResize_F);

            return ptr;
        }
        if(remaining_size >= 32){
            //SPLITTING THE BLOCK CAN CREATE AN ALLOCATABLE FREE BLOCK. SPLIT AND MOVE FREE BLOCK TO APPROPRIATE FREE LIST
            //REALLOCATE THE BLOCK TO SMALLER SIZE
            sf_free_header *headToResize_F = (sf_free_header *)headToResize;
            createHeader(headToResize_F, 1, padding_required, new_total_block_size, NULL, NULL);
            createFooter(headToResize_F, 1, padding_required, new_total_block_size, size);

            sf_blockprint(headToResize_F);

            //CREATE THE FREE BLOCK
            sf_free_header *new_free_block = (sf_free_header *)((char *)headToResize + new_total_block_size);
            createHeader(new_free_block, 0, 0, remaining_size, NULL, NULL);
            createFooter(new_free_block, 0, 0, remaining_size, 0);

            sf_blockprint(new_free_block);

            //COALESCE IF THE NEXT BLOCK IN MEMORY IS FREE
            sf_free_header *next_block = (sf_free_header *)((char *)new_free_block + remaining_size);
            if(next_block->header.allocated == 0){
                int combined_block_size = remaining_size + (next_block->header.block_size << 4);
                createHeader(new_free_block, 0, 0, combined_block_size, NULL, NULL);
                createFooter(new_free_block, 0, 0, combined_block_size, 0);

                sf_blockprint(new_free_block);

                int nextHeader_block_size = next_block->header.block_size << 4;


                if(nextHeader_block_size >= list_4_min && nextHeader_block_size <= list_4_max){
                    //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
                    if(seg_free_list[3].head == next_block){
                        seg_free_list[3].head = next_block->next;
                    }
                    //OTHERWISE, THE COALESCE BLOCK IS SOMEHWERE IN THE FREELIST. SEVER CONNECTIONS AS NEEDED.
                    else{
                        if(next_block->prev != NULL){
                            next_block->prev->next = next_block->next;
                        }
                        if(next_block->next != NULL){
                            next_block->next->prev = next_block->prev;
                        }
                    }
                }
                if(nextHeader_block_size >= list_3_min && nextHeader_block_size <= list_3_max){
                    //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
                    if(seg_free_list[2].head == next_block){
                        seg_free_list[2].head = next_block->next;
                    }
                    //OTHERWISE, THE COALESCE BLOCK IS SOMEHWERE IN THE FREELIST. SEVER CONNECTIONS AS NEEDED.
                    else{
                        if(next_block->prev != NULL){
                            next_block->prev->next = next_block->next;
                        }
                        if(next_block->next != NULL){
                            next_block->next->prev = next_block->prev;
                        }
                    }
                }
                if(nextHeader_block_size >= list_2_min && nextHeader_block_size <= list_2_max){
                    //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
                    if(seg_free_list[1].head == next_block){
                        seg_free_list[1].head = next_block->next;
                    }
                    //OTHERWISE, THE COALESCE BLOCK IS SOMEHWERE IN THE FREELIST. SEVER CONNECTIONS AS NEEDED.
                    else{
                        if(next_block->prev != NULL){
                            next_block->prev->next = next_block->next;
                        }
                        if(next_block->next != NULL){
                            next_block->next->prev = next_block->prev;
                        }
                    }
                }
                if(nextHeader_block_size >= list_1_min && nextHeader_block_size <= list_1_max){
                    //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
                    if(seg_free_list[0].head == next_block){
                        seg_free_list[0].head = next_block->next;
                    }
                    //OTHERWISE, THE COALESCE BLOCK IS SOMEHWERE IN THE FREELIST. SEVER CONNECTIONS AS NEEDED.
                    else{
                        if(next_block->prev != NULL){
                            next_block->prev->next = next_block->next;
                        }
                        if(next_block->next != NULL){
                            next_block->next->prev = next_block->prev;
                        }
                    }
                }
            }

            //PLACE INTO PROPER LIST
            int new_block_size = new_free_block->header.block_size << 4;
            if(new_block_size >= list_4_min && new_block_size <= list_4_max){
                if(seg_free_list[3].head != NULL){
                    new_free_block->next = seg_free_list[3].head;
                    seg_free_list[3].head->prev = new_free_block;
                }
                seg_free_list[3].head = new_free_block;
            }
            if(new_block_size >= list_3_min && new_block_size <= list_3_max){
                if(seg_free_list[2].head != NULL){
                    new_free_block->next = seg_free_list[2].head;
                    seg_free_list[2].head->prev = new_free_block;
                }
                seg_free_list[2].head = new_free_block;
            }
            if(new_block_size >= list_2_min && new_block_size <= list_2_max){
                if(seg_free_list[1].head != NULL){
                    new_free_block->next = seg_free_list[1].head;
                    seg_free_list[1].head->prev = new_free_block;
                }
                seg_free_list[1].head = new_free_block;
            }
            if(new_block_size >= list_1_min && new_block_size <= list_1_max){
                if(seg_free_list[0].head != NULL){
                    new_free_block->next = seg_free_list[0].head;
                    seg_free_list[0].head->prev = new_free_block;
                }
                seg_free_list[0].head = new_free_block;
            }

            sf_snapshot();

            return ptr;
        }

    }
	return NULL;
}

void sf_free(void *ptr) {
    //IF POINTER IS NULL, ABORT
    if(ptr == NULL){
        abort();
    }

    sf_header *toFree1 = (sf_header *)((char *)ptr - 8);
    sf_footer *toFree1Footer = (sf_footer *)((char *)toFree1 + (toFree1->block_size << 4) - 8);

    //IF ALLOC IN HEADER OR FOOTER IS 0, ABORT
    if(toFree1->allocated == 0 || toFree1Footer->allocated == 0){
        abort();
    }

    //IF THE HEADER OF THE BLOCK IS BEFORE heap_start OR BLOCK ENDS AFTER heap_end, ABORT
    void *heapStartAddress = get_heap_start();
    void *heapEndAddress = get_heap_end();

    if((void *)toFree1 < heapStartAddress){
        abort();
    }

    if((void *)toFree1Footer > heapEndAddress){
        abort();
    }

    //IF THE PADDED AND ALLOC BITS IN THE HEADER AND FOOTER ARE INCONSISTENT, ABORT
    if((toFree1->padded == 1 && toFree1Footer->padded == 0) ||
        (toFree1->padded == 0 && toFree1Footer->padded == 1)){
        abort();
    }
    if((toFree1->allocated == 1 && toFree1Footer->allocated == 0) ||
        (toFree1->allocated == 0 && toFree1Footer->allocated == 1)){
        abort();
    }

    sf_free_header *free1 = (sf_free_header *)toFree1;
    int toFree_block_size = toFree1->block_size << 4;

    //IF THE SIZE CONTAINS PADDING BUT PADDED BIT IS NOT SET, ABORT
    //THIS IS BECAUSE requested_size IS THE SIZE WITHOUT PADDING AND ADDING 16 IS ONLY HEADER AND FOOTER.
    if(toFree1Footer->requested_size + 16 != toFree_block_size && toFree1Footer->padded == 0){
        abort();
    }

    //IF THE NEXT CONTIGUOUS BLOCK IN MEMORY IS ALSO FREE, COALESCE
    sf_header *nextHeader = (sf_header *)((char *)toFree1 + toFree_block_size);
    if(nextHeader->allocated == 0){
        //COALESCE
        int nextHeader_block_size = nextHeader->block_size << 4;

        int combined_block_size = toFree_block_size + nextHeader_block_size;
        sf_free_header *nextFreeHeader = (sf_free_header *)nextHeader;
        // sf_free_header *nextFreesNext = nextFreeHeader->next;


        if(nextHeader_block_size >= list_4_min && nextHeader_block_size <= list_4_max){
            //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
            if(seg_free_list[3].head == nextFreeHeader){
                seg_free_list[3].head = nextFreeHeader->next;
            }
            //OTHERWISE IT IS LINKED TO ANOTHER BLOCK IN THE LIST AND THE CONNECTIONS SHOULD BE SEVERED
            else{
                if(nextFreeHeader->prev != NULL){
                    nextFreeHeader->prev->next = nextFreeHeader->next;
                }
                if(nextFreeHeader->next != NULL){
                    nextFreeHeader->next->prev = nextFreeHeader->prev;
                }
            }
        }
        if(nextHeader_block_size >= list_3_min && nextHeader_block_size <= list_3_max){
            //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
            if(seg_free_list[2].head == nextFreeHeader){
                seg_free_list[2].head = nextFreeHeader->next;
            }
            //OTHERWISE IT IS LINKED TO ANOTHER BLOCK IN THE LIST AND THE CONNECTIONS SHOULD BE SEVERED
            else{
                if(nextFreeHeader->prev != NULL){
                    nextFreeHeader->prev->next = nextFreeHeader->next;
                }
                if(nextFreeHeader->next != NULL){
                    nextFreeHeader->next->prev = nextFreeHeader->prev;
                }
            }
        }
        if(nextHeader_block_size >= list_2_min && nextHeader_block_size <= list_2_max){
            //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
            if(seg_free_list[1].head == nextFreeHeader){
                seg_free_list[1].head = nextFreeHeader->next;
            }
            //OTHERWISE IT IS LINKED TO ANOTHER BLOCK IN THE LIST AND THE CONNECTIONS SHOULD BE SEVERED
            else{
                if(nextFreeHeader->prev != NULL){
                    nextFreeHeader->prev->next = nextFreeHeader->next;
                }
                if(nextFreeHeader->next != NULL){
                    nextFreeHeader->next->prev = nextFreeHeader->prev;
                }
            }
        }
        if(nextHeader_block_size >= list_1_min && nextHeader_block_size <= list_1_max){
            //IF THE NEXT HEADER TO COALESCE WITH IS POINTED TO BY ARRAY, REMOVE THAT POINTER.
            if(seg_free_list[0].head == nextFreeHeader){
                seg_free_list[0].head = nextFreeHeader->next;
            }
            //OTHERWISE IT IS LINKED TO ANOTHER BLOCK IN THE LIST AND THE CONNECTIONS SHOULD BE SEVERED
            else{
                if(nextFreeHeader->prev != NULL){
                    nextFreeHeader->prev->next = nextFreeHeader->next;
                }
                if(nextFreeHeader->next != NULL){
                    nextFreeHeader->next->prev = nextFreeHeader->prev;
                }
            }
        }


        createHeader(free1, 0, 0, combined_block_size, NULL, NULL);
        createFooter(free1, 0, 0, combined_block_size, 0);

        sf_blockprint(free1);

        if(combined_block_size >= list_4_min && combined_block_size <= list_4_max){
            if(seg_free_list[3].head != NULL){
                free1->next = seg_free_list[3].head;
                seg_free_list[3].head->prev = free1;
            }
            seg_free_list[3].head = free1;
        }
        if(combined_block_size >= list_3_min && combined_block_size <= list_3_max){
            if(seg_free_list[2].head != NULL){
                free1->next = seg_free_list[2].head;
                seg_free_list[2].head->prev = free1;
            }
            seg_free_list[2].head = free1;
        }
        if(combined_block_size >= list_2_min && combined_block_size <= list_2_max){
            if(seg_free_list[1].head != NULL){
                free1->next = seg_free_list[1].head;
                seg_free_list[1].head->prev = free1;
            }
            seg_free_list[1].head = free1;
        }
        if(combined_block_size >= list_1_min && combined_block_size <= list_1_max){
            if(seg_free_list[0].head != NULL){
                free1->next = seg_free_list[0].head;
                seg_free_list[0].head->prev = free1;
            }
            seg_free_list[0].head = free1;
        }

    }
    else{
        //JUST FREE BY SETTING ALLOCATED TO 0 AND PLACE IT INTO THE HEAD OF THE PROPER LIST
        createHeader(free1, 0, 0, toFree_block_size, NULL, NULL);
        createFooter(free1, 0, 0, toFree_block_size, 0);

        sf_blockprint(free1);

        if(free1->header.block_size << 4 >= list_4_min && free1->header.block_size << 4 <= list_4_max){
            if(seg_free_list[3].head != NULL){
                free1->next = seg_free_list[3].head;
                seg_free_list[3].head->prev = free1;
            }
            seg_free_list[3].head = free1;
        }
        if(free1->header.block_size << 4  >= list_3_min && free1->header.block_size << 4  <= list_3_max){
            if(seg_free_list[2].head != NULL){
                free1->next = seg_free_list[2].head;
                seg_free_list[2].head->prev = free1;
            }
            seg_free_list[2].head = free1;
        }
        if(free1->header.block_size << 4  >= list_2_min && free1->header.block_size << 4  <= list_2_max){
            if(seg_free_list[1].head != NULL){
                free1->next = seg_free_list[1].head;
                seg_free_list[1].head->prev = free1;
            }
            seg_free_list[1].head = free1;
        }
        if(free1->header.block_size << 4  >= list_1_min && free1->header.block_size << 4  <= list_1_max){
            if(seg_free_list[0].head != NULL){
                free1->next = seg_free_list[0].head;
                seg_free_list[0].head->prev = free1;
            }
            seg_free_list[0].head = free1;
        }

    }
    sf_snapshot();

}
