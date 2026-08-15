#include "screen.h"
#include <stdarg.h>
#include <string.h>

screen_t screen;

void screen_init(void) {
    memset(screen.buf, ' ', SCREEN_SIZE);
    screen.offset = 0;
    screen.length = 0;
    if (SCREEN_SIZE > 0) {
        screen.buf[SCREEN_SIZE - 1] = '\0';
    }
}

// 获取UTF-8序列长度
static int utf8_seq_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; // 非法字节按单字节处理
}

void screen_seek(long offset, int whence) {
    long target_pos = 0;   // 目标字符位置
    long cur_pos = 0;      // 当前字符位置

    if (whence == SEEK_CUR) {
        size_t pos = 0;
        long count = 0;
        while (pos < screen.offset) {
            unsigned char c = (unsigned char)screen.buf[pos];
            pos += utf8_seq_len(c);
            count++;
        }
        cur_pos = count;
    }

    switch (whence) {
        case SEEK_SET:
            target_pos = offset;
            break;
        case SEEK_CUR:
            target_pos = cur_pos + offset;
            break;
        case SEEK_END: {
            size_t pos = 0;
            long total = 0;
            while (pos < screen.length) {
                unsigned char c = (unsigned char)screen.buf[pos];
                pos += utf8_seq_len(c);
                total++;
            }
            target_pos = total + offset;
            break;
        }
        default:
            return;
    }
	
    if (target_pos < 0) target_pos = 0;
    size_t pos = 0;
	long count = 0;
	while (pos < SCREEN_SIZE && count < target_pos) {
		unsigned char c = (unsigned char)screen.buf[pos];
		pos += utf8_seq_len(c);
		count++;
	}
	if (pos > SCREEN_SIZE) pos = SCREEN_SIZE;
	screen.offset = pos;
}

void screen_puts(char *str) {
	while (*str && screen.offset < SCREEN_SIZE) {
		screen.buf[screen.offset++] = *str++;
		if (screen.offset > screen.length) {
			screen.length = screen.offset;
		}
	}
	if (screen.length < SCREEN_SIZE) {
        screen.buf[screen.length] = '\0';
    } else {
        screen.buf[SCREEN_SIZE - 1] = '\0';
    }
} 
void screen_appends(char *str) {
	screen.offset = screen.length;
	screen_puts(str);
}
char *screen_gets(char *str, int n) {
    if (screen.offset >= screen.length || n <= 0) {
        return NULL;
    }
    int i = 0;
    while (screen.offset < screen.length && i < n - 1) {
        char c = screen.buf[screen.offset++];
        str[i++] = c;
        if (c == '\n') break;
    }
    str[i] = '\0';
    return str;
}
