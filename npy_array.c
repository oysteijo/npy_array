/* npy_array.c
 
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

#include "npy_array.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define NPY_ARRAY_MAGIC_STRING {0x93,'N','U','M','P','Y'}
#define NPY_ARRAY_MAJOR_VERSION_IDX 6
#define NPY_ARRAY_MINOR_VERSION_IDX 7

#define NPY_ARRAY_HEADER_LENGTH 2
#define NPY_ARRAY_HEADER_LENGTH_LOW_IDX 8
#define NPY_ARRAY_HEADER_LENGTH_HIGH_IDX 9

#define NPY_ARRAY_SHAPE_BUFSIZE 512

#define NPY_ARRAY_DICT_BUFSIZE 1024
#define NPY_ARRAY_MAGIC_LENGTH 6
#define NPY_ARRAY_VERSION_HEADER_LENGTH 4
#define NPY_ARRAY_PREHEADER_LENGTH (NPY_ARRAY_MAGIC_LENGTH + NPY_ARRAY_VERSION_HEADER_LENGTH)

static int64_t read_mapped( void *map, void *buffer, uint64_t nbytes );
typedef struct _map_handler_t {
    char *start_pos;
    char *current_pos;
    size_t length;
} map_handler_t;

/* If the endianness can be omitted, that would be very nice. Let's make a guess
   if it isn't set. */
static inline char _guess_endianness( const npy_array_t *m )
{
    if( (!strchr( "<>|", (int)  m->endianness) || (m->endianness==0) )){
        if( m->elem_size == 1) return '|';
        if( m->typechar == 'S') return '|';
        int val = 1;
        return (*(char *)&val == 1) ? '<' : '>';
    } else 
        return m->endianness;
}

size_t npy_array_get_header( const npy_array_t *m,  char *buf )
{
    char *p = buf;

    static char magic[] = NPY_ARRAY_MAGIC_STRING;
    memcpy( p, magic, NPY_ARRAY_MAGIC_LENGTH );
    p += NPY_ARRAY_MAGIC_LENGTH;

    static char version[NPY_ARRAY_HEADER_LENGTH] = { 1, 0 };
    memcpy( p, version, NPY_ARRAY_HEADER_LENGTH );
    p += NPY_ARRAY_HEADER_LENGTH;

    char dict[NPY_ARRAY_DICT_BUFSIZE] = { '\0' };
    char shape[NPY_ARRAY_SHAPE_BUFSIZE] = { '\0' };
    char *ptr = shape;

    for( int i = 0; i < m->ndim; i++)
        ptr += sprintf(ptr, "%d,", (int) m->shape[i]);
    assert( ptr - shape < NPY_ARRAY_SHAPE_BUFSIZE );

    /* Potential bug? There are some additional whitespaces after the dictionaries saved from
     * Python/Numpy. Those are not documented? I have tested that this dictionary actually works */

    /* Yes. It seems to work without all the whitespaces, however it does say:

       It is terminated by a newline (\n) and padded with spaces (\x20) to make the total of:
       len(magic string) + 2 + len(length) + HEADER_LEN be evenly divisible by 64 for alignment purposes.

       ... which sounds smart enough for me, and with some reverse engineering it looks like HEADER_LEN is 108.

     */
#define HEADER_LEN 108
    /* WARNING: This code looks inocent and simple, but it was really a struggle. Do not touch unless you like pain! */
    size_t len = sprintf(dict, "{'descr': '%c%c%zu', 'fortran_order': %s, 'shape': (%s), }",
            _guess_endianness( m ),
            m->typechar,
            m->elem_size,
            m->fortran_order ? "True": "False",
            shape );

    assert( len < HEADER_LEN ); /* FIXME: This can go wrong for really big arrays with a lot of dimensions */
    len += sprintf( dict + len, "%*s\n", (int) (HEADER_LEN - len + NPY_ARRAY_PREHEADER_LENGTH - 1), " " );

    const uint16_t _len = (uint16_t) (len);
    memcpy( p, &_len, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy( p, dict, len);

    return len + NPY_ARRAY_PREHEADER_LENGTH;
#undef HEADER_LEN
}

size_t npy_array_calculate_datasize( const npy_array_t *m )
{
    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 && (idx < m->ndim) )
        n_elements *= m->shape[ idx++ ];
    return n_elements * m->elem_size;
}


