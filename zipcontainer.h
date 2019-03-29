#ifndef __ZIPCONTAINER_H__
#define __ZIPCONTAINER_H__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define LOCAL_HEADER_LENGTH 30
#define LOCAL_HEADER_SIGNATURE 0x04034b50

typedef struct _local_file_header_t
{
    uint32_t  local_file_header_signature;    /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
    uint16_t  version_needed_to_extract;      /*  2 bytes */
    uint16_t  general_purpose_bit_flag;       /*  2 bytes */
    uint16_t  compression_method;             /*  2 bytes */
    uint16_t  last_mod_file_time;             /*  2 bytes */
    uint16_t  last_mod_file_date;             /*  2 bytes */
    uint32_t  crc_32;                         /*  4 bytes */
    uint32_t  compressed_size;                /*  4 bytes */
    uint32_t  uncompressed_size;              /*  4 bytes */
    uint16_t  file_name_length;               /*  2 bytes */
    uint16_t  extra_field_length;             /*  2 bytes */
    char     *file_name;                      /*  (variable size) */
    char     *extra_field;                    /*  (variable size) */
} local_file_header_t;

#define DATA_DESCRIPTOR_LENGTH 12
#define DATA_DESCRIPTOR_SIGNATURE 0x08074b50 
typedef struct _data_descriptor_t
{
    uint32_t signature;
    uint32_t crc_32;                          /*  4 bytes */
    uint32_t compressed_size;                 /*  4 bytes */
    uint32_t uncompressed_size;               /*  4 bytes */
} data_descriptor_t;

#define CENTRAL_DIRECTORY_HEADER_LENGTH 46
#define CENTRAL_DIRECTORY_HEADER_SIGNATURE 0x02014b50
typedef struct _central_directory_header_t
{
    uint32_t central_file_header_signature;   /* 4 bytes  (CENTRAL_DIRECTORY_HEADER_SIGNATURE) */
    uint16_t version_made_by;                 /* 2 bytes */
    uint16_t version_needed_to_extract;       /* 2 bytes */
    uint16_t general_purpose_bit_flag;        /* 2 bytes */
    uint16_t compression_method;              /* 2 bytes */
    uint16_t last_mod_file_time;              /* 2 bytes */
    uint16_t last_mod_file_date;              /* 2 bytes */
    uint32_t crc_32;                          /* 4 bytes */
    uint32_t compressed_size;                 /* 4 bytes */
    uint32_t uncompressed_size;               /* 4 bytes */
    uint16_t file_name_length;                /* 2 bytes */
    uint16_t extra_field_length;              /* 2 bytes */
    uint16_t file_comment_length;             /* 2 bytes */
    uint16_t disk_number_start;               /* 2 bytes */
    uint16_t internal_file_attributes;        /* 2 bytes */
    uint32_t external_file_attributes;        /* 4 bytes */
    uint32_t relative_offset_of_local_header; /* 4 bytes */

    char    *file_name;                       /*  (variable size) */
    char    *extra_field;                     /*  (variable size) */
    char    *file_comment;                    /*  (variable size) */
} central_directory_header_t;

/* This is actually not used yet in this library */
#define DIGITAL_SIGNATURE_LENGTH 6
#define DIGITAL_SIGNATURE_SIGNATURE 0x05054b50
typedef struct _digital_signature_t
{
    uint32_t  header_signature;               /*  4 bytes  (0x05054b50) */
    uint16_t  size_of_data;                   /*  2 bytes */
    char     *signature_data;                 /*  (variable size) */
} digital_signature_t;

#define END_OF_CENTRAL_DIR_LENGTH 22
#define END_OF_CENTRAL_DIR_SIGNATURE 0x06054b50
typedef struct _end_of_central_dir_t
{
    uint32_t end_of_central_dir_signature;    /*  4 bytes (END_OF_CENTRAL_DIR_SIGNATURE) */
    uint16_t number_of_this_disk;             /*  2 bytes */
    uint16_t number_of_the_disk_start_of_cd;  /*  2 bytes */ 
    uint16_t total_num_entries_this_disk;     /*  2 bytes */
    uint16_t total_num_entries_cd;            /*  2 bytes */
    uint32_t size_of_cd;                      /*  4 bytes */
    uint32_t offset_cd_wrt_disknum;           /*  4 bytes */
    uint16_t ZIP_file_comment_length;         /*  2 bytes */
    char *   ZIP_file_comment;                /*  (variable size) */
} end_of_central_dir_t;

