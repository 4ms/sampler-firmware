#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#define FF_MAX_LFN 255

using FRESULT = uint8_t;

typedef char TCHAR;
typedef unsigned int UINT;	/* int must be 16-bit or 32-bit */
typedef unsigned char BYTE; /* char must be 8-bit */
typedef uint16_t WORD;		/* 16-bit unsigned integer */
typedef uint32_t DWORD;		/* 32-bit unsigned integer */
typedef uint64_t QWORD;		/* 64-bit unsigned integer */
typedef WORD WCHAR;			/* UTF-16 character type */
typedef QWORD FSIZE_t;
typedef DWORD LBA_t;

#define FA_READ 0x01
#define FA_WRITE 0x02
#define FA_OPEN_EXISTING 0x00
#define FA_CREATE_NEW 0x04
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS 0x10
#define FA_OPEN_APPEND 0x30

enum {
	FR_OK = 0,
	FR_INT_ERR = 1,
};

using FIL = FILE;

// inline FRESULT f_open(FIL *fp, const TCHAR *path, BYTE mode) {
// 	if (mode & FA_READ)
// 		fp = fopen(path, "r");
// 	return FR_OK;
// }

inline FRESULT f_close(FIL *fp) {
	return fclose(fp);
}

inline FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br) {
	*br = fread(buff, btr, 1, fp);
	return FR_OK;
}

// inline FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
inline FRESULT f_lseek(FIL *fp, FSIZE_t ofs) {
	return fseek(fp, ofs, SEEK_SET);
}

inline FSIZE_t f_tell(FIL *fp) {
	return ftell(fp);
}

inline FSIZE_t f_size(FIL *fp) {
	fseek(fp, 0, SEEK_END);
	auto size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}
