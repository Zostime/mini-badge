#include "main.h"

#include "GUI.h"
#include "ff.h"
#include "rtc_utils.h"
#include "usbd_cdc_if.h"

#include "bootloader_api.h"

#include "shell.h"
#include "system.h"
#include "screen.h"
#include "env.h"

#include <stdbool.h>

size_t screen_get_line_byte_offset(uint8_t line) {
	size_t byte_offset = 0;
	
    for(uint8_t i=0; i<line; i++) {
		char ch[SCREEN_CHAR_BYTES + 1];	// \0
		size_t current_width = 0;
		size_t last_offset = screen.offset;
		
		screen_seek(byte_offset, SEEK_SET, UNIT_BYTE);
		while(screen_gets(ch, 1, UNIT_CHAR) != EOS && ch[0] != '\n') {	// line
			size_t w = SYS_GetStrWidth(ch);
			if(current_width + w > SYS_SCREEN_W) {
				// 若超宽则恢复 offset 到该字符之前
				screen_seek(last_offset, SEEK_SET, UNIT_BYTE);
				byte_offset = last_offset;
				break;
			}
			current_width += w;
			byte_offset+=strlen(ch);
			last_offset = screen.offset;
		}
		if (ch[0] == '\n') {
			byte_offset++;   // 跳过换行符
		}
	}
	return byte_offset;
}

size_t count_screen_lines(void) {
    size_t byte_offset = 0;   // 当前字节偏移
    size_t line_count = 0;    // 总行数
    char ch[SCREEN_CHAR_BYTES + 1]; 

    while(1) {
        size_t current_width = 0;     // 当前屏幕行已占宽度
        int line_ended = 0;           // 本行是否已结束

        screen_seek(byte_offset, SEEK_SET, UNIT_BYTE);
        size_t last_offset = screen.offset;

        // 读取字符直到行结束或文本结束
        while(screen_gets(ch, 1, UNIT_CHAR) != EOS) {
            if(ch[0] == '\n') {
                byte_offset++;
                line_ended = 1;
                break;
            }

            size_t w = SYS_GetStrWidth(ch);
            if(current_width + w > SYS_SCREEN_W) {
                screen_seek(last_offset, SEEK_SET, UNIT_BYTE);
                byte_offset = last_offset;
                line_ended = 1;
                break;
            }			
            current_width += w;
            byte_offset += strlen(ch);
            last_offset = screen.offset; 
        }

        if(line_ended) {
            line_count++;
        } 
		else {
            if(current_width > 0) {
                line_count++;
            }
            break;
        }
    }

    return line_count;
}

FRESULT screen_scrollback(uint8_t direction) { 
	FIL fil;     
	FRESULT res;    
	UINT bytes_written;	
	size_t remaining;
    long start_byte;
	
	static char line_buf[SCREEN_LINE_MAX_CHARS * SCREEN_CHAR_BYTES + 1];
	switch(direction) {
		case SCROLL_UP:
			res = f_open(&fil, SH_SCROLL_HISTORY_PATH, FA_WRITE | FA_OPEN_APPEND | FA_OPEN_ALWAYS);
			if(res != FR_OK) return res;
		
			start_byte = screen_get_line_byte_offset(1);	// SCROLL_UP 1 Line	
			if(start_byte > screen.length) {
				start_byte = screen.length;
			}
			
			memcpy(line_buf, screen.buf, start_byte);
			line_buf[start_byte] = '\0';
			
			remaining = screen.length - start_byte;
			memmove(screen.buf, screen.buf + start_byte, remaining);    
			screen.length = remaining;
			screen_seek(remaining, SEEK_SET, UNIT_BYTE);

			// 清空剩余部分
			memset(screen.buf + screen.length, ' ', SCREEN_SIZE - screen.length);
			
			// 追加数据到文件末尾
			res = f_write(&fil, line_buf, strlen(line_buf), &bytes_written);
			break;
			
		case SCROLL_DOWN: {
			break; 
		}
	} 
	f_close(&fil);
	return res;
}

uint8_t cdc_rx_buf[SH_CMD_SIZE];
volatile uint8_t cdc_rx_ready = 0;
uint16_t cdc_rx_len = 0;
int CDC_ReadLine(char *buf, int size) {
    int idx = 0;
    while(1) {
        if(cdc_rx_ready) {
            cdc_rx_ready = 0;
            for(int i = 0; i < cdc_rx_len && idx < size - 1; i++) {
                char c = cdc_rx_buf[i];
                if(c == '\r' || c == '\n') {
                    buf[idx] = '\0';
                    return idx;
                }
                else if (c == 8 || c == 127) {
                    if(idx > 0) idx--;
                }
                else {
                    buf[idx++] = c;
                }
            }
            buf[idx] = '\0';
            return idx;
        }
    }
}

