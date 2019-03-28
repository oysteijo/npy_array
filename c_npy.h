#ifndef __C_NPY_H__
#define __C_NPY_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define C_NPY_MAX_DIMENSIONS 8

typedef struct _cmatrix_t {
    char    *data;
    size_t   shape[ C_NPY_MAX_DIMENSIONS ];
    int32_t  ndim;
    char     endianness;
    char     typechar;
    size_t   elem_size;
    bool     fortran_order;
} cmatrix_t;

cmatrix_t *  c_npy_matrix_read_file   ( const char *filename);
void         c_npy_matrix_dump        ( const cmatrix_t *m );
void         c_npy_matrix_write_file  ( const char *filename, const cmatrix_t *m );
void         c_npy_matrix_free        ( cmatrix_t *m );

cmatrix_t ** c_npy_matrix_array_read  ( const char *filename );
int          c_npy_matrix_array_write ( const char *filename, const cmatrix_t **array );
size_t       c_npy_matrix_array_length( const cmatrix_t **arr);
void         c_npy_matrix_array_free  ( cmatrix_t **arr);

#endif  /* __C_NPY_H__ */
