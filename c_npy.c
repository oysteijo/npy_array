#include "c_npy.h"
#include "zipcontainer.h"
#include "crc32.h"
#include "dostime.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define C_NPY_MAGIC_STRING {0x93,'N','U','M','P','Y'}
#define C_NPY_MAGIC_LENGTH 6
#define C_NPY_VERSION_HEADER_LENGTH 4
#define C_NPY_PREHEADER_LENGTH (C_NPY_MAGIC_LENGTH + C_NPY_VERSION_HEADER_LENGTH)
#define C_NPY_MAJOR_VERSION_IDX 6
#define C_NPY_MINOR_VERSION_IDX 7

#define C_NPY_HEADER_LENGTH 2
#define C_NPY_HEADER_LENGTH_LOW_IDX 8
#define C_NPY_HEADER_LENGTH_HIGH_IDX 9

#define C_NPY_SHAPE_BUFSIZE 512
#define C_NPY_DICT_BUFSIZE 1024

/*
typedef struct _cmatrix_t {
    char    *data;
    size_t   shape[ C_NPY_MAX_DIMENSIONS ];
    int32_t  ndim;
    char     endianness;
    char     typechar;
    size_t   elem_size;
    bool     fortran_order;
} cmatrix_t;
*/

static void _header_from_cmatrix( const cmatrix_t *m,  char *buf, size_t *hlen )
{
    char *p = buf;

    static char magic[] = C_NPY_MAGIC_STRING;
    memcpy( p, magic, C_NPY_MAGIC_LENGTH );
    p += C_NPY_MAGIC_LENGTH;

    static char version[C_NPY_HEADER_LENGTH] = { 1, 0 };
    memcpy( p, version, C_NPY_HEADER_LENGTH );
    p += C_NPY_HEADER_LENGTH;

    char dict[C_NPY_DICT_BUFSIZE] = { '\0' };
    char shape[C_NPY_SHAPE_BUFSIZE] = { '\0' };
    char *ptr = shape;

    for( int i = 0; i < m->ndim; i++)
        ptr += sprintf(ptr, "%d,", (int) m->shape[i]);
    assert( ptr - shape < C_NPY_SHAPE_BUFSIZE );


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
    len += sprintf( dict + len, "%*s\n", (int) (HEADER_LEN - len + C_NPY_PREHEADER_LENGTH - 1), " " );

    const uint16_t _len = (uint16_t) (len);
    memcpy( p, &_len, sizeof(uint16_t));
    p += sizeof(uint16_t);
    memcpy( p, dict, len);

    *hlen = len + C_NPY_PREHEADER_LENGTH;
#undef HEADER_LEN
}

static size_t _calculate_datasize( const cmatrix_t *m )
{
    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 && (idx < m->ndim) )
        n_elements *= m->shape[ idx++ ];
    return n_elements * m->elem_size;
}

static uint32_t _crc32_from_cmatrix( const cmatrix_t *m, uint32_t *size )
{
    char header[C_NPY_DICT_BUFSIZE + C_NPY_PREHEADER_LENGTH] = {'\0'};

    size_t hlen = 0;
    _header_from_cmatrix( m, header, &hlen );
    size_t datasize = _calculate_datasize( m );
    if(size) *size = hlen + datasize;
    uint32_t crc;
    crc = crc32( 0, header, hlen );
    return crc32( crc, m->data, datasize );
}

/* FIXME: tell caller about the fails */
static void _write_matrix( FILE *fp, const cmatrix_t *m )
{

    char header[C_NPY_DICT_BUFSIZE + C_NPY_PREHEADER_LENGTH] = {'\0'};

    size_t hlen = 0;
    _header_from_cmatrix( m, header, &hlen );

    size_t chk = fwrite( header, sizeof(char), hlen, fp );
    if( chk != hlen){
        fprintf(stderr, "Could not write header data.\n");
    }

    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 )
        n_elements *= m->shape[ idx++ ];

    chk = fwrite( m->data, m->elem_size, n_elements, fp );
    if( chk != n_elements){
        fprintf(stderr, "Could not write all data.\n");
    }
    return;
}

