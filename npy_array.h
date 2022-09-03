/* npy_array.h
 
npy_array - C library for handling numpy arrays
 
Copyright (C) 2020-2022 

   Øystein Schønning-Johansen <oysteijo@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the
   distribution.

3. The names of the authors may not be used to endorse or promote
   products derived from this software without specific prior
   written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
    /* Consider map_addr as a private member. Do modify this pointer! Used for unmap() */
    void             *map_addr;      /* pointer to the map if array is mmap()'ed -- else NULL */
} npy_array_t;


npy_array_t*      npy_array_load       ( const char *filename );
npy_array_t*      npy_array_mmap       ( const char *filename );
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

#define _NARG( ...) _NARG_(__VA_ARGS__,8,7,6,5,4,3,2,1,0)
#define _NARG_(...) _ARG_N(__VA_ARGS__)
#define _ARG_N(_1,_2,_3,_4,_5,_6,_7,_8,N,...) N

#define SHAPE(...) .shape = {__VA_ARGS__}, .ndim = _NARG(__VA_ARGS__)
#define NPY_ARRAY_BUILDER(_data,_shape,...) \
    &(npy_array_t){ .data=(char*)_data, _shape, __VA_ARGS__ } 

#define NPY_DTYPE_FLOAT16    .typechar='f', .elem_size=2
#define NPY_DTYPE_FLOAT32    .typechar='f', .elem_size=4
#define NPY_DTYPE_FLOAT64    .typechar='f', .elem_size=8
#define NPY_DTYPE_FLOAT128   .typechar='f', .elem_size=16

#define NPY_DTYPE_INT8       .typechar='i', .elem_size=1
#define NPY_DTYPE_INT16      .typechar='i', .elem_size=2
#define NPY_DTYPE_INT32      .typechar='i', .elem_size=4
#define NPY_DTYPE_INT64      .typechar='i', .elem_size=8

#define NPY_DTYPE_UINT8      .typechar='u', .elem_size=1
#define NPY_DTYPE_UINT16     .typechar='u', .elem_size=2
#define NPY_DTYPE_UINT32     .typechar='u', .elem_size=4
#define NPY_DTYPE_UINT64     .typechar='u', .elem_size=8

#define NPY_DTYPE_COMPLEX64  .typechar='c', .elem_size=8
#define NPY_DTYPE_COMPLEX128 .typechar='c', .elem_size=16
#define NPY_DTYPE_COMPLEX256 .typechar='c', .elem_size=32

#define NPY_DTYPE_BOOL       .typechar='b', .elem_size=1

#endif  /* __NPY_ARRAY_H__ */
