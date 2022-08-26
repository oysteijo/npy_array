#include <npy_array.h>
#include <stdlib.h>
int main()
{
    /* set some sizes */
    int n_rows = 4;
    int n_cols = 3;

    /* Allocate the raw data - this can be from a blas or another */
    float *arraydata = malloc( n_rows * n_cols * sizeof( float ));

    /* fill in some data */
    for( int i = 0; i < n_rows * n_cols; i++ )
        arraydata[i] = (float) i;

    npy_array_t array = {
        .data = (char*) arraydata,
        .shape = { n_rows, n_cols },
        .ndim = 2,
        .endianness = '<',
        .typechar = 'f',
        .elem_size = sizeof(float),
    };
    npy_array_save( "my_4_by_3_array.npy", &array );
    free ( arraydata );
    return 0;
}


