#include <stdint.h>

static inline int32_t SSAT16(int32_t x) {
	asm("ssat %[dst], #16, %[src]" : [dst] "=r"(x) : [src] "r"(x));
	return x;
}
