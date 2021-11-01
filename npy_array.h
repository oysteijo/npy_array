#ifndef __NPY_ARRAY_H__
#define __NPY_ARRAY_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

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


npy_array_t*      npy_array_load       ( const char *filename );
void              npy_array_dump       ( const npy_array_t *m );
void              npy_array_save       ( const char *filename, const npy_array_t *m );
void              npy_array_free       ( npy_array_t *m );

/* Convenient functions - I'll make them public for now, but use these with care
   as I might remove these from the exported list of public functions. */
size_t            npy_array_calculate_datasize ( const npy_array_t *m );
size_t            npy_array_get_header         ( const npy_array_t *m,  char *buf );

static inline int64_t read_file( void *fp, void *buffer, uint64_t nbytes )
{
    return (int64_t) fread( buffer, 1, nbytes, (FILE *) fp );
}

/* _read_matrix() might be public in the future as a macro or something.
   Don't use it now as I will change name of it in case I make it public. */
typedef int64_t (*reader_func)( void *fp, void *buffer, uint64_t nbytes );
npy_array_t *     _read_matrix( void *fp, reader_func read_func );
#endif  /* __NPY_ARRAY_H__ */