static char *find_header_item( const char *item, const char *header)
{
    char *s = strstr(header, item);
    return s ? s + strlen(item) : NULL;
}

static inline char endianness(){
    int val = 1;
    return (*(char *)&val == 1) ? '<' : '>';
}

/* consider if this function should be exported to the end user */
npy_array_t * _read_matrix( void *fp, reader_func read_func )
{
    char fixed_header[NPY_ARRAY_PREHEADER_LENGTH + 1];
    size_t chk = read_func( fp, fixed_header, NPY_ARRAY_PREHEADER_LENGTH );
    if( chk != NPY_ARRAY_PREHEADER_LENGTH ){
        fprintf(stderr, "Cannot read pre header bytes.\n");
        return NULL;
    }
    for( int i = 0; i < NPY_ARRAY_MAGIC_LENGTH; i++ ){
        static char magic[] = NPY_ARRAY_MAGIC_STRING;
        if( magic[i] != fixed_header[i] ){
            fprintf(stderr,"File format not recognised as numpy array.\n");
            return NULL;
        }
    }
    char major_version = fixed_header[NPY_ARRAY_MAJOR_VERSION_IDX];
    char minor_version = fixed_header[NPY_ARRAY_MINOR_VERSION_IDX];

    if(major_version != 1){
        fprintf(stderr,"Wrong numpy save version. Expected version 1.x This is version %d.%d\n", (int)major_version, (int)minor_version);
        return NULL;
    }

    /* FIXME! This may fail for version 2 and it may also fail on big endian systems.... */
    uint16_t header_length = 0;
    header_length |= fixed_header[NPY_ARRAY_HEADER_LENGTH_LOW_IDX];
    header_length |= fixed_header[NPY_ARRAY_HEADER_LENGTH_HIGH_IDX] << 8;   /* Is a byte always 8 bit? */
    
    char header[header_length + 1];
    chk = read_func( fp, header, header_length );
    if( chk != header_length){
        fprintf(stderr, "Cannot read header. %d bytes.\n", header_length);
        return NULL;
    }
    header[header_length] = '\0';
#if VERBOSE
    printf("Header length: %d\nHeader dictionary: \"%s\"\n", header_length, header);
#endif

    npy_array_t *m = calloc( 1, sizeof *m );
    if ( !m ){
        fprintf(stderr, "Cannot allocate memory dor matrix structure.\n");
        return NULL;
    }

    char *descr   = find_header_item("'descr': '", header);
    assert(descr);
    if ( strchr("<>|", descr[0] ) ){
        m->endianness = descr[0];
        if( descr[0] != '|' && ( descr[0] != endianness())){
            fprintf(stderr, "Warning: Endianess of system and file does not match.");
        }
    } else {
        fprintf(stderr,"Warning: Endianness not found.");
    }

    /* FIXME Potential bug: Is the typechar always one byte? */
    m->typechar = descr[1];

    /* FIXME: Check the **endptr (second argument which is still NULL here)*/
    m->elem_size = (size_t) strtoll( &descr[2], NULL, 10);
    assert( m->elem_size > 0 );

    /* FIXME: This only works if there is one and only one leading spaces. */
    char *fortran = find_header_item("'fortran_order': ", header);
    assert( fortran );

    if(strncmp(fortran, "True", 4) == 0 )
        m->fortran_order = true;
    else if(strncmp(fortran, "False", 5) == 0 )
        m->fortran_order = false;
    else
        fprintf(stderr, "Warning: No matrix order found, assuming fortran_order=False");

    /* FIXME: This only works if there is one and only one leading spaces. */
    char *shape   = find_header_item("'shape': ", header);
    assert(shape);
    while (*shape != ')' ) {
        if( !isdigit( (int) *shape ) ){
            shape++;
            continue;
        }
        m->shape[m->ndim] = strtol( shape, &shape, 10);
        m->ndim++;
        assert( m->ndim < NPY_ARRAY_MAX_DIMENSIONS );
    }

    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 )
        n_elements *= m->shape[ idx++ ];

#if VERBOSE
    printf("Number of elements: %llu\n", (unsigned long long) n_elements );
