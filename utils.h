/******************************************************************************

Implemented A5/1 under the terms of the GNU General Public License 
as published by the Free Software Foundation. 

Modified and redistrubuted under the terms of the GNU. 

A copy of the GNU General Public License is included along with this program;

GDB online is an online compiler and debugger tool for C: 
https://www.onlinegdb.com


*******************************************************************************/

#ifndef OSMOCORE_UTIL_H
#define OSMOCORE_UTIL_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include <stdint.h>

struct value_string {
	unsigned int value;
	const char *str;
};

const char *get_value_string(const struct value_string *vs, uint32_t val);
int get_string_value(const struct value_string *vs, const char *str);

char *osmo_ubit_dump(const uint8_t *bits, unsigned int len);
char *osmo_hexdump_nospc(const unsigned char *buf, int len);

int osmo_hexparse(const char *str, uint8_t *b, int max_len);


#endif