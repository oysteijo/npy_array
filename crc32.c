/* Taken from Rosetta code impplementation  which again resamble
   the examples from RFC 1952 section 8 and others.

   I, Oystein, do not claim any rights of course. I also think
   this code should be considered Public Domain.
 */

#include "crc32.h"
#include <stdbool.h>
uint32_t crc32(uint32_t crc, const char *buf, size_t len)
{
	static uint32_t table[256];
	static bool have_table = false;
 
	/* This check is not thread safe; there is no mutex. */
	if (!have_table) {
		/* Calculate CRC table. */
		for (int i = 0; i < 256; i++) {
            uint32_t rem = i;  /* remainder from polynomial division */
			for (int j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= 0xedb88320;
				} else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = true;
	}
 
	crc = ~crc;
	const char *q = buf + len;
	for (const char *p = buf; p < q; p++) {
		uint8_t octet = *p;  /* Cast to unsigned octet. */
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}
