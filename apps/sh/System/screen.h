#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdio.h> 

#define SCREEN_LINE_MAX_CHARS	(30)	// 汉字
#define SCREEN_MAX_LINES		(16)	// 汉字
#define SCREEN_CHAR_BYTES     	(3)		// 3B/UTF-8
#define SCREEN_SIZE ((SCREEN_LINE_MAX_CHARS * SCREEN_MAX_LINES)*SCREEN_CHAR_BYTES + 1) // +1为 '\0' 
#define SCREEN_PRINTF_BUFSIZ 	(512)
#define UNIT_BYTE 0
#define UNIT_CHAR 1
#define EOS 0	//End of Screen

typedef struct {
    char buf[SCREEN_SIZE];
    size_t offset;	// BYTE
	size_t length;	// BYTE
} screen_t;

extern screen_t screen;

void screen_init(void);
void screen_seek(long offset, int whence, int unit);
void screen_putc(char c);
void screen_puts(char *str);
void screen_appends(char *str);
void screen_printf(const char *format, ...);
char *screen_gets(char *str, long n, int unit);

#endif
