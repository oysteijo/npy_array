/* npy_array_list.h 

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

static inline int64_t read_zip( void *fp, void *buffer, uint64_t nbytes )
{
    return (int64_t) zip_fread( (zip_file_t *) fp, buffer, nbytes );
}

#endif /*  __NPY_ARRAY_LIST_H__  */
