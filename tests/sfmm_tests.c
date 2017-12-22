#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "sfmm.h"


int find_list_index_from_size(int sz) {

    size_t list_1_min = LIST_1_MIN;
    size_t list_1_max = LIST_1_MAX;
    size_t list_2_min = LIST_2_MIN;
    size_t list_2_max = LIST_2_MAX;
    size_t list_3_min = LIST_3_MIN;
    size_t list_3_max = LIST_3_MAX;
    size_t list_4_min = LIST_4_MIN;
    size_t list_4_max = LIST_4_MAX;
	if (sz >= list_1_min && sz <= list_1_max) return 0;
	else if (sz >= list_2_min && sz <= list_2_max) return 1;
	else if (sz >= list_3_min && sz <= list_3_max) return 2;
	else if (sz >= list_4_min && sz <= list_4_max) return 3;
    return -1;
}

Test(sf_memsuite_student, Malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x);

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	sf_header *header = (sf_header*)((char*)x - 8);

	/* There should be one block of size 4064 in list 3 */
	free_list *fl = &seg_free_list[find_list_index_from_size(PAGE_SZ - (header->block_size << 4))];

	cr_assert_not_null(fl, "Free list is null");

	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert_null(fl->head->next, "Found more blocks than expected!");
	cr_assert(fl->head->header.block_size << 4 == 4064);
	cr_assert(fl->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(get_heap_start() + PAGE_SZ == get_heap_end(), "Allocated more than necessary!");
}

Test(sf_memsuite_student, Malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 2);

	cr_assert_null(x, "x is not NULL!");
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sf_memsuite_student, free_double_free, .init = sf_mem_init, .fini = sf_mem_fini, .signal = SIGABRT) {
	sf_errno = 0;
	void *x = sf_malloc(sizeof(int));
	sf_free(x);
	sf_free(x);
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(sizeof(long));
	void *y = sf_malloc(sizeof(double) * 10);
	/* void *z = */ sf_malloc(sizeof(char));

	sf_free(y);

	free_list *fl = &seg_free_list[find_list_index_from_size(96)];

	cr_assert_not_null(fl->head, "No block in expected free list");
	cr_assert_null(fl->head->next, "Found more blocks than expected!");
	cr_assert(fl->head->header.block_size << 4 == 96);
	cr_assert(fl->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(sizeof(long));
	void *x = sf_malloc(sizeof(double) * 11);
	void *y = sf_malloc(sizeof(char));
	/* void *z = */ sf_malloc(sizeof(int));

	sf_free(y);
	sf_free(x);

	free_list *fl_y = &seg_free_list[find_list_index_from_size(32)];
	free_list *fl_x = &seg_free_list[find_list_index_from_size(144)];

	cr_assert_null(fl_y->head, "Unexpected block in list!");
	cr_assert_not_null(fl_x->head, "No block in expected free list");
	cr_assert_null(fl_x->head->next, "Found more blocks than expected!");
	cr_assert(fl_x->head->header.block_size << 4 == 144);
	cr_assert(fl_x->head->header.allocated == 0);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	/* void *u = */ sf_malloc(1);          //32
	void *v = sf_malloc(LIST_1_MIN); //48
	void *w = sf_malloc(LIST_2_MIN); //160
	void *x = sf_malloc(LIST_3_MIN); //544
	void *y = sf_malloc(LIST_4_MIN); //2080
	/* void *z = */ sf_malloc(1); // 32

	int allocated_block_size[4] = {48, 160, 544, 2080};

	sf_free(v);
	sf_free(w);
	sf_free(x);
	sf_free(y);

	// First block in each list should be the most recently freed block
	for (int i = 0; i < FREE_LIST_COUNT; i++) {
		sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
		cr_assert_not_null(fh, "list %d is NULL!", i);
		cr_assert(fh->header.block_size << 4 == allocated_block_size[i], "Unexpected free block size!");
		cr_assert(fh->header.allocated == 0, "Allocated bit is set!");
	}

	// There should be one free block in each list, 2 blocks in list 3 of size 544 and 1232
	for (int i = 0; i < FREE_LIST_COUNT; i++) {
		sf_free_header *fh = (sf_free_header *)(seg_free_list[i].head);
		if (i != 2)
		    cr_assert_null(fh->next, "More than 1 block in freelist [%d]!", i);
		else {
		    cr_assert_not_null(fh->next, "Less than 2 blocks in freelist [%d]!", i);
		    cr_assert_null(fh->next->next, "More than 2 blocks in freelist [%d]!", i);
		}
	}
}

Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 10);

	free_list *fl = &seg_free_list[find_list_index_from_size(32)];

	cr_assert_not_null(x, "x is NULL!");
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.block_size << 4 == 32, "Free Block size not what was expected!");

	sf_header *header = (sf_header*)((char*)x - 8);
	cr_assert(header->block_size << 4 == 64, "Realloc'ed block size not what was expected!");
	cr_assert(header->allocated == 1, "Allocated bit is not set!");
}

Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 8);
	void *y = sf_realloc(x, sizeof(char));

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_header *header = (sf_header*)((char*)y - 8);
	cr_assert(header->allocated == 1, "Allocated bit is not set!");
	cr_assert(header->block_size << 4 == 48, "Block size not what was expected!");

	free_list *fl = &seg_free_list[find_list_index_from_size(4048)];

	// There should be only one free block of size 4048 in list 3
	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
	cr_assert(fl->head->header.block_size << 4 == 4048, "Free block size not what was expected!");
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_header *header = (sf_header*)((char*)y - 8);
	cr_assert(header->block_size << 4 == 32, "Realloc'ed block size not what was expected!");
	cr_assert(header->allocated == 1, "Allocated bit is not set!");


	// After realloc'ing x, we can return a block of size 48 to the freelist.
	// This block will coalesce with the block of size 4016.
	free_list *fl = &seg_free_list[find_list_index_from_size(4064)];

	cr_assert_not_null(fl->head, "No block in expected free list!");
	cr_assert(fl->head->header.allocated == 0, "Allocated bit is set!");
	cr_assert(fl->head->header.block_size << 4 == 4064, "Free block size not what was expected!");
}


