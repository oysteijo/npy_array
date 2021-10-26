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

typedef int64_t (*reader_func)( void *fp, void *buffer, uint64_t nbytes );

npy_array_t*      npy_array_load       ( const char *filename );
void              npy_array_dump       ( const npy_array_t *m );
void              npy_array_save       ( const char *filename, const npy_array_t *m );
void              npy_array_free       ( npy_array_t *m );

npy_array_t *     _read_matrix         ( void *fp, reader_func read_func );
#endif  /* __NPY_ARRAY_H__ */
