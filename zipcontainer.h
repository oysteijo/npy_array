#ifndef __ZIPCONTAINER_H__
#define __ZIPCONTAINER_H__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#define LOCAL_HEADER_LENGTH 30
typedef struct _local_file_header_t
{
    uint32_t  local_file_header_signature;  /*  4 bytes  (0x04034b50) */
    uint16_t  version_needed_to_extract;    /*  2 bytes */
    uint16_t  general_purpose_bit_flag;     /*  2 bytes */
    uint16_t  compression_method;           /*  2 bytes */
    uint16_t  last_mod_file_time;           /*  2 bytes */
    uint16_t  last_mod_file_date;           /*  2 bytes */
    uint32_t  crc_32;                       /*  4 bytes */
    uint32_t  compressed_size;              /*  4 bytes */
    uint32_t  uncompressed_size;            /*  4 bytes */
    uint16_t  file_name_length;             /*  2 bytes */
    uint16_t  extra_field_length;           /*  2 bytes */
    char     *file_name;                    /*  (variable size) */
    char     *extra_field;                  /*  (variable size) */
} local_file_header_t;

typedef struct _data_descriptor_t
{
    uint32_t signature;
    uint32_t crc_32;                        /*  4 bytes */
    uint32_t compressed_size;               /*  4 bytes */
    uint32_t uncompressed_size;             /*  4 bytes */
} data_descriptor_t;

typedef struct _central_directory_header_t
{
    uint32_t central_file_header_signature;   /* 4 bytes  (0x02014b50) */
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

typedef struct _digital_signature_t
{
    uint32_t  header_signature;              /*  4 bytes  (0x05054b50) */
    uint16_t  size_of_data;                  /*  2 bytes */
    char     *signature_data;                /*  (variable size) */
} digital_signature_t;

#endif /* __ZIPCONTAINER_H__ */

