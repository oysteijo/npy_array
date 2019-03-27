#ifndef __CRC32_H__
#define __CRC32_H__
#include <inttypes.h>
#include <stdlib.h>
uint32_t crc32(uint32_t crc, const char *buf, size_t len);
#endif /* __CRC32_H__ */
