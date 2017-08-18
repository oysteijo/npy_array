#include "c_npy.h"

int main(int argc, char *argv[])
{
    if( argc != 2 ) return -1;

    cmatrix_t **arr = c_npy_matrix_array_read( argv[1] );
    size_t len = c_npy_matrix_array_length( arr );
    for (unsigned int i = 0; i < len; i++ )
        c_npy_matrix_dump( arr[i] );

    //c_npy_matrix_write_file( "tester_save.npy", m);
    c_npy_matrix_array_free( arr );
    return 0;
}