//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sf_memsuite_student, malloc_and_free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(2425);
    free_list *fl = &seg_free_list[find_list_index_from_size(1648)];
    cr_assert(fl->head->header.block_size << 4 == 1648, "Free block wrong!");
    sf_header *head_x = (sf_header *)((char *)x - 8);
    cr_assert(head_x->block_size << 4 == 2448, "Malloc block size wrong");

    void *y = sf_malloc(5362);
    fl = &seg_free_list[find_list_index_from_size(352)];
    cr_assert(fl->head->header.block_size << 4 == 352, "Free block wrong!");
    sf_header *head_y = (sf_header *)((char *)y - 8);
    cr_assert(head_y->block_size << 4 == 5392, "Malloc block size wrong");

    sf_free(x);
    free_list *fl_1 = &seg_free_list[find_list_index_from_size(352)];
    free_list *fl_2 = &seg_free_list[find_list_index_from_size(2448)];
    cr_assert(fl_1->head->header.block_size << 4 == 352, "Free block wrong!");
    cr_assert(fl_2->head->header.block_size << 4 == 2448, "Free block wrong!");

    void *z = sf_malloc(2000);
    //THERE SHOULD BE TWO BLOCKS, SIZE 432 AND 353 IN THE SECOND LIST
    fl = &seg_free_list[find_list_index_from_size(432)];
    cr_assert(fl->head->header.block_size << 4 == 432, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 352, "Free block wrong!!");
    sf_header *head_z = (sf_header *)((char *)z - 8);
    cr_assert(head_z->block_size << 4 == 2016, "Malloc block size wrong");

    sf_free(y);
    fl_1 = &seg_free_list[find_list_index_from_size(432)];
    fl_2 = &seg_free_list[find_list_index_from_size(5744)];

    cr_assert(fl_1->head->header.block_size << 4 == 432, "Free block wrong!");
    cr_assert_null(fl_1->head->next, "There should be no free block!");

    cr_assert(fl_2->head->header.block_size << 4 == 5744, "Free block not expected! MALLOC SBRK IS INCORRECT DURING malloc(5362) CALL");
    cr_assert_null(fl_2->head->next, "There should be no free block!");
}