static bool path_normalize(const char *src, char *dst) {
    if (strncmp(src, "0:", 2) != 0) return false;

    char stack[SH_MAX_PATH] = {0};
    char temp[SH_MAX_PATH];
    strcpy(temp, src + 2);   // 跳过 "0:"
    if (temp[0] == '\0') strcpy(temp, "/");

    char *token = strtok(temp, "/");
    while (token) {
        if (strcmp(token, ".") == 0) {
            // 忽略
        } else if (strcmp(token, "..") == 0) {
            char *last = strrchr(stack, '/');
            if (last) *last = '\0';
            else stack[0] = '\0';
        } else {
            if (stack[0] != '\0') strcat(stack, "/");
            strcat(stack, token);
        }
        token = strtok(NULL, "/");
    }

    if (stack[0] == '\0') {
        strcpy(dst, "0:/");      // 根目录统一为"0:/"
    } else {
        snprintf(dst, SH_MAX_PATH, "0:/%s", stack);
    }
    return true;
}

/**
 * @brief  展开用户输入的路径为规范绝对路径
 * @param  input: 路径字符串
 * @param  cur: 当前工作目录
 * @param  out: 输出规范路径，容量至少 MAX_PATH
 * @retval 是否成功
 */
bool path_expand(const char *input, const char *cur, char *out) {
    char temp[SH_MAX_PATH];

    if (input[0] == '\0') {
        strcpy(out, cur);
        return true;
    }

    // 处理home简写 "~"
    if (input[0] == '~') {
        snprintf(temp, sizeof(temp), "%s%s", env_getenv("HOME"), input + 1);
    }
    // 处理完整路径
    else if (input[0] == '0' && input[1] == ':') {
        snprintf(temp, sizeof(temp), "%s", input); 
    }
    // 处理绝对路径
    else if (input[0] == '/') {
        snprintf(temp, sizeof(temp), "0:%s", input);
    }
    // 相对路径
    else {
        if (strcmp(cur, "0:/") == 0) {
            snprintf(temp, sizeof(temp), "0:/%s", input);
        } else {
            snprintf(temp, sizeof(temp), "%s/%s", cur, input);
        }
    }

    return path_normalize(temp, out);
}

/**
 * @brief  展开命令字符串中的环境变量
 * @param  src: 原始输入字符串
 * @param  dst: 展开后的输出缓冲区
 * @param  dst_size: 输出缓冲区大小
 * @retval 展开后字符串长度, -1 表示缓冲区不足
 */
int shell_expand_vars(const char *src, char *dst, size_t dst_size)
{
    size_t si = 0;  // 源索引
    size_t di = 0;  // 目标索引

    if (dst_size == 0) return -1;

    while (src[si] != '\0') {
        char c = src[si];

        // 处理变量展开
        if (c == '$') {
            si++;
            char var_name[ENV_NAME_MAX];
            size_t name_len = 0;

            // ${VAR}
            if (src[si] == '{') {
                si++;
                while (src[si] != '\0' && src[si] != '}' && name_len < ENV_NAME_MAX - 1) {
                    var_name[name_len++] = src[si++];
                }
                if (src[si] == '}') si++;
            } else {
                // $VAR
                while ((src[si] == '_') ||
                       (src[si] >= 'A' && src[si] <= 'Z') ||
                       (src[si] >= 'a' && src[si] <= 'z') ||
                       (src[si] >= '0' && src[si] <= '9'))
                {
                    if (name_len < ENV_NAME_MAX - 1) {
                        var_name[name_len++] = src[si];
                    }
                    si++;
                }
            }
            var_name[name_len] = '\0';

            const char *val = (name_len > 0) ? env_getenv(var_name) : NULL;
            if (val != NULL) {
                // 拷贝变量值到输出缓冲区
                while (*val != '\0') {
                    if (di >= dst_size - 1) {
                        dst[di] = '\0';
                        return -1; // 缓冲区不足
                    }
                    dst[di++] = *val++;
                }
            }
            continue;
        }

        if (di >= dst_size - 1) {
            dst[di] = '\0';
            return -1;
        }
        dst[di++] = c;
        si++;
    }

    dst[di] = '\0';
    return (int)di;
}

/**
 * @brief  解析命令行, 支持引号, 转义
 * @param  cmd: 输入命令字符串
 * @param  argv: 输出参数数组，末尾为 NULL
 * @param  max_args: 最大参数个数 (含命令和 NULL 哨兵)
 * @retval 实际参数个数
 */
