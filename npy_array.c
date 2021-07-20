#include "npy_array.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include <zip.h>

#define NPY_ARRAY_MAGIC_STRING {0x93,'N','U','M','P','Y'}
#define NPY_ARRAY_MAGIC_LENGTH 6
#define NPY_ARRAY_VERSION_HEADER_LENGTH 4
#define NPY_ARRAY_PREHEADER_LENGTH (NPY_ARRAY_MAGIC_LENGTH + NPY_ARRAY_VERSION_HEADER_LENGTH)
#define NPY_ARRAY_MAJOR_VERSION_IDX 6
#define NPY_ARRAY_MINOR_VERSION_IDX 7

#define NPY_ARRAY_HEADER_LENGTH 2
#define NPY_ARRAY_HEADER_LENGTH_LOW_IDX 8
#define NPY_ARRAY_HEADER_LENGTH_HIGH_IDX 9

#define NPY_ARRAY_SHAPE_BUFSIZE 512
#define NPY_ARRAY_DICT_BUFSIZE 1024

#define MAX_FILENAME_LEN 80

typedef int64_t (*reader_func)( void *fp, void *buffer, uint64_t nbytes );
static int64_t read_file( void *fp, void *buffer, uint64_t nbytes )
{
    return (int64_t) fread( buffer, 1, nbytes, (FILE *) fp );
}

static int64_t read_zip( void *fp, void *buffer, uint64_t nbytes )
{
    return (int64_t) zip_fread( (zip_file_t *) fp, buffer, nbytes );
}

static void _header_from_npy_array( const npy_array_t *m,  char *buf, size_t *hlen )
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
            m->endianness,
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

    *hlen = len + NPY_ARRAY_PREHEADER_LENGTH;
#undef HEADER_LEN
}

static size_t _calculate_datasize( const npy_array_t *m )
{
    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 && (idx < m->ndim) )
        n_elements *= m->shape[ idx++ ];
    return n_elements * m->elem_size;
}

#if 0
static uint32_t _crc32_from_npy_array( const npy_array_t *m, uint32_t *size )
{
    char header[NPY_ARRAY_DICT_BUFSIZE + NPY_ARRAY_PREHEADER_LENGTH] = {'\0'};

    size_t hlen = 0;
    _header_from_npy_array( m, header, &hlen );
    size_t datasize = _calculate_datasize( m );
    if(size) *size = hlen + datasize;
    uint32_t crc;
    crc = crc32( 0, header, hlen );
    return crc32( crc, m->data, datasize );
}
#endif

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

npy_array_list_t * npy_array_list_append( npy_array_list_t *list, npy_array_t *array, const char *filename, ... )
{
    /* First get the full filename */
    va_list ap1, ap2;
    int len;

    va_start( ap1, filename );
    va_copy( ap2, ap1 );
    len = vsnprintf( NULL, 0, filename, ap1 );
    va_end( ap1 );

    if( len > MAX_FILENAME_LEN ){
        /* If someone is trying to cook up a special filename that can contain executable code, I think it is wise
           to limit the length of the filename */
        fprintf( stderr, "Warning: Cannot save neural network. Your filename is too long."
                " Please limit the filename to %d characters.\n", MAX_FILENAME_LEN );
        return list;
    }

    char *real_filename = malloc( (len+1) * sizeof(char));
    assert( real_filename );

    vsprintf( real_filename, filename, ap2 );
    va_end( ap2 );

    /* And then we append...  But we have to find the last element first */
    npy_array_list_t *new_list;

    new_list = npy_array_list_new();
    if ( !new_list ) return list;
    new_list->array = array;
    new_list->filename = real_filename;
#if 0
    new_list->crc32 = _crc32_from_npy_array ( array, NULL );
#endif
    new_list->next = NULL;

    if(list) {
        npy_array_list_t *last = list;
        while(last->next)
            last = last->next;
        last->next = new_list;
        return list;
    }
    return new_list;
}

/* OMG: Unify some of the duplicate code in append/prepand functions. */
npy_array_list_t * npy_array_list_prepend( npy_array_list_t *list, npy_array_t *array, const char *filename, ... )
{
    /* First get the full filename */
    va_list ap1, ap2;
    int len;

    va_start( ap1, filename );
    va_copy( ap2, ap1 );
    len = vsnprintf( NULL, 0, filename, ap1 );
    va_end( ap1 );

    if( len > MAX_FILENAME_LEN ){
        /* If someone is trying to cook up a special filename that can contain executable code, I think it is wise
           to limit the length of the filename */
        fprintf( stderr, "Warning: Cannot save neural network. Your filename is too long."
                " Please limit the filename to %d characters.\n", MAX_FILENAME_LEN );
        return list;
    }

    char *real_filename = malloc( (len+1) * sizeof(char));
    assert( real_filename );

    vsprintf( real_filename, filename, ap2 );
    va_end( ap2 );

    npy_array_list_t *new_list;

    new_list = npy_array_list_new();
    if ( !new_list ) return list;
    new_list->array = array;
    new_list->filename = real_filename;
#if 0
    new_list->crc32 = _crc32_from_npy_array ( array, NULL );
#endif
    new_list->next = list;
    return new_list;
}