int c_npy_matrix_array_write( const char *filename, const cmatrix_t **array )
{
    FILE *fp = fopen( filename, "wb" );
    if ( !fp ){
        fprintf(stderr, "Cannot open file '%s' for writing\n", filename);
        return -1;
    }

    int n = (int) c_npy_matrix_array_length( array );

    for ( int i = 0; i < n; i++ ){
        /* Set the name of the file */
        char arrname[42] = {'\0' };
        sprintf( arrname, "arr_%d", i );

        /* Find the time */
        time_t now = time(NULL);
        dostime_t dt_now = unix2dostime( now );

        uint32_t size = 0;
        uint32_t crc32 = _crc32_from_cmatrix( array[i], &size );

        local_file_header_t lfh = {
            .local_file_header_signature = LOCAL_HEADER_SIGNATURE,                   /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
            .version_needed_to_extract   = 20,                           /*  2 bytes */
            .last_mod_file_time          = (uint16_t) (dt_now & 0xffff), /*  2 bytes */
            .last_mod_file_date          = (uint16_t) (dt_now >> 16),    /*  2 bytes */
            .crc_32                      = crc32,                        /*  4 bytes */
            .compressed_size             = size,                         /*  4 bytes */
            .uncompressed_size           = size,                         /*  4 bytes */
            .file_name_length            = strlen(arrname),              /*  2 bytes */
            .file_name                   = arrname                       /*  (variable size) */
        };
        _write_local_fileheader( fp, &lfh );
        _write_matrix          ( fp, array[i] );
    }

    uint32_t offset_count = 0;
    uint32_t total_namelength = 0;
    for ( int i = 0; i < n; i++ ){
        /* central directory */
        char arrname[42] = {'\0' };
        sprintf( arrname, "arr_%d", i );

        /* Find the time */
        /* OK ... the time might be different in CD than in local, but I guess that's not a problem? */
        time_t now = time(NULL);
        dostime_t dt_now = unix2dostime( now );

        uint32_t size;
        uint32_t crc32 = _crc32_from_cmatrix( array[i], &size );
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
            .file_name_length              = strlen(arrname),              /*  2 bytes */
            .file_name                     = arrname,                      /*  (variable size) */
            .relative_offset_of_local_header = offset_count                /* 4 bytes */
        };

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
    return n;
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

static cmatrix_t * _read_matrix( FILE *fp )
{
    char fixed_header[C_NPY_PREHEADER_LENGTH + 1];
    size_t chk = fread( fixed_header, sizeof(char), C_NPY_PREHEADER_LENGTH, fp );
    if( chk != C_NPY_PREHEADER_LENGTH ){
        fprintf(stderr, "Cannot read pre header bytes.\n");
        return NULL;
    }
    for( int i = 0; i < C_NPY_MAGIC_LENGTH; i++ ){
        static char magic[] = C_NPY_MAGIC_STRING;
        if( magic[i] != fixed_header[i] ){
            fprintf(stderr,"File format not recognised as numpy array.\n");
            return NULL;
        }
    }
    char major_version = fixed_header[C_NPY_MAJOR_VERSION_IDX];
    char minor_version = fixed_header[C_NPY_MINOR_VERSION_IDX];

    if(major_version != 1){
        fprintf(stderr,"Wrong numpy save version. Expected version 1.x This is version %d.%d\n", (int)major_version, (int)minor_version);
        return NULL;
    }

    /* FIXME! This may fail for version 2 and it may also fail on big endian systems.... */
    uint16_t header_length = 0;
    header_length |= fixed_header[C_NPY_HEADER_LENGTH_LOW_IDX];
    header_length |= fixed_header[C_NPY_HEADER_LENGTH_HIGH_IDX] << 8;   /* Is a byte always 8 bit? */
    
    char header[header_length + 1];
    chk = fread( header, sizeof(char), header_length, fp );
    if( chk != header_length){
        fprintf(stderr, "Cannot read header. %d bytes.\n", header_length);
        return NULL;
    }
    header[header_length] = '\0';
#if VERBOSE
    printf("Header length: %d\nHeader dictionary: \"%s\"\n", header_length, header);
#endif

    cmatrix_t *m = calloc( 1, sizeof *m );
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

#if VERBOSE
    if(descr[0] == '<') printf("Little Endian\n");
    if(descr[0] == '>') printf("Big Endian (Be carefull)\n");
    if(descr[0] == '|') printf("Not relevant endianess\n");

    if(descr[1] == 'f') printf("float number\n");
    if(descr[1] == 'i') printf("integer number\n");

    printf("each item is %d bytes.\n", (int) m->elem_size );
#endif

    char *fortran = find_header_item("'fortran_order': ", header);
    assert( fortran );

    if(strncmp(fortran, "True", 4) == 0 )
        m->fortran_order = true;
    else if(strncmp(fortran, "False", 5) == 0 )
        m->fortran_order = false;
    else
        fprintf(stderr, "Warning: No matrix order found, assuming fortran_order=False");


    char *shape   = find_header_item("'shape': ", header);
    assert(shape);
    while (*shape != ')' ) {
        if( !isdigit( (int) *shape ) ){
            shape++;
            continue;
        }
        m->shape[m->ndim] = strtol( shape, &shape, 10);
        m->ndim++;
        assert( m->ndim < C_NPY_MAX_DIMENSIONS );
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

    chk = fread( m->data, m->elem_size, n_elements, fp );
    if( chk != n_elements){
        fprintf(stderr, "Could not read all data.\n");
        free( m->data );
        free( m );
        return NULL;
    }

    return m;
}

cmatrix_t * c_npy_matrix_read_file( const char *filename )
{
    FILE *fp = fopen(filename, "rb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for reading.\n", filename );
        perror("Error");
        return NULL;
    }

    cmatrix_t *m = _read_matrix( fp );
    if(!m) { fprintf(stderr, "Cannot read matrix.\n"); }

    fclose(fp);
    return m;
}

#define _MAX_ARRAY_LENGTH 128
cmatrix_t ** c_npy_matrix_array_read( const char *filename )
{
    FILE *fp = fopen(filename, "rb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for reading.\n", filename );
        perror("Error");
        return NULL;
    }

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
    
    cmatrix_t *_array[_MAX_ARRAY_LENGTH] = {NULL};
    int count = 0;

    while ( true ){

        local_file_header_t lh;
        _read_local_fileheader( fp, &lh );
        if (count == 2 ) break;
#if VERBOSE
        printf("HEADER: %d\n", count);
        _dump_local_fileheader( &lh );
#endif
        free( lh.file_name );    /* Oh!  Barf... */
        free( lh.extra_field );
        /* FIXME: Support for compressed files */
        if( lh.compression_method != 0 ){
            fprintf(stderr, "local file '%s' is compressed. Skipping.\n", lh.file_name);
            fprintf(stderr, "Still no support for reading compressed files.\n"
                    "Please store numpy array with np.savez() instead of np.savez_compressed().\n");
            continue;
        }

        /* FIXME: Support for encrypted files */
        if( is_encypted( lh.general_purpose_bit_flag )) {
            fprintf(stderr, "local file '%s' is encrypted. Skipping.\n", lh.file_name);
            continue;
        }

        if( lh.general_purpose_bit_flag & (uint16_t) 0x4 ){
            fprintf(stderr, "No support for streamed datafiles with data descriptor.\n");
            /* fread( ... ); */
            continue;
        }

        /* done reading header - read the matrix */
        if( NULL == (_array[count] = _read_matrix( fp ))){
            fprintf(stderr, "Cannot read matrix.\n");
            continue;
        }

        count++;
        assert( count < _MAX_ARRAY_LENGTH );
    }
#if I_REALLY_DONT_CARE_ABOUT_THIS_SINCE_IVE_ALREADY_READ_ALL_THE_DATA_I_NEED
    /* FIXME: Read all the central directory */
    central_directory_header_t cdh;
    /* OK reading a local header failed, so we have to go back some bytes */
    fseek( fp, -LOCAL_HEADER_LENGTH, SEEK_CUR );
    for (int i = 0;  i < count; i++ ){
        _read_central_directory_header( fp, &cdh );
        printf("=== Central directory header %d ===\n", i);
        _central_directory_header_dump( &cdh );
#if 1
        free( cdh.file_name );
        free( cdh.file_comment );
        free( cdh.extra_field );    
#endif
    }
    end_of_central_dir_t eocd = {0};
    _read_end_of_central_dir( fp, &eocd );
    _dump_end_of_central_dir( &eocd );
    free( eocd->ZIP_file_comment );
#endif  /* I_REALLY_DONT_CARE_ABOUT_THIS_SINCE_IVE_ALREADY_READ_ALL_THE_DATA_I_NEED */
    /* size_t len = c_npy_matrix_array_length( _array ); */
    size_t len = count;
    if( len == 0 ){
        /* What? Maybe this is not a a zip file */
        fclose(fp);
        return NULL;
    }

    cmatrix_t** retarray = calloc( len, sizeof *retarray);
    if( !retarray ){
        fprintf( stderr, "Cannot allocate memory for array of matrices.\n");
        for( unsigned int i = 0; i < len; i++)
            c_npy_matrix_free( _array[i] );
        fclose( fp );
        return NULL;
    }
    /* Copy the pointers */
    for( unsigned int i = 0; i < len; i++ )
        retarray[i] = _array[i];

    /* Here we put in a lot of checks! CRC32 among the tests */

    fclose(fp);
    return retarray;
}


