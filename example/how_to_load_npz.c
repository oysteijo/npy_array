#include <npy_array_list.h>
#include <stdio.h>

int main(int argc , char *argv[] )
{
    if( argc != 2 ){
        printf("Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    npy_array_list_t *list = npy_array_list_load( argv[1] );

    if( !list ){
        printf("Cannot read .npz file '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    }
    
    size_t len = npy_array_list_length( list );

    printf("number of npy_arrays in file: %lu\n", len);
    for ( npy_array_list_t *iter = list; iter; iter = iter->next ){
        printf("File: %s\n", iter->filename ? iter->filename : "(None)");
        npy_array_dump( iter->array );
    }

    /* Keep in mind that `npy_array_list_load()` load everything into memory. Memory for
       the list structure, the npy_arrays and the array data - are all allocated on the heap.
       You should therefor call `np_array_list_free()` to clean up. Ant that clean everything.
       Make sure you *do not* call `npy_array_list_free()` if you do not have allocated
       everything on the heap.
    */

    npy_array_list_free( list );
    return 0;
}