/* I hate passing pointer to files in functions, but I've sone an exception here. */

static inline void _write_local_fileheader( local_file_header_t *lf, FILE *fp )
{
    fwrite( &lf->local_file_header_signature, sizeof(uint32_t), 1, fp) ;    /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
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
    fwrite(  lf->file_name,                   sizeof(char), lf->file_name_length, fp) ;    /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
    fwrite(  lf->extra_field,                 sizeof(char), lf->extra_field_length, fp) ;    /*  4 bytes  (LOCAL_HEADER_SIGNATURE) */
}

static inline void _write_cental_directory_fileheader( central_directory_header_t *cdh, FILE *fp )
{
    fwrite( &cdh->central_file_header_signature, sizeof(uint32_t), 1, fp);   /* 4 bytes  (CENTRAL_DIRECTORY_HEADER_SIGNATURE) */
    fwrite( &cdh->version_made_by, sizeof(uint16_t), 1, fp);                 /* 2 bytes */
    fwrite( &cdh->version_needed_to_extract, sizeof(uint16_t), 1, fp);       /* 2 bytes */
    fwrite( &cdh->general_purpose_bit_flag, sizeof(uint16_t), 1, fp);        /* 2 bytes */
    fwrite( &cdh->compression_method, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite( &cdh->last_mod_file_time, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite( &cdh->last_mod_file_date, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite( &cdh->crc_32, sizeof(uint32_t), 1, fp);                          /* 4 bytes */
    fwrite( &cdh->compressed_size, sizeof(uint32_t), 1, fp);                 /* 4 bytes */
    fwrite( &cdh->uncompressed_size, sizeof(uint32_t), 1, fp);               /* 4 bytes */
    fwrite( &cdh->file_name_length, sizeof(uint16_t), 1, fp);                /* 2 bytes */
    fwrite( &cdh->extra_field_length, sizeof(uint16_t), 1, fp);              /* 2 bytes */
    fwrite( &cdh->file_comment_length, sizeof(uint16_t), 1, fp);             /* 2 bytes */
    fwrite( &cdh->disk_number_start, sizeof(uint16_t), 1, fp);               /* 2 bytes */
    fwrite( &cdh->internal_file_attributes, sizeof(uint16_t), 1, fp);        /* 2 bytes */
    fwrite( &cdh->external_file_attributes, sizeof(uint32_t), 1, fp);        /* 4 bytes */
    fwrite( &cdh->relative_offset_of_local_header, sizeof(uint32_t), 1, fp); /* 4 bytes */

    fwrite( cdh->file_name,    sizeof(char), cdh->file_name_length, fp);    /*  (variable size) */
    fwrite( cdh->extra_field,  sizeof(char), cdh->extra_field_length, fp);  /*  (variable size) */
    fwrite( cdh->file_comment, sizeof(char), cdh->file_comment_length, fp); /*  (variable size) */
}

static inline void _write_eocd( end_of_central_dir_t *eocd, FILE *fp )
{
    fwrite( &eocd->end_of_central_dir_signature, sizeof(uint32_t), 1, fp);    /*  4 bytes (END_OF_CENTRAL_DIR_SIGNATURE) */
    fwrite( &eocd->number_of_this_disk, sizeof(uint16_t), 1, fp);             /*  2 bytes */
    fwrite( &eocd->number_of_the_disk_start_of_cd, sizeof(uint16_t), 1, fp);  /*  2 bytes */ 
    fwrite( &eocd->total_num_entries_this_disk, sizeof(uint16_t), 1, fp);     /*  2 bytes */
    fwrite( &eocd->total_num_entries_cd, sizeof(uint16_t), 1, fp);            /*  2 bytes */
    fwrite( &eocd->size_of_cd, sizeof(uint32_t), 1, fp);                      /*  4 bytes */
    fwrite( &eocd->offset_cd_wrt_disknum, sizeof(uint32_t), 1, fp);           /*  4 bytes */
    fwrite( &eocd->ZIP_file_comment_length, sizeof(uint16_t), 1, fp);         /*  2 bytes */
    fwrite( eocd->ZIP_file_comment, sizeof(char), eocd->ZIP_file_comment_length, fp );  /*  (variable size) */
}

#endif /* __ZIPCONTAINER_H__ */

