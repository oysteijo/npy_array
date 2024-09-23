#include "npy_array_list.h"

// the first block must be the same as in copying_read.c
int main(int argc, char *argv[])
{
    const char* fname = (argc != 2) ? "copy.npz" : argv[1];

    const double data[] = {0,1,2, 3,4,5};
    const int32_t idata[] = {0,1, 2,3,
                             4,5, 6,7,
                             8,9, 10,11};
    const char names[][8] = { "double", "int" };

    npy_array_list_t* list = NULL;

    // the first npy_array_t* holds a reference to the data array
    list = npy_array_list_append( list,
        NPY_ARRAY_BUILDER_COPY(data, SHAPE(2,3), NPY_DTYPE_FLOAT64), names[0] );
    // the second npy_array_t* holds a copy of the data array (hence DEEPCOPY)
    list = npy_array_list_append( list,
        NPY_ARRAY_BUILDER_DEEPCOPY(idata, SHAPE(3,2,2), NPY_DTYPE_INT32), names[1] );

    npy_array_list_save_compressed( fname, list, ZIP_CM_DEFAULT, 0 );
    npy_array_list_free( list );
}
