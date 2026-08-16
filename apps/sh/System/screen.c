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
void screen_seek(long offset, int whence, int unit) {
    if (unit == UNIT_BYTE) {
        // 按字节偏移
        long new_pos = 0;
        switch (whence) {
            case SEEK_SET:
                new_pos = offset;
                break;
            case SEEK_CUR:
                new_pos = (long)screen.offset + offset;
                break;
            case SEEK_END:
                new_pos = (long)screen.length + offset;
                break;
            default:
                return;
        }
        // 边界检查
        if (new_pos < 0) new_pos = 0;
        if (new_pos > (long)SCREEN_SIZE) new_pos = SCREEN_SIZE;
        screen.offset = (size_t)new_pos;

    } else if (unit == UNIT_CHAR) {
        // 按字符偏移(UTF-8码点)
        long target_char = 0;   // 目标字符位置
        long cur_char = 0;      // 当前字符位置

        // 计算当前offset对应的字符位置
        size_t pos = 0;
        long count = 0;
        while (pos < screen.offset) {
            unsigned char c = (unsigned char)screen.buf[pos];
            pos += utf8_seq_len(c);
            count++;
        }
        cur_char = count;

        switch (whence) {
            case SEEK_SET:
                target_char = offset;
                break;
            case SEEK_CUR:
                target_char = cur_char + offset;
                break;
            case SEEK_END: {
                // 计算有效内容长度的字符数
                size_t p = 0;
                long total = 0;
                while (p < screen.length) {
                    unsigned char c = (unsigned char)screen.buf[p];
                    p += utf8_seq_len(c);
                    total++;
                }
                target_char = total + offset;
                break;
            }
            default:
                return;
        }

        if (target_char < 0) target_char = 0;
        // 将目标字符位置转换为字节偏移
        pos = 0;
        count = 0;
        while (pos < SCREEN_SIZE && count < target_char) {
            unsigned char c = (unsigned char)screen.buf[pos];
            pos += utf8_seq_len(c);
            count++;
        }
        if (pos > SCREEN_SIZE) pos = SCREEN_SIZE;
        screen.offset = pos;

    } else {
        return;
    }
}

void screen_putc(char c) {
	if(screen.offset >= SCREEN_SIZE) return;
    screen.buf[screen.offset++] = c;
    if(screen.offset > screen.length) {
        screen.length = screen.offset;
    }
}

void screen_puts(char *str) {
	while(*str) {
		screen_putc(*str++);
    }
	if (screen.length < SCREEN_SIZE) {
        screen.buf[screen.length] = '\0';
    }
	else {
        screen.buf[SCREEN_SIZE - 1] = '\0';
    }
} 

void screen_appends(char *str) {
	screen.offset = screen.length;
	screen_puts(str);
}

void screen_printf(const char *format, ...) {
    char tmp[SCREEN_PRINTF_BUFSIZ];
    va_list args;
    va_start(args, format);
    vsnprintf(tmp, sizeof(tmp), format, args);
    va_end(args);
    screen_puts(tmp);
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

