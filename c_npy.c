#include "c_npy.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define C_NPY_MAX_DIMENSIONS 8
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
    size_t   itemsize;
    bool     fortran_order;
} cmatrix_t;
*/

static char *find_header_item( const char *item, const char *header)
{
    char *s = strstr(header, item);
    return s ? s + strlen(item) : NULL;
}

static inline char endianness(){
    int val = 1;
    return (*(char *)&val == 1) ? '<' : '>';
}

cmatrix_t * c_npy_read_from_file( const char *filename )
{
    FILE *fp = fopen(filename, "rb");
    if( !fp ){
        fprintf(stderr,"Cannot open '%s' for reading.\n", filename );
        perror("Error");
        return NULL;
    }

    char fixed_header[C_NPY_PREHEADER_LENGTH + 1];
    size_t chk = fread( fixed_header, sizeof(char), C_NPY_PREHEADER_LENGTH, fp );
    if( chk != C_NPY_PREHEADER_LENGTH ){
        fprintf(stderr, "Cannot read pre header bytes.\n");
        fclose(fp);
        return NULL;
    }
    for( int i = 0; i < C_NPY_MAGIC_LENGTH; i++ ){
        static char magic[] = C_NPY_MAGIC_STRING;
        if( magic[i] != fixed_header[i] ){
            fprintf(stderr,"File format not recognised as numpy array.\n");
            fclose(fp);
            return NULL;
        }
    }
    char major_version = fixed_header[C_NPY_MAJOR_VERSION_IDX];
    char minor_version = fixed_header[C_NPY_MINOR_VERSION_IDX];

    if(major_version != 1){
        fprintf(stderr,"Wrong numpy save version. Expected version 1.x This is version %d.%d\n", (int)major_version, (int)minor_version);
        fclose(fp);
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
        fclose(fp);
        return NULL;
    }
    header[header_length] = '\0';
#if VERBOSE
    printf("Header length: %d\nHeader dictionary: \"%s\"\n", header_length, header);
#endif

    cmatrix_t *m = calloc( 1, sizeof *m );
    if ( !m ){
        fprintf(stderr, "Cannot allocate memory dor matrix structure.\n");
        fclose(fp);
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
    /* FIXME Potential bug: Is the itemsize always one digit only? */
    m->itemsize = descr[2] - '0';
    assert( m->itemsize > 0 );

#if VERBOSE
    if(descr[0] == '<') printf("Little Endian\n");
    if(descr[0] == '>') printf("Big Endian (Be carefull)\n");
    if(descr[0] == '|') printf("Not relevant endianess\n");

    if(descr[1] == 'f') printf("float number\n");
    if(descr[1] == 'i') printf("integer number\n");

    printf("each item is %d bytes.\n", (int) m->itemsize );
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

    m->data = malloc( n_elements * m->itemsize );
    if ( !m->data ){
        fprintf(stderr, "Cannot allocate memory for matrix data.\n");
        free( m );
        fclose( fp );
        return NULL;
    }

    chk = fread( m->data, m->itemsize, n_elements, fp );
    if( chk != n_elements){
        fprintf(stderr, "Could not read all data.\n");
        free( m->data );
        free( m );
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return m;
}

void c_npy_dump( const cmatrix_t *m )
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
    printf("(%d bytes each element)\n", (int) m->itemsize);
    printf("Fortran order: %s\n", m->fortran_order ? "True" : "False" );
    return;
}

void c_npy_write_to_file( const char *filename, const cmatrix_t *m )
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
    assert( ptr - shape < 512 );

    /* Potential bug? There are some additional whitespaces after the dictionaries saved from
     * Python/Numpy. Those are not documented? I have tested that this dictionary actually works */
    size_t len = sprintf(dict, "{'descr': '%c%c%c', 'fortran_order': %s, 'shape': (%s), }",
            m->endianness,
            m->typechar,
            (char) m->itemsize + '0',
            m->fortran_order ? "True": "False",
            shape);

    assert( len < 1024 );
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

    chk = fwrite( m->data, m->itemsize, n_elements, fp );
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
