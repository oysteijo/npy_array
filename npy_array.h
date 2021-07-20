#ifndef __NPY_ARRAY_H__
#define __NPY_ARRAY_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define NPY_ARRAY_MAX_DIMENSIONS 8

typedef struct _npy_array_t {
    char             *data;
    size_t            shape[ NPY_ARRAY_MAX_DIMENSIONS ];
    int32_t           ndim;
    char              endianness;
    char              typechar;
    size_t            elem_size;
    bool              fortran_order;
} npy_array_t;

typedef struct _npy_array_list_t {
    npy_array_t      *array;
    char             *filename;
    struct _npy_array_list_t *next;
} npy_array_list_t;

npy_array_t*      npy_array_load       ( const char *filename);
void              npy_array_dump       ( const npy_array_t *m );
void              npy_array_save       ( const char *filename, const npy_array_t *m );
void              npy_array_free       ( npy_array_t *m );

npy_array_list_t* npy_array_list_load  ( const char *filename );
int               npy_array_list_save  ( const char *filename, npy_array_list_t *array_list );
size_t            npy_array_list_length( npy_array_list_t *array_list);
void              npy_array_list_free  ( npy_array_list_t *array_list);

npy_array_list_t* npy_array_list_prepend( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);
npy_array_list_t* npy_array_list_append ( npy_array_list_t *list, npy_array_t *array, const char *filename, ...);

#endif  /* __NPY_ARRAY_H__ */
