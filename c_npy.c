#include "c_npy.h"
#include "zipcontainer.h"

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

static cmatrix_t * _read_matrix( FILE *fp );

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

    /* FIXME: Check if this is a zipped file */

    cmatrix_t *_array[_MAX_ARRAY_LENGTH] = {NULL};
    int count = 0;

    while ( true ){
        char header[LOCAL_HEADER_LENGTH];
        size_t chk = fread( header, 1, LOCAL_HEADER_LENGTH, fp ); /* FIXME */
        if (chk != LOCAL_HEADER_LENGTH ){
            fprintf(stderr, "Cannot read header.\n");
            break;
        }
        if (*(uint32_t*)(header) != 0x04034B50 )
            break;

        local_file_header_t lh;
        /* We cannot assume the structure is "packed" we therefore assign one and one element */
        lh.local_file_header_signature = *(uint32_t*)(header);      /*  4 bytes */    
        lh.version_needed_to_extract   = *(uint16_t*)(header+4);    /*  2 bytes */    
        lh.general_purpose_bit_flag    = *(uint16_t*)(header+6);    /*  2 bytes */    
        lh.compression_method          = *(uint16_t*)(header+8);    /*  2 bytes */
        lh.last_mod_file_time          = *(uint16_t*)(header+10);   /*  2 bytes */
        lh.last_mod_file_date          = *(uint16_t*)(header+12);   /*  2 bytes */
        lh.crc_32                      = *(uint32_t*)(header+14);   /*  4 bytes */
        lh.compressed_size             = *(uint32_t*)(header+18);   /*  4 bytes */
        lh.uncompressed_size           = *(uint32_t*)(header+22);   /*  4 bytes */
        lh.file_name_length            = *(uint16_t*)(header+26);   /*  2 bytes */
        lh.extra_field_length          = *(uint16_t*)(header+28);   /*  2 bytes */

        char local_file_name[lh.file_name_length+1];
        lh.file_name = local_file_name;
        chk = fread( lh.file_name, sizeof(char), lh.file_name_length, fp );
        if( chk != lh.file_name_length )
            fprintf(stderr, "Trouble...\n");
        local_file_name[lh.file_name_length] = '\0';
        char local_extra_field[lh.extra_field_length+1];
        lh.extra_field = local_extra_field;
        chk = fread( lh.extra_field, sizeof(char), lh.extra_field_length, fp );
        if( chk != lh.extra_field_length )
            fprintf(stderr, "More trouble...\n");
        local_extra_field[lh.extra_field_length] = '\0';

        /*  Done reading header... are we ready? */
#if VERBOSE
        printf("HEADER: %d\n", count);
        printf("lh.local_file_header_signature: %d\n", lh.local_file_header_signature);
        printf("lh.version_needed_to_extract:   %d\n", lh.version_needed_to_extract);
        printf("lh.general_purpose_bit_flag:    %d\n", lh.general_purpose_bit_flag );
        printf("lh.compression_method:          %d\n", lh.compression_method);
        printf("lh.last_mod_file_time:          %d\n", lh.last_mod_file_time);
        printf("lh.last_mod_file_date:          %d\n", lh.last_mod_file_date);
        printf("lh.crc_32:                      %d\n", lh.crc_32);
        printf("lh.compressed_size:             %d\n", lh.compressed_size);
        printf("lh.uncompressed_size:           %d\n", lh.uncompressed_size);
        printf("lh.file_name_length:            %d\n", lh.file_name_length);
        printf("lh.extra_field_length:          %d\n", lh.extra_field_length);

        printf("Filename: %s\n", lh.file_name );
        printf("Extra field: %s\n", lh.extra_field );
#endif
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

    /* FIXME: Read all the central directory */

    /* size_t len = c_npy_matrix_array_length( _array ); */
    size_t len = count;
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

size_t c_npy_matrix_array_length( cmatrix_t **arr)
{
    size_t len = 0;
    for( len = 0; len < _MAX_ARRAY_LENGTH && arr[len]; len++ )
        ;
    return len;
}

void c_npy_matrix_array_free( cmatrix_t **arr )
{
    size_t len = c_npy_matrix_array_length( arr );
    for( unsigned int i = 0; i < len; i++)
        c_npy_matrix_free( arr[i] );
    free( arr );
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
    /* FIXME Potential bug: Is the elem_size always one digit only? */
    m->elem_size = descr[2] - '0';
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

    static char magic[] = C_NPY_MAGIC_STRING;
    size_t chk = fwrite( magic, sizeof(char), C_NPY_MAGIC_LENGTH, fp );
    if( chk != C_NPY_MAGIC_LENGTH ){
        fprintf(stderr, "Cannot write magic.\n");
        fclose(fp);
        return;
    }

    char version[C_NPY_HEADER_LENGTH] = { 1, 0 };
    chk = fwrite( version, sizeof(char), C_NPY_HEADER_LENGTH, fp );
    if( chk != C_NPY_HEADER_LENGTH ){
        fprintf(stderr, "Cannot write version.\n");
        fclose(fp);
        return;
    }

    char dict[C_NPY_DICT_BUFSIZE] = { 0 };
    char shape[C_NPY_SHAPE_BUFSIZE] = { 0 };
    char *ptr = shape;

    for( int i = 0; i < m->ndim - 1; i++)
        ptr += sprintf(ptr, "%d, ", (int) m->shape[i]);
    ptr += sprintf( ptr, "%d", (int) m->shape[m->ndim-1] );
    assert( ptr - shape < C_NPY_SHAPE_BUFSIZE );

    /* Potential bug? There are some additional whitespaces after the dictionaries saved from
     * Python/Numpy. Those are not documented? I have tested that this dictionary actually works */
    size_t len = sprintf(dict, "{'descr': '%c%c%c', 'fortran_order': %s, 'shape': (%s), }",
            m->endianness,
            m->typechar,
            (char) m->elem_size + '0',
            m->fortran_order ? "True": "False",
            shape);

    assert( len < C_NPY_DICT_BUFSIZE );
    uint16_t len_short = (uint16_t) len;
    chk = fwrite( &len_short, sizeof(uint16_t), 1, fp);
    if( chk != 1 ){
        fprintf(stderr, "Cannot write header size.\n");
        fclose(fp);
        return;
    }

    chk = fwrite( dict, sizeof(char), len, fp);
    if( chk != len ){
        fprintf(stderr, "Cannot write header.\n");
        fclose(fp);
        return;
    }

    size_t n_elements = 1;
    int idx = 0;
    while ( m->shape[ idx ] > 0 )
        n_elements *= m->shape[ idx++ ];

    chk = fwrite( m->data, m->elem_size, n_elements, fp );
    if( chk != n_elements){
        fprintf(stderr, "Could not write all data.\n");
    }

    fclose(fp);
    return;
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
