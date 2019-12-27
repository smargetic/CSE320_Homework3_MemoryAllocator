#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();
    double* ptr = sf_malloc(sizeof(double));
    double* ptr2 = sf_malloc(3*sizeof(double));
    double* ptr3 = sf_malloc(6*sizeof(double));

    printf("%f\n", *ptr);
    printf("%f\n", *ptr2);
    printf("%f\n", *ptr3);

    sf_free(ptr);
    double* newPtr = sf_realloc(ptr2, 2*sizeof(double));



    printf("%f\n", *newPtr);

    sf_mem_fini();

    return EXIT_SUCCESS;
}
