#include "npy_array.h"
#include <stdio.h>
int main(int argc, char *argv[])
{
    if( argc != 2 ){
        printf("Usage: %s <filename>\n\nProvided filename is supposed to contain a numpy matrix\n", argv[0]);
        return -1;
    }

    npy_array_list_t *list = npy_array_list_load( argv[1] );
    size_t len = npy_array_list_length( list );
    printf("Length: %d\n", (int) len );
    if( len == 0 ){
        /* This is possibly a matrix stored with 'save' instead of 'savez' */
        npy_array_t *arr = npy_array_load( argv[1] );
        if( !arr ){
            printf("nope! No array there!\n");
            return -1;
        }

        npy_array_dump( arr );
        npy_array_free( arr );
        return 0;
    }

    printf("number of objects in file: %lu\n", len);
    int filename_counter = 0;
    for ( npy_array_list_t *iter = list; iter; iter = iter->next ){
        npy_array_dump( iter->array );
        char fn[30];
        sprintf( fn, "array_%d.npy", filename_counter++ );
        printf( "'%s'\n", iter->filename ? iter->filename : "(NULL)" );
        npy_array_save( iter->filename ? iter->filename : fn , iter->array );
    }

    npy_array_list_save( "written_array.npz", list);
    npy_array_list_free( list );
    return 0;
}
