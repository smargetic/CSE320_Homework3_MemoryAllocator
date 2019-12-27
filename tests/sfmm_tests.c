#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"

#define MIN_BLOCK_SIZE (32)

void assert_free_block_count(size_t size, int count);
void assert_free_list_block_count(size_t size, int count);

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & BLOCK_SIZE_MASK))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		 size, count, cnt);
}

Test(sf_memsuite_student, malloc_an_Integer_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}


Test(sf_memsuite_student, malloc_three_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(3 * PAGE_SZ - 2 * sizeof(sf_block));

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}



Test(sf_memsuite_student, malloc_over_four_pages, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 2);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(16336, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}


Test(sf_memsuite_student, free_quick, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(32);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(3936, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_no_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(224, 1);
	assert_free_block_count(3760, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sf_memsuite_student, free_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(544, 1);
	assert_free_block_count(3440, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}


Test(sf_memsuite_student, freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(224, 3);
	assert_free_block_count(1808, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)",

                    i, bp, (char *)y - 2*sizeof(sf_header));
}


Test(sf_memsuite_student, realloc_larger_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int));
	/* void *y =*/  sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 10);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 64, "Realloc'ed block size not what was expected!");

	assert_free_block_count(0, 2);
	assert_free_block_count(3920, 1);
}


Test(sf_memsuite_student, realloc_smaller_block_splinter, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(int) * 8);
	void *y = sf_realloc(x, sizeof(char));

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 48, "Block size not what was expected!");

	// There should be only one free block of size 4000.
	assert_free_block_count(0, 1);
	assert_free_block_count(4000, 1);
}

Test(sf_memsuite_student, realloc_smaller_block_free_block, .init = sf_mem_init, .fini = sf_mem_fini) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!"); //SOMETHING WRONG
	cr_assert((bp->header & BLOCK_SIZE_MASK) == 32, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size 48 to the freelist.
	// This block will go into the main freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

//this test tests the freeing of a block where there is a free block to the right of it
Test(sf_memsuite_student, free_right_coalesce, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	/*void *x =*/ sf_malloc(200);
	/*void *y =*/ sf_malloc(300);
	 void *z = sf_malloc(4);

//	sf_free(y);
	sf_free(z);

	assert_free_block_count(0, 1);
	//assert_free_block_count(544, 1);
	assert_free_block_count(3472, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}


//this tests freeing a block right before the epilogue

Test(sf_memsuite_student, free_coalesce_at_end, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	void*x =sf_malloc(32);
	void *z =sf_malloc(320);
	//sf_free(y);
	sf_free(x);
	sf_free(z);

	assert_free_block_count(0, 1);
	//assert_free_block_count(544, 1);
	assert_free_block_count(4048, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	///*void*x=*/sf_malloc(1);
	//void *a =sf_malloc(320);
	///*void *b =*/sf_malloc(320);
	//void *c =sf_malloc(320);
	///*void *d =*/sf_malloc(320);
	//assert_free_block_count(0, 3);
	//sf_free(y);
	//sf_free(c);
	//sf_free(a);

	//assert_free_block_count(0, 1);
	//assert_free_block_count(544, 1);
	//assert_free_block_count(4048, 1);
	//cr_assert(sf_errno == 0, "sf_errno is not zero!");
}



//this thests freeing a block right after the prologue
Test(sf_memsuite_student, free_coalesce_at_beginning, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;

	void*x =sf_malloc(32);
	void *z =sf_malloc(320);
	//sf_free(y);
	sf_free(z);
	sf_free(x);

	assert_free_block_count(0, 1);
	//assert_free_block_count(544, 1);
	assert_free_block_count(4048, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

//tests double --> dif size than integer
Test(sf_memsuite_student, malloc_an_Double_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(double));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

//tests array
Test(sf_memsuite_student, malloc_an_Array_check_freelist, .init = sf_mem_init, .fini = sf_mem_fini) {
	sf_errno = 0;
	char arr[4];
	//= "array[10];"
	int size = sizeof(arr);
	int *x = sf_malloc(size);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}


//this tests reallocation to the same size
//Test(sf_memsuite_student, realloc_sameSize, .init = sf_mem_init, .fini = sf_mem_fini) {
//	void *x = sf_malloc(sizeof(int));
//	/* void *y =*/  sf_malloc(10);

//	x = sf_realloc(x, sizeof(int));

//	cr_assert_not_null(x, "x is NULL!");
//	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
//	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
//	cr_assert((bp->header & BLOCK_SIZE_MASK) == 48, "Realloc'ed block size not what was expected!");

	//assert_free_block_count(0, 1);
	//assert_free_block_count(3920, 1);
//}

//tests if free is given with nonesistant pointer

//Test(sf_memsuite_student, free_nonexist, .init = sf_mem_init, .fini = sf_mem_fini) {
	//sf_errno = 0;
	///* void *x = */ sf_malloc(8);
	//int rand = 8;
	///*void *y =*/ sf_malloc(200);
	///* void *z = */ sf_malloc(1);

	//sf_free(&rand);

	///assert_free_block_count(0, 2);
	//assert_free_block_count(224, 1);
	//assert_free_block_count(3760, 1);
	//cr_assert(sf_errno == 0, "sf_errno is zero!");
//}
