#include "c_npy.h"
#include <stdio.h>
int main(int argc, char *argv[])
{
    if( argc != 2 ){
        printf("Usage: %s <filename>\n\nProvided filename is supposed to contain a numpy matrix\n", argv[0]);
        return -1;
    }

    cmatrix_t **arr = c_npy_matrix_array_read( argv[1] );
    size_t len = c_npy_matrix_array_length( arr );
    if( len == 0 ){
        /* This is possibly a matrix stored with 'save' instead of 'savez' */
        cmatrix_t *arr = c_npy_matrix_read_file( argv[1] );
        if( !arr ){
            printf("nope! No array there!\n");
            return -1;
        }

        c_npy_matrix_dump( arr );
        c_npy_matrix_free( arr );
        return 0;
    }

    printf("number of objects in file: %lu\n", len);
    for (unsigned int i = 0; i < len; i++ ){
        c_npy_matrix_dump( arr[i] );
        char fn[40];
        sprintf(fn, "written%d.npy", i);
        c_npy_matrix_write_file( fn, arr[i] );
    }


    //c_npy_matrix_write_file( "tester_save.npy", m);
    c_npy_matrix_array_free( arr );
    return 0;
}
