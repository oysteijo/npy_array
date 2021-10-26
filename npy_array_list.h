#ifndef __NPY_ARRAY_LIST_H__
#define __NPY_ARRAY_LIST_H__

#include "npy_array.h"
#include <zip.h>

typedef struct _npy_array_list_t {
    npy_array_t      *array;
    char             *filename;
    struct _npy_array_list_t *next;
} npy_array_list_t;

npy_array_list_t* npy_array_list_load           ( const char *filename );
int               npy_array_list_save           ( const char *filename, npy_array_list_t *array_list );
int               npy_array_list_save_compressed( const char *filename, npy_array_list_t *array_list,
                                                  zip_int32_t comp, zip_uint32_t comp_flags);
size_t            npy_array_list_length         ( npy_array_list_t *array_list);
void              npy_array_list_free           ( npy_array_list_t *array_list);

npy_array_list_t* npy_array_list_prepend( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);
npy_array_list_t* npy_array_list_append ( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);

#endif /*  __NPY_ARRAY_LIST_H__  */
