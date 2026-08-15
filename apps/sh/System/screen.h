#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdio.h> 

#define SCREEN_SIZE (30*16)*3 + 1 // (30*16)*3 为全屏汉字所需字节数量, +1为 '\0' 

typedef struct {
    char buf[SCREEN_SIZE];
    size_t offset;
	size_t length;
} screen_t;

extern screen_t screen;

void screen_init(void);
void screen_seek(long offset, int whence);
void screen_puts(char *str);
void screen_appends(char *str);
char *screen_gets(char *str, int n);

#endif
