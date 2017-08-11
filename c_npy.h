#ifndef __C_NPY_H__
#define __C_NPY_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define C_NPY_MAX_DIMENSIONS 8

typedef struct _cmatrix_t {
    char    *data;
    size_t   shape[ C_NPY_MAX_DIMENSIONS ];
    int32_t  ndim;
    char     endianness;
    char     typechar;
    size_t   itemsize;
    bool     fortran_order;
} cmatrix_t;

cmatrix_t * c_npy_read_from_file    ( const char *filename);
void        c_npy_dump              ( const cmatrix_t *m );
void        c_npy_write_to_file     ( const char *filename, const cmatrix_t *m );
void        c_npy_matrix_free       (       cmatrix_t *m );

#endif  /* __C_NPY_H__ */