Test(sf_memsuite_student, malloc_non_head, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(2425);
    free_list *fl = &seg_free_list[find_list_index_from_size(1648)];
    cr_assert(fl->head->header.block_size << 4 == 1648, "Free block wrong!");

    void *y = sf_malloc(5220);
    fl = &seg_free_list[find_list_index_from_size(496)];
    cr_assert(fl->head->header.block_size << 4 == 496, "Free block wrong!");
    sf_header *head_y = (sf_header *)((char *)y - 8);
    cr_assert(head_y->block_size << 4 == 5248, "Malloc block size wrong");

    sf_free(x);
    free_list *fl_1 = &seg_free_list[find_list_index_from_size(496)];
    free_list *fl_2 = &seg_free_list[find_list_index_from_size(2448)];
    cr_assert(fl_1->head->header.block_size << 4 == 496, "Free block wrong!");
    cr_assert(fl_2->head->header.block_size << 4 == 2448, "Free block wrong!");

    void *z = sf_malloc(2000);
    //THERE SHOULD BE TWO BLOCKS, SIZE 432 AND 353 IN THE SECOND LIST
    fl = &seg_free_list[find_list_index_from_size(432)];
    cr_assert(fl->head->header.block_size << 4 == 432, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 496, "Free block wrong!");
    sf_header *head_z = (sf_header *)((char *)z - 8);
    cr_assert(head_z->block_size << 4 == 2016, "Malloc block size wrong");

    void *a = sf_malloc(480);
    //THE SECOND BLOCK IN THE LIST SHOULD BE ALLOCATED. LINKS IN THE LIST FROM THE PREVIOUS BLOCK SHOULD BE SEVERED
    fl = &seg_free_list[find_list_index_from_size(432)];
    cr_assert(fl->head->header.block_size << 4 == 432, "Free block is wrong!");
    cr_assert_null(fl->head->next, "There should be no free block!");
    sf_header *head_a = (sf_header *)((char *)a - 8);
    cr_assert(head_a->block_size << 4 == 496, "Malloc block size wrong");
}

Test(sf_memsuite_student, realloc_smaller_block_coalesce, .init = sf_mem_init, .fini = sf_mem_fini){
    sf_malloc(1500);
    free_list *fl = &seg_free_list[find_list_index_from_size(2576)];
    cr_assert(fl->head->header.block_size << 4 == 2576, "Free block wrong!");

    void *y = sf_malloc(2000);
    fl = &seg_free_list[find_list_index_from_size(560)];
    cr_assert(fl->head->header.block_size << 4 == 560, "Free block wrong!");

    void *z = sf_realloc(y, 1600);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(y == z, "Malloced and Realloced pointers are different!");
}

Test(sf_memsuite_student, realloc_coalesce_and_free, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(1500);
    free_list *fl = &seg_free_list[find_list_index_from_size(2576)];
    cr_assert(fl->head->header.block_size << 4 == 2576, "Free block wrong!");

    void *y = sf_malloc(2000);
    fl = &seg_free_list[find_list_index_from_size(560)];
    cr_assert(fl->head->header.block_size << 4 == 560, "Free block wrong!");

    void *z = sf_realloc(y, 1600);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(y == z, "Malloced and Realloced pointers are different!");

    sf_free(x);
    fl = &seg_free_list[find_list_index_from_size(1520)];
    cr_assert(fl->head->header.block_size << 4 == 1520, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 960, "Next free block wrong!");

    sf_malloc(624);
    fl = &seg_free_list[find_list_index_from_size(880)];
    cr_assert(fl->head->header.block_size << 4 == 880, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 960, "Next free block wrong!");

    sf_realloc(z, 1440);
    fl = &seg_free_list[find_list_index_from_size(1120)];
    cr_assert(fl->head->header.block_size << 4 == 1120, "Free block wrong! BLOCK DID NOT COALESCE OR INCORRECTLY COALESCE DURING REALLOC!");
    cr_assert(fl->head->next->header.block_size << 4 == 880, "Next free block wrong!");
}

