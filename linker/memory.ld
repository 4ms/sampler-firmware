/* DDR RAM */

/* _UNUSED0		= 0xC0000000; */
/* _UNUSED0_SZ		= 1M; 		/*0x00100000*/ */
/* _UNUSED0_END	= 0xC0100000; */

/* _TTB			= 0xC0100000; */
/* _TTB_SZ			= 1M; */
/* _TTB_END		= 0xC0200000; */

/* _A7_CODE 		= 0xC2000040; */
/* _A7_CODE_SZ  	= 16M - 0x40;/*0x00FFFFD8*/ */
/* _A7_CODE_END 	= 0xC3000000; */

/* _A7_RAM 		= 0xC3000000; */
/* _A7_RAM_SZ 		= 144M;		/*0x09000000*/ */
/* _A7_RAM_END 	= 0xCC000000; */

/* _A7_HEAP		= 0xCC000000; */
/* _A7_HEAP_SZ 	= 63M; 		/*0x03F00000*/ */
/* _A7_HEAP_END	= 0xCFF00000; */

/* _DMABUF			= 0xDFF00000; */
/* _DMABUF_SZ		= 1M;		/*0x00100000*/ */
/* _DMABUF_END		= 0xE0000000; */

/* /1* SRAM *1/ */

/* _M4_CODE 		= 0x10000000; */	
/* _M4_CODE_SZ		= 320K;		/*0x00050000*/ */
/* _M4_CODE_END	= 0x10050000; */

/* _M4_STACK		= 0x10050000; */
/* _M4_STACK_SZ	= 64K - 256; */
/* _M4_STACK_END	= 0x1005FF00; */

/* _M4_SHARED_LIST 	= 0x1005FF00; */
/* _A7_SHARED_LIST 	= 0x3005FF00; */
/* _SHARED_LIST_SZ 	= 256; */
/* _M4_SHARED_LIST_END = 0x10060000; */
