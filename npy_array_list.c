/* npy_array_list.c
 
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

#include "npy_array_list.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#define MAX_FILENAME_LEN 80
static npy_array_list_t * npy_array_list_new()
{
    npy_array_list_t *list = malloc( sizeof(npy_array_list_t ));
    if( !list )
        return NULL; /* Whoops. */

    list->array    = NULL;
    list->filename = NULL;
    list->next     = NULL;
    return list;
}

void npy_array_list_free( npy_array_list_t *list )
{
    if( !list )
        return;
    if( list->array )
        npy_array_free( list->array );
    if( list->filename )
        free( list->filename );
    if( list->next )
        npy_array_list_free( list->next );
    free( list );
}

static char * _va_args_filename( const char *filename, va_list ap1 )
{
    va_list ap2;
    int len;
     
    va_copy( ap2, ap1); 
    len = vsnprintf( NULL, 0, filename, ap1 );
    va_end( ap1 );

    if( len > MAX_FILENAME_LEN ){
        /* If someone is trying to cook up a special filename that can contain executable code, I think it is wise
           to limit the length of the filename */
        fprintf( stderr, "Warning: Cannot save numpy array. Your filename is too long."
                " Please limit the filename to %d characters.\n", MAX_FILENAME_LEN );
        return NULL;
    }

    char *real_filename = malloc( (len+1) * sizeof(char));
    assert( real_filename );

    vsprintf( real_filename, filename, ap2 );
    va_end( ap2 );

    return real_filename;
}

static npy_array_list_t * _list_append( npy_array_list_t *list, npy_array_list_t *new_elem )
{
    if(list) {
        npy_array_list_t *last = list;
        while(last->next)
            last = last->next;
        last->next = new_elem;
        return list;
    }
    new_elem->next = NULL;
    return new_elem;
}

static npy_array_list_t * _list_prepend( npy_array_list_t *list, npy_array_list_t *new_elem )
{
    new_elem->next = list;
    return new_elem;
}

#define create_extend_list_func(oper) \
npy_array_list_t * npy_array_list_ ##oper ( npy_array_list_t *list, npy_array_t *array, const char *filename, ... ) \
{ \
    npy_array_list_t *new_list = npy_array_list_new(); \
    if ( !new_list ) return list; \
    new_list->array = array; \
\
    va_list ap1; \
    va_start( ap1, filename ); \
    new_list->filename = _va_args_filename( filename, ap1 ); \
    assert( new_list->filename ); \
    va_end( ap1 ); \
\
    return _list_ ##oper ( list, new_list ); \
} 
/* Expand the macros */
create_extend_list_func(append)
create_extend_list_func(prepend)
#undef create_extend_list_func

static char * _new_internal_filename( int n )
{
    char *filename = malloc( MAX_FILENAME_LEN * sizeof(char) );
    if( filename )
        sprintf( filename, "arr_%d.npy", n );
    return filename;
}

int npy_array_list_save( const char *filename, npy_array_list_t *array_list )
{
    return npy_array_list_save_compressed( filename, array_list, ZIP_CM_STORE, 0);
}

#define _BUFSIZE 1024
int npy_array_list_save_compressed( const char *filename, npy_array_list_t *array_list, zip_int32_t comp, zip_uint32_t comp_flags)
{
    if ( !array_list )
        return 0;

    /* FIXME: Better test */
    zip_t *zip = zip_open( filename, ZIP_CREATE | ZIP_TRUNCATE, NULL );
    assert( zip );

    int n = 0;
    for( npy_array_list_t *iter = array_list; iter; iter = iter->next ){
        if ( zip_set_file_compression(zip, n, comp, comp_flags) < 0){
            /* what is wrong? */
        }
        /* if the filename is not set, set one. However.. if the list was created with append and prepend,
         * this will never be true. */
        if(!iter->filename)
            iter->filename = _new_internal_filename( n );

        char header[_BUFSIZE] = {'\0'};

        npy_array_t *m = iter->array;

        size_t hlen = npy_array_get_header( m, header );
        size_t datasize = npy_array_calculate_datasize( m );

        char *matrix_npy_format = malloc( hlen + datasize );
        assert( matrix_npy_format );
        memcpy( matrix_npy_format, header, hlen );
        memcpy( matrix_npy_format + hlen, m->data, datasize );

        /* FIXME: Check for error */
        zip_source_t *s = zip_source_buffer_create( matrix_npy_format, hlen + datasize, 1, NULL);
        assert(s); 

        int idx = zip_file_add( zip, iter->filename, s, ZIP_FL_ENC_UTF_8 );
        if( idx != n )
            fprintf( stderr, "Warning: Index and counter mismatch.");

        // fprintf(stderr, "Error: %s\n", zip_strerror( zip ));
        
        n++;
    }
    if ( zip_close( zip ) < 0 ){
        fprintf(stderr, "What? no close?\n");
        fprintf(stderr, "Error: %s\n", zip_strerror( zip ));
    }
    return n;
}

npy_array_list_t * npy_array_list_load( const char *filename )
{
    /* FIXME: better check. what went wrong? */
    zip_t *zip = zip_open(filename, ZIP_RDONLY, NULL );
    if( !zip ){
        fprintf(stderr, "cannot zip_open file: %s\n", filename );
        return NULL;
    }

    npy_array_list_t *list = NULL;
    for( int i = 0; i < zip_get_num_entries( zip, 0 ); i++ ){
        zip_file_t *fp = zip_fopen_index( zip, i, 0 ); /* FIXME Better checks */
        if (!fp ){
            fprintf(stderr, "Warning: Cannot open internal file of index %d in archive '%s'.\n", i, filename );
            continue;
        }
        npy_array_t *arr = _read_matrix( fp, &read_zip );
        zip_fclose( fp );
        if(!arr) {
            fprintf(stderr, "Warning: Cannot read matrix.\n");
            continue;
        }
        list = npy_array_list_append( list, arr, zip_get_name( zip, i, 0 ));
    }

    zip_close(zip);
    
    return list;
}

size_t npy_array_list_length( npy_array_list_t *arr)
{
    if (!arr) return 0;

    size_t len = 0;
    npy_array_list_t *iter = arr;

    while( iter ){
        len++;
        iter = iter->next;
    }
    return len;
}