Test(sf_memsuite_student, malloc_middle_block, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(1500);
    free_list *fl = &seg_free_list[find_list_index_from_size(2576)];
    cr_assert(fl->head->header.block_size << 4 == 2576, "Free block wrong!");

    void *y = sf_malloc(2000);
    fl = &seg_free_list[find_list_index_from_size(560)];
    cr_assert(fl->head->header.block_size << 4 == 560, "Free block wrong!");

    void *z = sf_realloc(y, 1600);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(y == z, "Malloced and Realloced pointers are different!");

    sf_free(x);
    fl = &seg_free_list[find_list_index_from_size(1520)];
    cr_assert(fl->head->header.block_size << 4 == 1520, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 960, "Next free block wrong!");

    sf_malloc(624);
    fl = &seg_free_list[find_list_index_from_size(880)];
    cr_assert(fl->head->header.block_size << 4 == 880, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 960, "Next free block wrong!");

    void *a = sf_malloc(944);
    fl = &seg_free_list[find_list_index_from_size(880)];
    cr_assert(fl->head->header.block_size << 4 == 880, "Free block wrong!");
    cr_assert_null(fl->head->next, "There should not be a next block in this list. It should be allocated!");

    sf_free(z);
    fl = &seg_free_list[find_list_index_from_size(1616)];
    cr_assert(fl->head->header.block_size << 4 == 1616, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 880, "Next free block is wrong!");

    sf_free(a);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 1616, "Next free block is wrong!");
    cr_assert(fl->head->next->next->header.block_size << 4 == 880, "Next of the next's free block is wrong!");

    sf_malloc(1600);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 880, "Next free block is wrong!");
}

Test(sf_memsuite_student,realloc_same_size_block, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(7);
    free_list *fl = &seg_free_list[find_list_index_from_size(4064)];
    cr_assert(fl->head->header.block_size << 4 == 4064, "Free block wrong!");
    sf_header *head = (sf_header *)((char*)x - 8);
    cr_assert(head->block_size << 4 == 32, "Malloc wrong size block");

    void *y = sf_realloc(x, 8);
    fl = &seg_free_list[find_list_index_from_size(4064)];
    cr_assert(fl->head->header.block_size << 4 == 4064, "Free block wrong!");
    head = (sf_header *)((char*)y - 8);
    cr_assert(head->block_size << 4 == 32, "Malloc wrong size block");

    cr_assert(x == y, "Pointers are not the same!");
}

Test(sf_memsuite_student,realloc_requestedsize_equal_currentblocksize, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(2528);
    free_list *fl = &seg_free_list[find_list_index_from_size(1552)];
    cr_assert(fl->head->header.block_size << 4 == 1552, "Free block wrong!");
    sf_header *head = (sf_header *)((char*)x - 8);
    cr_assert(head->block_size << 4 == 2544, "Malloc wrong size block");

    void *y = sf_realloc(x, 2544);
    fl = &seg_free_list[find_list_index_from_size(2544)];
    cr_assert(fl->head->header.block_size << 4 == 2544, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 3088, "Free block wrong!");
    head = (sf_header *)((char*)y - 8);
    cr_assert(head->block_size << 4 == 2560, "Malloc wrong size block");
}

Test(sf_memsuite_student,malloc_exactly_4_pages, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(16368);
    sf_header *head = (sf_header *)((char*)x - 8);
    cr_assert(head->block_size << 4 == 16384, "Malloc wrong size block");

    free_list *fl;
    for(int i = 0; i < 4; i++){
        fl = &seg_free_list[i];
        cr_assert_null(fl->head, "Malloc wrong size block");
    }
}

