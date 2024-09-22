#include "npy_array_list.h"

int main(int argc, char *argv[])
{
    if( argc != 2 ) return -1;

    double data[] = {0,1,2,3,4,5};

    npy_array_list_t* list = NULL;

    // the first npy_array_t* holds a reference to the data array
    list = npy_array_list_append( list,
        NPY_ARRAY_BUILDER_COPY(data, SHAPE(3,2), NPY_DTYPE_FLOAT64), "matrix" );
    // the second npy_array_t* holds a copy of the data array (hence DEEPCOPY)
    list = npy_array_list_append( list,
        NPY_ARRAY_BUILDER_DEEPCOPY(data, SHAPE(2,1,2), NPY_DTYPE_FLOAT64), "tensor" );

    npy_array_list_save_compressed( argv[1], list, ZIP_CM_DEFAULT, 0 );
    npy_array_list_free( list );
}
