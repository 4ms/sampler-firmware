void *memcpy(void *dest, const void *src, unsigned int n) {
	char *dp = (char *)dest;
	const char *sp = (const char *)src;
	while (n--)
		*dp++ = *sp++;
	return dest;
}
