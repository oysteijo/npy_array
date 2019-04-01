/* zipcontainer.h - Øystein Schønning-Johansen 2019 
 *
 * This is mainly a little set of algorithms to read and write PKZIP formated files.
 * The current limitation is basicall that it only reads the these three structures:
 *
 *  * local file header
 *  * central directory file header
 *  * end of central directory file header
 *
 *  ... however this is enough to read  and create some basic zip files like numpy arrays.
 */
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

#if 0
/* FIXME. This structure is not yet in use in this library. */
#define DATA_DESCRIPTOR_LENGTH 12
#define DATA_DESCRIPTOR_SIGNATURE 0x08074b50 
typedef struct _data_descriptor_t
{
    uint32_t signature;
    uint32_t crc_32;                          /*  4 bytes */
    uint32_t compressed_size;                 /*  4 bytes */
    uint32_t uncompressed_size;               /*  4 bytes */
} data_descriptor_t;
#endif

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

#if 0
/* This is actually not used yet in this library */
#define DIGITAL_SIGNATURE_LENGTH 6
#define DIGITAL_SIGNATURE_SIGNATURE 0x05054b50
typedef struct _digital_signature_t
{
    uint32_t  header_signature;               /*  4 bytes  (0x05054b50) */
    uint16_t  size_of_data;                   /*  2 bytes */
    char     *signature_data;                 /*  (variable size) */
} digital_signature_t;
#endif

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

void _read_local_fileheader             ( FILE *fp, local_file_header_t *lfh );
void _read_central_directory_fileheader ( FILE *fp, central_directory_header_t *cdh );
void _read_end_of_central_dir           ( FILE *fp, end_of_central_dir_t *eocd );

void _write_local_fileheader            ( FILE *fp, const local_file_header_t * lfh );
void _write_cental_directory_fileheader ( FILE *fp, const central_directory_header_t *cdh );
void _write_end_of_central_dir          ( FILE *fp, const end_of_central_dir_t *eocd );

#ifndef NDEBUG
void _dump_local_fileheader             ( const local_file_header_t * lfh );
void _dump_central_directory_fileheader ( const central_directory_header_t *cdh );
void _dump_end_of_central_dir           ( const end_of_central_dir_t *eocd );
#endif
#endif /* __ZIPCONTAINER_H__*/
