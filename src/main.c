#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

    sf_mem_init();

    double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    printf("%p\n", ptr);

///////////////////////////////////////////////////////////////////////////

    void* ptr2 = sf_malloc(2500);

    printf("%p\n", ptr2);
///////////////////////////////////////////////////////////////////////////

    void* ptr3 = sf_malloc(6000);

    printf("%p\n", ptr3);

///////////////////////////////////////////////////////////////////////////

    void* ptr4 = sf_malloc(7725);

    printf("%p\n", ptr4);


    sf_free(ptr);

///////////////////////////////////////////////////////////////////////////////
    void* ptr5 = sf_malloc(sizeof(double));

    printf("%p\n", ptr5);
///////////////////////////////////////////////////////////////////////////////

    // void *ptr = sf_malloc(48);
    // printf("%p\n", ptr);

    // // void *ptr_realloc = sf_realloc(ptr, 32);
    // void *ptr_realloc = sf_realloc(ptr, 16);
    // printf("%p\n", ptr_realloc);


    // void *x = sf_malloc(1500);

    // void *y = sf_malloc(2000);

    // void *z = sf_realloc(y, 1600);

    // sf_free(x);

    // sf_malloc(624);

    // void *a = sf_malloc(944);

    // sf_free(z);

    // sf_free(a);

    // sf_malloc(1600);

    // void *x = sf_malloc(2528);
    // sf_realloc(x, 2544);

    // void *x = sf_malloc(2425);
    // void *y = sf_malloc(5362);
    // sf_free(x);
    // sf_malloc(2000);
    // sf_free(y);

    sf_mem_fini();

    return EXIT_SUCCESS;

}