int shell_parse(char *cmd, char *argv[], int max_args) {
    int argc = 0;
    char *src = cmd;
    char *dst = cmd;
    int in_squote = 0;
    int in_dquote = 0;
    int token_started = 0;

    extern const char *env_getenv(const char *name);

    while(*src && (dst - cmd) < SH_CMD_SIZE - 1)
    {
        char c = *src;

        // 反斜杠转义
        if(c == '\\' && !in_squote) {
            src++;
            if(*src == '\0') break;
            if(!token_started) {
                if (argc < max_args - 1) argv[argc++] = dst;
                token_started = 1;
            }
            if((dst - cmd) < SH_CMD_SIZE - 1) *dst++ = *src;
            src++;
            continue;
        }

        // 双引号
        if(c == '"' && !in_squote) {
            in_dquote = !in_dquote;
            src++;
            continue;
        }

        // 单引号
        if(c == '\'' && !in_dquote) {
            in_squote = !in_squote;
            src++;
            continue;
        }

        // 分隔符
        if((c == ' ' || c == '\t' || c == '\r' || c == '\n') &&
            !in_squote && !in_dquote)
        {
            if(token_started) {
                if((dst - cmd) < SH_CMD_SIZE - 1) *dst++ = '\0';
                token_started = 0;
            }
            src++;
            continue;
        }

        // 普通字符
        if(!token_started) {
            if(argc < max_args - 1) argv[argc++] = dst;
            token_started = 1;
        }
        if((dst - cmd) < SH_CMD_SIZE - 1) *dst++ = c;
        src++;
    }

    if(token_started && (dst - cmd) < SH_CMD_SIZE - 1) *dst = '\0';
    argv[argc] = NULL;
    return argc;
}

void Shell_Init(void) {
	screen_init();
	SYS_Init();
	env_init();
}

