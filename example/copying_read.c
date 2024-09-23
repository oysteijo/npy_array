#include "npy_array_list.h"

#include <string.h>
#define MIN(x,y) ((x)<(y)?(x):(y))

// the first block must be the same as in copying.c
int main(int argc, char *argv[])
{
    const char* fname = (argc != 2) ? "copy.npz" : argv[1];

    const double data[] = {0,1,2, 3,4,5};
    const int32_t idata[] = {0,1, 2,3,
                             4,5, 6,7,
                             8,9, 10,11};
    const char names[][8] = { "double", "int" };

    // we load the file and check whether it contains the same data
    npy_array_list_t *list = npy_array_list_load( fname );

    int errors = 0;
    int visited_data = 0,
        visited_idata = 0;
    npy_array_list_t *elem = list;
    while (elem != NULL) {
        npy_array_t* ary = elem->array;
        size_t ary_sz = npy_array_calculate_datasize( ary );

        char* cmp = NULL;
        size_t cmp_sz = 0;

        if (!strcmp( elem->filename, names[0] )) {
            cmp = (char*)data;
            cmp_sz = sizeof(data);
            visited_data++;
        } else if (!strcmp( elem->filename, names[1] )) {
            cmp = (char*)idata;
            cmp_sz = sizeof(idata);
            visited_idata++;
        }

        errors += (cmp_sz != ary_sz);
        if (cmp != NULL) {
            errors += (memcmp( ary->data, cmp, MIN(cmp_sz,ary_sz) ) != 0);
        } else {
            errors++;
        }

        elem = elem->next;
    }

    errors += (visited_data != 1);
    errors += (visited_idata != 1);

    npy_array_list_free( list );

    // we return the number of errors/inconsistencies
    return errors;
}