#endif
    m->map_addr = NULL;
    if( read_func == read_mapped ){
        map_handler_t *mh = (map_handler_t*) fp;
        m->data = mh->current_pos;
        m->map_addr = mh->start_pos;
        return m;
    }

    m->data = malloc( n_elements * m->elem_size );
    if ( !m->data ){
        fprintf(stderr, "Cannot allocate memory for matrix data.\n");
        free( m );
        return NULL;
    }

    chk = read_func( fp, m->data, m->elem_size * n_elements ); /* Can the multiplication overflow? */ 
    if( chk != m->elem_size * n_elements){
        fprintf(stderr, "Could not read all data.\n");
        free( m->data );
        free( m );
        return NULL;
    }
    return m;
}

npy_array_t * npy_array_load( const char *filename )
{
    FILE *fp = fopen(filename, "rb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for reading.\n", filename );
        perror("Error");
        return NULL;
    }

    npy_array_t *m = _read_matrix( fp, &read_file);
    if(!m) { fprintf(stderr, "Cannot read matrix.\n"); }

    fclose(fp);
    return m;
}

static int64_t read_mapped( void *map, void *buffer, uint64_t nbytes )
{
    map_handler_t *mh = (map_handler_t*) map;
    size_t offset = mh->current_pos - mh->start_pos;
    assert( mh->length >= offset );
    size_t bytes_left = mh->length - offset;
    size_t bytes_to_read = bytes_left > nbytes ? nbytes : bytes_left;
    memcpy( buffer, mh->current_pos, bytes_to_read );
    mh->current_pos += bytes_to_read;
    return (int64_t) nbytes;
}

npy_array_t * npy_array_mmap( const char *filename )
{
    int fd = -1;
    if( (fd = open( filename, O_RDONLY  )) == -1 ){
        perror( filename );
        return NULL;
    }

    off_t len = lseek( fd, 0, SEEK_END );

    char *data = mmap( 0, len, PROT_READ,MAP_SHARED, fd, 0);
    close( fd );
    if( data == MAP_FAILED ){
        perror("mmap failed!");
        return NULL;
    }

    map_handler_t mh = { .start_pos = (char*) data, .current_pos = (char*) data, .length = len };

    npy_array_t *m = _read_matrix( &mh, &read_mapped);
    if(!m) {
        fprintf(stderr, "Cannot read matrix.\n");
        munmap( data, len );
    }
    return m;
}

void npy_array_dump( const npy_array_t *m )
{
    if(!m){
        fprintf(stderr, "Warning: No matrix found. (%s)\n", __func__);
        return;
    }
    printf("Dimensions   : %d\n", m->ndim);
    printf("Shape        : ( ");
    for( int i = 0; i < m->ndim - 1; i++) printf("%d, ", (int) m->shape[i]);
    printf("%d )\n", (int) m->shape[m->ndim-1]);
    printf("Type         : '%c' ", m->typechar);
    printf("(%d bytes each element)\n", (int) m->elem_size);
    printf("Fortran order: %s\n", m->fortran_order ? "True" : "False" );
    return;
}

void npy_array_save( const char *filename, const npy_array_t *m )
{
    if( !m ){
        fprintf(stderr, "Warning: No matrix found. (%s)\n", __func__);
        return;
    }

    FILE *fp = fopen( filename, "wb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for writing.\n", filename );
        perror("Error");
        return;
    }

    char header[NPY_ARRAY_DICT_BUFSIZE + NPY_ARRAY_PREHEADER_LENGTH] = {'\0'};
    size_t hlen = npy_array_get_header( m, header );

    size_t chk = fwrite( header, 1, hlen, fp );
    if( chk != hlen){
        fprintf(stderr, "Could not write header data.\n");
    }

    size_t datasize = npy_array_calculate_datasize( m );
    chk = fwrite( m->data, 1, datasize, fp );
    if( chk != datasize){
        fprintf(stderr, "Could not write all data.\n");
    }
    fclose(fp);
}

void npy_array_free( npy_array_t *m )
{
    if( !m ){
        fprintf(stderr, "Warning: No matrix found. (%s)\n", __func__);
        return;
    }

    if( m->map_addr ){
        /* We need the length of the data mapped. We are mapping the whole file, so we have to
           recalculate the size.  */
        uint16_t header_length = 0;
        char *preheader = m->map_addr;
        header_length |= preheader[NPY_ARRAY_HEADER_LENGTH_LOW_IDX];
        header_length |= preheader[NPY_ARRAY_HEADER_LENGTH_HIGH_IDX] << 8;

        size_t len = NPY_ARRAY_PREHEADER_LENGTH + header_length + npy_array_calculate_datasize(m);
        munmap( m->map_addr, len );
    } else
        free( m->data );

    free( m );
}