Test(sf_memsuite_student,malloc_headblock_of_list, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(1500);
    free_list *fl = &seg_free_list[find_list_index_from_size(2576)];
    cr_assert(fl->head->header.block_size << 4 == 2576, "Free block wrong!");

    void *y = sf_malloc(2000);
    fl = &seg_free_list[find_list_index_from_size(560)];
    cr_assert(fl->head->header.block_size << 4 == 560, "Free block wrong!");

    void *z = sf_realloc(y, 1600);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert(y == z, "Malloced and Realloced pointers are different!");

    sf_free(x);
    fl = &seg_free_list[find_list_index_from_size(1520)];
    cr_assert(fl->head->header.block_size << 4 == 1520, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 960, "Next free block wrong!");

    sf_malloc(1504);
    fl = &seg_free_list[find_list_index_from_size(960)];
    cr_assert(fl->head->header.block_size << 4 == 960, "Free block wrong!");
    cr_assert_null(fl->head->next, "There should not be a next block!");
}

Test(sf_memsuite_student,many_malloc_realloc_free, .init = sf_mem_init, .fini = sf_mem_fini){
    void *x = sf_malloc(1000);
    free_list *fl = &seg_free_list[find_list_index_from_size(3072)];
    cr_assert(fl->head->header.block_size << 4 == 3072, "Free block wrong!");
    sf_header *head = (sf_header *)((char *)x - 8);
    cr_assert(head->block_size << 4 == 1024, "Wrong malloc block size!");

    void *y = sf_malloc(4000);
    fl = &seg_free_list[find_list_index_from_size(3152)];
    cr_assert(fl->head->header.block_size << 4 == 3152, "Free block wrong!");
    head = (sf_header *)((char *)y - 8);
    cr_assert(head->block_size << 4 == 4016, "Wrong malloc block size!");

    void *z = sf_realloc(x, 840);
    fl = &seg_free_list[find_list_index_from_size(160)];
    cr_assert(fl->head->header.block_size << 4 == 160, "Free block wrong!");
    cr_assert_null(fl->head->next, "There should not be a next free block");
    head = (sf_header *)((char *)z - 8);
    cr_assert(head->block_size << 4 == 864, "Wrong realloc block size!");
    cr_assert(x == z, "Wrong realloc pointer!");
    free_list *fl_2 = &seg_free_list[find_list_index_from_size(3152)];
    cr_assert(fl_2->head->header.block_size << 4 == 3152, "Free block wrong!");
    cr_assert_null(fl_2->head->next, "There should not be a next free block");

    void *a = sf_realloc(z, 400);
    fl = &seg_free_list[find_list_index_from_size(608)];
    cr_assert(fl->head->header.block_size << 4 == 608, "Free block wrong!");
    cr_assert_null(fl->head->next, "There should not be a next free block");
    head = (sf_header *)((char *)a - 8);
    cr_assert(head->block_size << 4 == 416, "Wrong realloc block size!");
    cr_assert(z == a, "Wrong realloc pointer!");
    fl_2 = &seg_free_list[find_list_index_from_size(3152)];
    cr_assert(fl_2->head->header.block_size << 4 == 3152, "Free block wrong!");
    cr_assert_null(fl_2->head->next, "There should not be a next free block!");

    sf_free(y);
    fl = &seg_free_list[find_list_index_from_size(7168)];
    cr_assert(fl->head->header.block_size << 4 == 7168, "Free block wrong!");
    fl = &seg_free_list[find_list_index_from_size(608)];
    cr_assert(fl->head->header.block_size << 4 == 608, "Free block wrong!");

    void *b = sf_malloc(6400);
    fl = &seg_free_list[find_list_index_from_size(752)];
    cr_assert(fl->head->header.block_size << 4 == 752, "Free block wrong!");
    cr_assert(fl->head->next->header.block_size << 4 == 608, "There should be a next block!");
    fl_2 = &seg_free_list[find_list_index_from_size(7168)];
    cr_assert_null(fl_2->head, "There should be no free block");
    head = (sf_header *)((char *)b - 8);
    cr_assert(head->block_size << 4 == 6416, "Wrong malloc block size!");
}