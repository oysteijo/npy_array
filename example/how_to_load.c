#include <npy_array.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    if( argc != 2 ){
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    npy_array_t *arr = npy_array_load( argv[1] );
    if( !arr ){
        printf("Cannot read NumPy file '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }

    npy_array_dump( arr );

    float *array;
    int n_rows, n_cols;
    if( (arr->typechar == 'f') && (arr->elem_size == 4) && (arr->ndim = 2) ){
        array = (float*) arr->data;
        n_rows = arr->shape[0];
        n_cols = arr->shape[1];
        for( int i = 0; i < n_rows; i++ ) {
            for( int j = 0; j < n_cols; j++ )
                printf( "%5.1f  ", *(array + i * n_cols + j ));
            printf("\n");
        }
    } else
        printf("Try with a npy array of 2 dimensions and 'float32' precision elements.\n");

    npy_array_free( arr );
    return 0;
}