size_t c_npy_matrix_array_length( const cmatrix_t **arr)
{
    if (!arr) return 0;
    size_t len = 0;
    for( len = 0; len < _MAX_ARRAY_LENGTH && arr[len]; len++ )
        ;
    return len;
}

void c_npy_matrix_array_free( cmatrix_t **arr )
{
    size_t len = c_npy_matrix_array_length( (const cmatrix_t**) arr );

    for( unsigned int i = 0; i < len; i++)
        c_npy_matrix_free( arr[i] );
    free( arr );
}

void c_npy_matrix_dump( const cmatrix_t *m )
{
    if(!m){
        fprintf(stderr, "Warning: No matrix found. (%s)\n", __func__);
        return;
    }
    printf("Dimensions   : %d\n", m->ndim);
    printf("Shape        : ( ");
    for( int i = 0; i < m->ndim - 1; i++) printf("%d, ", (int) m->shape[i]);
    printf("%d)\n", (int) m->shape[m->ndim-1]);
    printf("Type         : '%c' ", m->typechar);
    printf("(%d bytes each element)\n", (int) m->elem_size);
    printf("Fortran order: %s\n", m->fortran_order ? "True" : "False" );
    return;
}


void c_npy_matrix_write_file( const char *filename, const cmatrix_t *m )
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
    _write_matrix( fp, m );
    fclose(fp);
}

void c_npy_matrix_free( cmatrix_t *m )
{
    if( !m ){
        fprintf(stderr, "Warning: No matrix found. (%s)\n", __func__);
        return;
    }

    free( m->data );
    free( m );
}
