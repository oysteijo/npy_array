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

static void _write_local_fileheader( local_file_header_t *lf, FILE *fp )
{
    fwrite( &lf->local_file_header_signature, sizeof(uint32_t), 1, fp) ;    /*  4 bytes  (0x04034b50) */
    fwrite( &lf->version_needed_to_extract,   sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->general_purpose_bit_flag,    sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->compression_method,          sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->last_mod_file_time,          sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->last_mod_file_date,          sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->crc_32,                      sizeof(uint32_t), 1, fp) ;    /*  4 bytes */
    fwrite( &lf->compressed_size,             sizeof(uint32_t), 1, fp) ;    /*  4 bytes */
    fwrite( &lf->uncompressed_size,           sizeof(uint32_t), 1, fp) ;    /*  4 bytes */
    fwrite( &lf->file_name_length,            sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite( &lf->extra_field_length,          sizeof(uint16_t), 1, fp) ;    /*  2 bytes */
    fwrite(  lf->file_name,                   sizeof(char), lf->file_name_length, fp) ;    /*  4 bytes  (0x04034b50) */
    fwrite(  lf->extra_field,                 sizeof(char), lf->extra_field_length, fp) ;    /*  4 bytes  (0x04034b50) */
}

static void _write_cental_directory_fileheader( central_directory_header_t *cdh, FILE *fp )
{
    fwrite ( &cdh->central_file_header_signature, sizeof(uint32_t), 1, fp);   /* 4 bytes  (0x02014b50) */
    fwrite ( &cdh->version_made_by, sizeof(uint16_t), 1, fp);                 /* 2 bytes */
    fwrite ( &cdh->version_needed_to_extract, sizeof(uint16_t), 1, fp);       /* 2 bytes */
    fwrite ( &cdh->general_purpose_bit_flag, sizeof(uint16_t), 1, fp);        /* 2 bytes */
    fwrite ( &cdh->compression_method, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite ( &cdh->last_mod_file_time, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite ( &cdh->last_mod_file_date, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite ( &cdh->crc_32, sizeof(uint32_t), 1, fp);                          /* 4 bytes */
    fwrite ( &cdh->compressed_size, sizeof(uint32_t), 1, fp);                 /* 4 bytes */
    fwrite ( &cdh->uncompressed_size, sizeof(uint32_t), 1, fp);               /* 4 bytes */
    fwrite ( &cdh->file_name_length, sizeof(uint16_t), 1, fp);                /* 2 bytes */
    fwrite ( &cdh->extra_field_length, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite ( &cdh->file_comment_length, sizeof(uint16_t), 1, fp);             /* 2 bytes */
    fwrite ( &cdh->disk_number_start, sizeof(uint16_t), 1, fp);               /* 2 bytes */
    fwrite ( &cdh->internal_file_attributes, sizeof(uint16_t), 1, fp);        /* 2 bytes */
    fwrite ( &cdh->external_file_attributes, sizeof(uint32_t), 1, fp);        /* 4 bytes */
    fwrite ( &cdh->relative_offset_of_local_header, sizeof(uint32_t), 1, fp); /* 4 bytes */

    fwrite ( cdh->file_name,    sizeof(char), cdh->file_name_length, fp);    /*  (variable size) */
    fwrite ( cdh->extra_field,  sizeof(char), cdh->extra_field_length, fp);  /*  (variable size) */
    fwrite ( cdh->file_comment, sizeof(char), cdh->file_comment_length, fp); /*  (variable size) */
}

static void _write_eocd( end_of_central_dir_t *eocd, FILE *fp )
{
    fwrite( &eocd->end_of_central_dir_signature, sizeof(uint32_t), 1, fp);    /*  4 bytes (0x06054b50) */
    fwrite( &eocd->number_of_this_disk, sizeof(uint16_t), 1, fp);             /*  2 bytes */
    fwrite( &eocd->number_of_the_disk_start_of_cd, sizeof(uint16_t), 1, fp);  /*  2 bytes */ 
    fwrite( &eocd->total_num_entries_this_disk, sizeof(uint16_t), 1, fp);     /*  2 bytes */
    fwrite( &eocd->total_num_entries_cd, sizeof(uint16_t), 1, fp);            /*  2 bytes */
    fwrite( &eocd->size_of_cd, sizeof(uint32_t), 1, fp);                      /*  4 bytes */
    fwrite( &eocd->offset_cd_wrt_disknum, sizeof(uint32_t), 1, fp);           /*  4 bytes */
    fwrite( &eocd->ZIP_file_comment_length, sizeof(uint16_t), 1, fp);         /*  2 bytes */
    fwrite( eocd->ZIP_file_comment, sizeof(char), eocd->ZIP_file_comment_length, fp );  /*  (variable size) */
}

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
    size_t len = sprintf(dict, "{'descr': '%c%c%c', 'fortran_order': %s, 'shape': (%s), }",
            m->endianness,
            m->typechar,
            (char) m->elem_size + '0',
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
static void _write_matrix( const cmatrix_t *m, FILE *fp )
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
            .local_file_header_signature = 0x04034b50,                   /*  4 bytes  (0x04034b50) */
            .version_needed_to_extract   = 20,                           /*  2 bytes */
            .last_mod_file_time          = (uint16_t) (dt_now & 0xffff), /*  2 bytes */
            .last_mod_file_date          = (uint16_t) (dt_now >> 16),    /*  2 bytes */
            .crc_32                      = crc32,                        /*  4 bytes */
            .compressed_size             = size,                         /*  4 bytes */
            .uncompressed_size           = size,                         /*  4 bytes */
            .file_name_length            = strlen(arrname),              /*  2 bytes */
            .file_name                   = arrname                       /*  (variable size) */
        };
        _write_local_fileheader( &lfh, fp );
        _write_matrix          ( array[i], fp );
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
            .central_file_header_signature = 0x02014b50, /* 4 bytes */
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
        /* Oh! I have to count */
        _write_cental_directory_fileheader( &cdh, fp );
        offset_count += size + cdh.file_name_length + LOCAL_HEADER_LENGTH;
        total_namelength += cdh.file_name_length;
    }

    end_of_central_dir_t eocd = {
        .end_of_central_dir_signature    = 0x06054b50,       /*  4 bytes (0x06054b50) */
        .total_num_entries_this_disk     = (uint16_t) n,     /*  2 bytes */
        .total_num_entries_cd            = (uint16_t) n,     /*  2 bytes */
        .size_of_cd                      = (uint32_t) n * CENTRAL_DIRECTORY_HEADER_LENGTH + total_namelength,  /*  4 bytes */
        .offset_cd_wrt_disknum           = offset_count     /*  4 bytes */
    };

    _write_eocd( &eocd, fp );
    return n;
}

static cmatrix_t * _read_matrix( FILE *fp );
static void _read_end_of_central_dir( FILE *fp, end_of_central_dir_t *eocd );
static void end_of_central_dir_dump( end_of_central_dir_t *eocd );
static void _read_central_directory_header( FILE *fp, central_directory_header_t *cdh );
static void _central_directory_header_dump( central_directory_header_t *cdh );

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
#if 1 // I_REALLY_DONT_CARE_ABOUT_THIS_SINCE_IVE_ALREADY_READ_ALL_THE_DATA_I_NEED
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
    end_of_central_dir_dump( &eocd );
#endif
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

static void _read_end_of_central_dir( FILE *fp, end_of_central_dir_t *eocd )
{
    /* FIXME: Do checks all reads and malloc. */
    char buffer[END_OF_CENTRAL_DIR_LENGTH];
    size_t chk = fread( buffer, 1, END_OF_CENTRAL_DIR_LENGTH, fp ); /* FIXME */
    if (chk != END_OF_CENTRAL_DIR_LENGTH ){
        fprintf(stderr, "Cannot read buffer.\n");
        return;
    }

    if (*(uint32_t*)(buffer) != 0x06054b50 ) /* Whoops! */
        return;

    eocd->end_of_central_dir_signature   = *(uint32_t*)(buffer);     /*  4 bytes (0x06054b50) */
    eocd->number_of_this_disk            = *(uint16_t*)(buffer+4);   /*  2 bytes */
    eocd->number_of_the_disk_start_of_cd = *(uint16_t*)(buffer+6);   /*  2 bytes */ 
    eocd->total_num_entries_this_disk    = *(uint16_t*)(buffer+8);    /*  2 bytes */
    eocd->total_num_entries_cd           = *(uint16_t*)(buffer+10);  /*  2 bytes */
    eocd->size_of_cd                     = *(uint32_t*)(buffer+12);  /*  4 bytes */
    eocd->offset_cd_wrt_disknum          = *(uint32_t*)(buffer+16);  /*  4 bytes */
    eocd->ZIP_file_comment_length        = *(uint16_t*)(buffer+20);  /*  2 bytes */

    eocd->ZIP_file_comment = calloc( eocd->ZIP_file_comment_length+1, sizeof(char));
    assert( eocd->ZIP_file_comment);
    fread( eocd->ZIP_file_comment, sizeof(char), eocd->ZIP_file_comment_length, fp );

}

static void end_of_central_dir_dump( end_of_central_dir_t *eocd )
{
    printf("(%d) end_of_central_dir_signature\n",   eocd->end_of_central_dir_signature);
    printf("(%d) number_of_this_disk\n",            eocd->number_of_this_disk);
    printf("(%d) number_of_the_disk_start_of_cd\n", eocd->number_of_the_disk_start_of_cd);
    printf("(%d) total_num_entries_this_disk\n",    eocd->total_num_entries_this_disk);
    printf("(%d) total_num_entries_cd\n",           eocd->total_num_entries_cd);
    printf("(%d) size_of_cd\n",                     eocd->size_of_cd);
    printf("(%d) offset_cd_wrt_disknum\n",          eocd->offset_cd_wrt_disknum);
    printf("(%d) ZIP_file_comment_length\n",        eocd->ZIP_file_comment_length);
}

static void _read_central_directory_header( FILE *fp, central_directory_header_t *cdh )
{
    /* FIXME: Do checks all reads and malloc. */
    char buffer[CENTRAL_DIRECTORY_HEADER_LENGTH];
    size_t chk = fread( buffer, 1, CENTRAL_DIRECTORY_HEADER_LENGTH, fp ); /* FIXME */
    if (chk != CENTRAL_DIRECTORY_HEADER_LENGTH ){
        fprintf(stderr, "Cannot read buffer.\n");
        return;
    }

    if (*(uint32_t*)(buffer) != 0x02014b50 ) /* Whoops! */
        return;

    cdh->central_file_header_signature   = *(uint32_t*)(buffer);     /*  4 bytes */
    cdh->version_made_by                 = *(uint32_t*)(buffer+4);   /*  2 bytes */
    cdh->version_needed_to_extract       = *(uint32_t*)(buffer+6);   /*  2 bytes */
    cdh->general_purpose_bit_flag        = *(uint32_t*)(buffer+8);   /*  2 bytes */
    cdh->compression_method              = *(uint32_t*)(buffer+10);  /*  2 bytes */
    cdh->last_mod_file_time              = *(uint32_t*)(buffer+12);  /*  2 bytes */
    cdh->last_mod_file_date              = *(uint32_t*)(buffer+14);  /*  2 bytes */
    cdh->crc_32                          = *(uint32_t*)(buffer+16);  /*  4 bytes */
    cdh->compressed_size                 = *(uint32_t*)(buffer+20);  /*  4 bytes */
    cdh->uncompressed_size               = *(uint32_t*)(buffer+24);  /*  4 bytes */
    cdh->file_name_length                = *(uint32_t*)(buffer+28);  /*  2 bytes */
    cdh->extra_field_length              = *(uint32_t*)(buffer+30);  /*  2 bytes */
    cdh->file_comment_length             = *(uint32_t*)(buffer+32);  /*  2 bytes */
    cdh->disk_number_start               = *(uint32_t*)(buffer+34);  /*  2 bytes */
    cdh->internal_file_attributes        = *(uint32_t*)(buffer+36);  /*  2 bytes */
    cdh->external_file_attributes        = *(uint32_t*)(buffer+38);  /*  4 bytes */
    cdh->relative_offset_of_local_header = *(uint32_t*)(buffer+42);  /*  4 bytes */

    cdh->file_name = calloc( cdh->file_name_length+1, sizeof(char));
    assert( cdh->file_name);
    fread( cdh->file_name, sizeof(char), cdh->file_name_length, fp );

    cdh->extra_field = calloc( cdh->extra_field_length+1, sizeof(char));
    assert( cdh->extra_field);
    fread( cdh->extra_field, sizeof(char), cdh->extra_field_length, fp );

    cdh->file_comment = calloc( cdh->file_comment_length+1, sizeof(char));
    assert( cdh->file_comment);
    fread( cdh->file_comment, sizeof(char), cdh->file_comment_length, fp );
}

static void _central_directory_header_dump( central_directory_header_t *cdh )
{
    printf("(%d) central_file_header_signature\n", cdh->central_file_header_signature);   /* 4 bytes  (0x02014b50) */
    printf("(%d) version_made_by\n", cdh->version_made_by);                 /* 2 bytes */
    printf("(%d) version_needed_to_extract\n", cdh->version_needed_to_extract);       /* 2 bytes */
    printf("(%d) general_purpose_bit_flag\n", cdh->general_purpose_bit_flag);        /* 2 bytes */
    printf("(%d) compression_method\n", cdh->compression_method);              /* 2 bytes */
    printf("(%d) last_mod_file_time\n", cdh->last_mod_file_time);              /* 2 bytes */
    printf("(%d) last_mod_file_date\n", cdh->last_mod_file_date);              /* 2 bytes */
    printf("(%d) crc_32\n", cdh->crc_32);                          /* 4 bytes */
    printf("(%d) compressed_size\n", cdh->compressed_size);                 /* 4 bytes */
    printf("(%d) uncompressed_size\n", cdh->uncompressed_size);               /* 4 bytes */
    printf("(%d) file_name_length\n", cdh->file_name_length);                /* 2 bytes */
    printf("(%d) extra_field_length\n", cdh->extra_field_length);              /* 2 bytes */
    printf("(%d) file_comment_length\n", cdh->file_comment_length);             /* 2 bytes */
    printf("(%d) disk_number_start\n", cdh->disk_number_start);               /* 2 bytes */
    printf("(%d) internal_file_attributes\n", cdh->internal_file_attributes);        /* 2 bytes */
    printf("(%d) external_file_attributes\n", cdh->external_file_attributes);        /* 4 bytes */
    printf("(%d) relative_offset_of_local_header\n", cdh->relative_offset_of_local_header); /* 4 bytes */

    printf("Filename: %s\n", cdh->file_name );
    printf("Filecomment: %s\n", cdh->file_comment );
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
    _write_matrix( m, fp );
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
