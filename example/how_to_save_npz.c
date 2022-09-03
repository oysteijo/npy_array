#include <npy_array_list.h>
#include <stdlib.h>
#include <assert.h>

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

int main()
{
    /* set some sizes */
    int n_rows = 4;
    int n_cols = 3;

    /* OK - Now we want to save a .npz file. */

    int *iarray = malloc( n_rows * n_cols * sizeof(int));                 assert(iarray);
    for (int i = 0; i < n_rows * n_cols; i++ )
        iarray[i] = i;
    double *darray = malloc( n_rows * n_cols * sizeof(double));           assert(darray);
    for (int i = 0; i < n_rows * n_cols; i++ )
        darray[i] = (double) i;

    /* Wrap your np_array_t into a np_arra_list_t element. */
    npy_array_list_t head = {
        .array = NPY_ARRAY_BUILDER( iarray, SHAPE( n_rows, n_cols ), NPY_DTYPE_INT32 ) };

    /* You can also add an internal filename, of course. However, if no filename is given
       filenames like arr_0.npy, arr_1.npy will automatically be assigned. */


    /* Wrap the next element (array) in the list into another npy_arra_list_t struct */
    npy_array_list_t  another = {
        .array = NPY_ARRAY_BUILDER( darray, SHAPE( n_rows, n_cols ), NPY_DTYPE_FLOAT64 ) };

    /* Connect the two */
    head.next = &another;

    /* and save them */
    npy_array_list_save( "iarray_and_darray.npz", &head );

    free( iarray );
    free( darray );

    return 0;
}