void Shell_Run(void) {
    screen_seek(0, SEEK_SET, UNIT_CHAR);
	screen_puts("Mini-Badge Shell\n");
	screen_puts("Copyright (C) Zostime. Released under MIT License.\n\n");
	char cur_path[MAX_APP_PATH] = "0:/root";
	size_t cur_offset = screen.offset;
	bool refresh_screen = false;
	while (1)
	{   
		refresh_screen = false;
		screen_seek(cur_offset, SEEK_SET, UNIT_BYTE);
		/* 显示路径与提示符 */ {	
			const char *pwd = env_getenv("PWD");
			char display_path[MAX_APP_PATH];
			if(strncmp(pwd, "0:/root", 7) == 0) {
				snprintf(display_path, sizeof(display_path), "~%s", pwd + 7);
			} else {
				// 去掉开头"0:", 只显示'/'和其余部分
				if(strncmp(pwd, "0:", 2) == 0) {
					snprintf(display_path, sizeof(display_path), "%s", pwd + 2);
				} else {
					snprintf(display_path, sizeof(display_path), "%s", pwd);
				}
			}
			screen_printf("\033[37m%s\033[31m#\033[0m ", display_path);
		}
		
		uint8_t cur_lines = count_screen_lines();
		if(cur_lines > SCREEN_MAX_LINES) {
			refresh_screen = true;
			for(uint8_t i=0; i < cur_lines-SCREEN_MAX_LINES; i++) {
				screen_scrollback(SCROLL_UP);
			}
		}
		
		if(refresh_screen) LCD_Clear(BLACK);
		SYS_Printf(0,0,WHITE,BLACK,"%s",screen.buf);
		
		/* INPUT */ {
			char input[SH_CMD_SIZE];
			if (CDC_ReadLine(input, sizeof(input)) >= 0) {
				screen_printf("%s\n",input);
				SYS_Printf(0,0,WHITE,BLACK,"%s",screen.buf);
				cur_offset = screen.offset;

				/* Shell */
				char expanded[SH_CMD_SIZE];
				shell_expand_vars(input, expanded, sizeof(expanded));
									
				char *argv[SH_MAX_ARGS+2];
				int argc = shell_parse(expanded, argv, SH_MAX_ARGS+2);
				if(!argc) continue; // 无命令
				// 内建命令
				if(!strcmp(argv[0], "cd")) 
				{
					char dir_path[SH_MAX_PATH];
					if(argc == 1) strcpy(dir_path, "~"); // 仅有命令, 回到HOME
					else if(argc == 2) strcpy(dir_path, argv[1]);
					else {
						screen_puts("cd: too many arguments\n"); 
						cur_offset = screen.offset;
						continue;
					}
					
					// 路径展开
					char new_path[SH_MAX_PATH];
					path_expand(dir_path, cur_path, new_path);

					// 解析 (IDK为什么f_stat解析根目录是 FR_INVALID_NAME QAQ)
					DIR dir;
					if (f_opendir(&dir, new_path) == FR_OK) {
						f_closedir(&dir);
						strcpy(cur_path, new_path);
						env_set("PWD", new_path); 
					} 
					else {
						FIL file;
						if (f_open(&file, new_path, FA_READ) == FR_OK) {
							f_close(&file);
							screen_printf("cd: %s: Not a directory\n", dir_path);
						} 
						else screen_printf("cd: %s: No such file or directory\n", dir_path);
					}
					cur_offset = screen.offset;
					continue;
				}
				else if(!strcmp(argv[0], "echo")) 
				{
					bool option_end = false;
					bool newline = true;
					bool interpret_escapes = false;
					for(size_t i=1; i < argc; i++) {
						bool unknown_option = false;
						bool arg_parsing = false;
						if(argv[i][0]=='-' && argv[i][1]!='\0' && !option_end) {
							arg_parsing = true;
							if(!strcmp(argv[i], "--")) {option_end = true; continue;}
							bool _newline = newline;
							bool _interpret_escapes = interpret_escapes;
							size_t arglen = strlen(argv[i]);
							for(size_t j=1;j < arglen; j++) {
								switch(argv[i][j]) {
									case 'n': _newline=false; continue;
									case 'e': _interpret_escapes=true; continue;
									case 'E': _interpret_escapes=false; continue;
								}
								unknown_option = true;
								option_end = true;
								break;
							}
							if(!unknown_option) {
								newline = _newline;
								interpret_escapes = _interpret_escapes;
							}
						}
						if(!arg_parsing || unknown_option) {
							option_end = true;
						    if(interpret_escapes) {						
								for(const char *p = argv[i]; *p; p++) {
									if(*p == '\\') {
										if(p[1] == '\0') {screen_puts("\\"); break;}
										p++;
										switch (*p) {
											case 'n':  screen_puts("\n"); break;
											case 't':  screen_puts("\t"); break;
											case 'r':  screen_puts("\r"); break;
											case '\\': screen_puts("\\"); break;

											default: 
												screen_putc('\\');  
												screen_putc(*p); 
												break; // 未知转义
										}
									}
									else screen_putc(*p); 
								}
							}
							else screen_puts(argv[i]);
							if(i < argc-1) screen_puts(" ");
						}
					}
					if(newline) screen_puts("\n");
					cur_offset = screen.offset;
					continue;
				}
				else if(!strcmp(argv[0], "pwd")) 
				{
					screen_printf("%s\n", cur_path);
					cur_offset = screen.offset;
					continue;
				}
				else if(!strcmp(argv[0], "unset")) 
				{
					for(size_t i=1; i < argc; i++) {
						env_unset(argv[i]);
					}
					continue;
				}
				// ...
				else	// 外部命令 
				{ 
					// 由路径跳转到exe
					char new_path[SH_MAX_PATH];
					if(path_expand(argv[0], cur_path, new_path)) {
						DIR dir;
						FIL file;
						if(f_open(&file, new_path, FA_READ) == FR_OK) {
							f_close(&file);
							// JMP exe
							BOOTLOADER_REQUEST_APP(new_path);
						} 
						else{
							if(f_opendir(&dir, new_path) == FR_OK) {
								f_closedir(&dir);
								screen_printf("%s: Is a directory\n", argv[0]);
								cur_offset = screen.offset;
								continue;
							} 
						}						
					}
					// 由PATH跳转到exe
					const char *path_env = env_getenv("PATH");
					if(path_env && *path_env) {
						char path_copy[SH_MAX_PATH];
						strncpy(path_copy, path_env, sizeof(path_copy) - 1);
						path_copy[sizeof(path_copy) - 1] = '\0';

						char *dir = path_copy;   
						while(dir && *dir) {
							// 查找下一个冒号
							char *colon = strchr(dir, ':');
							if(colon) {
								*colon = '\0';         
							}

							// 忽略空目录
							if(*dir != '\0') {
								char candidate[SH_MAX_PATH];
								snprintf(candidate, sizeof(candidate), "%s/%s", dir, argv[0]);

								if(path_expand(candidate, cur_path, new_path)) {
									FIL file;
									if(f_open(&file, new_path, FA_READ) == FR_OK) {
										f_close(&file);
										BOOTLOADER_REQUEST_APP(new_path);
										break;
									}
								}
							}

							if(!colon) break;        
							dir = colon + 1;
						}
					}
					screen_printf("%s: command not found\n", argv[0]);
					cur_offset = screen.offset;
					continue;
				}
			}		
		}
	}
}
