#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdio.h> 

#define SCREEN_SIZE (30*16)*3 + 1 // (30*16)*3 为全屏汉字所需字节数量, +1为 '\0' 
#define SCREEN_PRINTF_BUFSIZ (512)
#define UNIT_BYTE 0
#define UNIT_CHAR 1

typedef struct {
    char buf[SCREEN_SIZE];
    size_t offset;
	size_t length;
} screen_t;

extern screen_t screen;

void screen_init(void);
void screen_seek(long offset, int whence, int unit);
void screen_puts(char *str);
void screen_appends(char *str);
void screen_printf(const char *format, ...);
char *screen_gets(char *str, int n);

#endif
