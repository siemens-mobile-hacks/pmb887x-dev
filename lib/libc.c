#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t len) {
	char *d = dest;
	const char *s = src;
	while (len--)
		*d++ = *s++;
	return dest;
}

void *memset(void *dest, int val, size_t len) {
	uint8_t *ptr = dest;
	while (len > 0) {
		*ptr++ = val;
		len--;
	}
	return dest;
}