static char * _new_internal_filename( int n )
{
    char *filename = malloc( MAX_FILENAME_LEN * sizeof(char) );
    if( filename )
        sprintf( filename, "arr_%d.npy", n );
    return filename;
}

int npy_array_list_save( const char *filename, npy_array_list_t *array_list )
{
    if ( !array_list )
        return 0;

    zip_t *zip = zip_open( filename, ZIP_CREATE | ZIP_TRUNCATE, NULL );
    assert( zip );
#if 0
    FILE *fp = fopen( filename, "wb" );

    if ( !fp ){
        fprintf(stderr, "Cannot open file '%s' for writing\n", filename);
        return -1;
    }
#endif
    int n = 0;
    for( npy_array_list_t *iter = array_list; iter; iter = iter->next ){
        /* if the filename is not set, set one. However.. if the list was created with append and prepend,
         * this will never be true. */
        if(!iter->filename)
            iter->filename = _new_internal_filename( n );

        char header[NPY_ARRAY_DICT_BUFSIZE + NPY_ARRAY_PREHEADER_LENGTH] = {'\0'};

        npy_array_t *m = iter->array;

        size_t hlen = 0;
        _header_from_npy_array( m, header, &hlen );
        size_t datasize = _calculate_datasize( m );

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
#if 0


        /* Find the time */
        time_t now = time(NULL);
        dostime_t dt_now = unix2dostime( now );

        uint32_t size = 0;
        uint32_t crc32 = _crc32_from_npy_array( iter->array, &size );

        local_file_header_t lfh = {
            .local_file_header_signature = LOCAL_HEADER_SIGNATURE,                   /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
            .version_needed_to_extract   = 20,                           /*  2 bytes */
            .last_mod_file_time          = (uint16_t) (dt_now & 0xffff), /*  2 bytes */
            .last_mod_file_date          = (uint16_t) (dt_now >> 16),    /*  2 bytes */
            .crc_32                      = crc32,                        /*  4 bytes */
            .compressed_size             = size,                         /*  4 bytes */
            .uncompressed_size           = size,                         /*  4 bytes */
            .file_name_length            = strlen(iter->filename),       /*  2 bytes */
        };
        lfh.file_name = iter->filename;
        _write_local_fileheader( fp, &lfh );
        _write_matrix          ( fp, iter->array );
        n++;
    }

    uint32_t offset_count = 0;
    uint32_t total_namelength = 0;
    for( npy_array_list_t *iter = array_list; iter; iter = iter->next ){
        /* central directory */

        /* Find the time */
        /* OK ... the time might be different in CD than in local, but I guess that's not a problem? */
        time_t now = time(NULL);
        dostime_t dt_now = unix2dostime( now );

        uint32_t size;
        uint32_t crc32 = _crc32_from_npy_array( iter->array, &size );
        central_directory_header_t cdh = {
            .central_file_header_signature = CENTRAL_DIRECTORY_HEADER_SIGNATURE, /* 4 bytes */
            .version_made_by               = 788,        /* My python uses this. Should I use something else? */ /* 2 bytes */
            .version_needed_to_extract     = 20,         /* 2 bytes */
            .last_mod_file_time            = (uint16_t) (dt_now & 0xffff), /*  2 bytes */
            .last_mod_file_date            = (uint16_t) (dt_now >> 16),    /*  2 bytes */
            .crc_32                        = crc32,                        /*  4 bytes */
            .compressed_size               = size,                         /*  4 bytes */
            .uncompressed_size             = size,                         /*  4 bytes */
            .external_file_attributes      = 0x1a40000,                    /*  Unix: -rw-r--r-- (644) */
            .file_name_length              = strlen(iter->filename),       /*  2 bytes */
            .relative_offset_of_local_header = offset_count                /* 4 bytes */
        };
        cdh.file_name = iter->filename;

        _write_cental_directory_fileheader( fp, &cdh );
        offset_count += size + cdh.file_name_length + LOCAL_HEADER_LENGTH;
        total_namelength += cdh.file_name_length;
    }

    end_of_central_dir_t eocd = {
        .end_of_central_dir_signature    = END_OF_CENTRAL_DIR_SIGNATURE,       /*  4 bytes (END_OF_CENTRAL_DIR_SIGNATURE) */
        .total_num_entries_this_disk     = (uint16_t) n,     /*  2 bytes */
        .total_num_entries_cd            = (uint16_t) n,     /*  2 bytes */
        .size_of_cd                      = (uint32_t) n * CENTRAL_DIRECTORY_HEADER_LENGTH + total_namelength,  /*  4 bytes */
        .offset_cd_wrt_disknum           = offset_count     /*  4 bytes */
    };

    _write_end_of_central_dir( fp, &eocd );
    fclose( fp );
    return n;
#endif
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

static inline bool is_encypted( uint16_t flag )
{
    return flag & (uint16_t) 0x1;
}

static npy_array_t * _read_matrix( void *fp, reader_func read_func )
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

#if 0
    if(descr[0] == '<') printf("Little Endian\n");
    if(descr[0] == '>') printf("Big Endian (Be carefull)\n");
    if(descr[0] == '|') printf("Not relevant endianess\n");

    if(descr[1] == 'f') printf("float number\n");
    if(descr[1] == 'i') printf("integer number\n");

    printf("each item is %d bytes.\n", (int) m->elem_size );
#endif

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

npy_array_list_t * npy_array_list_load( const char *filename )
{
    /* FIXME: Check error */
    zip_t *zip = zip_open(filename, ZIP_RDONLY, NULL );

#if 0
    FILE *fp = fopen(filename, "rb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for reading.\n", filename );
        perror("Error");
        return NULL;
    }
#endif
    npy_array_list_t *list = NULL;
#if 0
    /* Check that is is a PKZIP file (which happens to be the same as .npz format  */
    char check[5] = { '\0' };
    if( 4 != fread( check, 1, 4, fp )){
        fprintf(stderr, "Warning: cannot read from '%s'.\n", filename );
        fclose( fp );
        return NULL;
    }

    if( check[0] != 'P' || check[1] != 'K' ){
        fclose(fp);   /* Failing silently is intentional. caller should handle this. */
        return NULL;
    }
    fseek( fp, -4, SEEK_CUR );
    /* The check ends here */

    while ( true ){
        local_file_header_t *lh;
        if( NULL == ( lh = local_file_header_new_from_fp( fp )))
            break;

        /* FIXME: Support for compressed files */
        if( lh->compression_method != 0 ){
            fprintf(stderr, "local file '%s' is compressed. Skipping.\n", lh->file_name);
            fprintf(stderr, "Still no support for reading compressed files.\n"
                    "Please store numpy array with np.savez() instead of np.savez_compressed().\n");
            continue;
        }

        /* FIXME: Support for encrypted files */
        if( is_encypted( lh->general_purpose_bit_flag )) {
            fprintf(stderr, "local file '%s' is encrypted. Skipping.\n", lh->file_name);
            continue;
        }

        if( lh->general_purpose_bit_flag & (uint16_t) 0x4 ){
            fprintf(stderr, "No support for streamed datafiles with data descriptor.\n");
            /* fread( ... ); */
            continue;
        }

        /* done reading header - read the matrix */
        npy_array_t *a = _read_matrix( fp, &read_file );
        if( !a ){
            fprintf(stderr, "Cannot read matrix.\n");
            continue;
        }

        /* OH! lh->file_name is not NULL terminated, so we need some good ol' C string handling.... */
        char internal_filename[MAX_FILENAME_LEN+1];
        memset( internal_filename, 0, MAX_FILENAME_LEN+1 );
        for( int i = 0; i < lh->file_name_length; i++)
            internal_filename[i] = lh->file_name[i];

        list = npy_array_list_append( list, a, internal_filename );
        local_file_header_free( lh );
    }

    /* Here I could read the Central directory items, however I really do not care as
     * I already have all the data I need. Fuck the central directory!   */
    fclose( fp );
#endif
    for( int i = 0; i < zip_get_num_entries( zip, 0 ); i++ ){
        zip_file_t *fp = zip_fopen_index( zip, i, 0 ); /* FIXME Some checks */
        npy_array_t *arr = _read_matrix( fp, &read_zip );
        list = npy_array_list_append( list, arr, zip_get_name( zip, i, 0 ));
        zip_fclose( fp );
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
    size_t hlen = 0;
    _header_from_npy_array( m, header, &hlen );

    size_t chk = fwrite( header, 1, hlen, fp );
    if( chk != hlen){
        fprintf(stderr, "Could not write header data.\n");
    }

    size_t datasize = _calculate_datasize( m );
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

    free( m->data );
    free( m );
}
