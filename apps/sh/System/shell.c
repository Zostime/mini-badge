#include "main.h"

#include "GUI.h"
#include "ff.h"
#include "rtc_utils.h"
#include "usbd_cdc_if.h"

#include "bootloader_api.h"

#include "shell.h"
#include "system.h"
#include "screen.h"

#include <stdbool.h>

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

static bool path_normalize(const char *src, char *dst)
{
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
bool path_expand(const char *input, const char *cur, char *out)
{
    char temp[SH_MAX_PATH];

    if (input[0] == '\0') {
        strcpy(out, cur);
        return true;
    }

    // 处理home简写 "~"
    if (input[0] == '~') {
        snprintf(temp, sizeof(temp), "0:/root%s", input + 1);
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
 * @brief  解析命令行, 支持引号, 转义, 变量展开
 * @param  cmd :输入命令字符串
 * @param  argv :输出参数数组，末尾为 NULL
 * @param  max_args :最大参数个数 (含命令和 NULL 哨兵)
 * @retval 实际参数个数
 */
int shell_parse(char *cmd, char *argv[], int max_args)
{
    int argc = 0;
    char *src = cmd;      // 源指针
    char *dst = cmd;      // 目标指针（原地写入）
    int in_squote = 0;
    int in_dquote = 0;
    int token_started = 0;

    while (*src)
    {
        char c = *src;

        // 反斜杠转义
        if (c == '\\' && !in_squote) {
            src++;
            if (*src == '\0') break;
            if (!token_started) {
                argv[argc++] = dst;   // 记录 token 起始位置
                token_started = 1;
            }
            *dst++ = *src++;          // 复制被转义的字符
            continue;
        }

        // 双引号
        if (c == '"' && !in_squote) {
            in_dquote = !in_dquote;
            src++;
            continue;
        }

        // 单引号
        if (c == '\'' && !in_dquote) {
            in_squote = !in_squote;
            src++;
            continue;
        }

        // 分隔符
        if ((c == ' ' || c == '\t' || 
			c == '\r' || c == '\n') &&
            !in_squote && !in_dquote) 
			{
            if (token_started) {
                *dst++ = '\0';	// 写入结束符并跳过
                token_started = 0;
            }
            src++;
            continue;
        }

        // 普通字符
        if (!token_started) {
            argv[argc++] = dst;	// 记录新 token 起始位置
            token_started = 1;
        }
        *dst++ = c;
        src++;
    }

    if (token_started) *dst='\0';

    argv[argc] = NULL;
    return argc;
}

void Shell_Init(void) {
	screen_init();
	SYS_Init();
}

void Shell_Run(void) {
    screen_seek(0, SEEK_SET, UNIT_CHAR);
	screen_puts("Mini-Badge Shell\n");
	screen_puts("Copyright (C) Zostime. Released under MIT License.\n\n");
	char cur_path[MAX_APP_PATH] = "0:/root";
	size_t cur_offset = screen.offset;
	while (1)
	{   
		screen_seek(cur_offset, SEEK_SET, UNIT_BYTE);
		/* 显示路径与提示符 */ {	
			char display_path[MAX_APP_PATH];
			if (strncmp(cur_path, "0:/root", 7) == 0) {
				snprintf(display_path, sizeof(display_path), "~%s", cur_path + 7);
			} else {
				// 去掉开头"0:", 只显示'/'和其余部分
				if (strncmp(cur_path, "0:", 2) == 0) {
					snprintf(display_path, sizeof(display_path), "%s", cur_path + 2);
				} else {
					snprintf(display_path, sizeof(display_path), "%s", cur_path);
				}
			}
			screen_printf("\033[37m%s\033[31m#\033[0m ", display_path);
			SYS_Printf(0,0,WHITE,BLACK,"%s",screen.buf);
		}
		
		/* INPUT */ {
			char input[SH_CMD_SIZE];
			if (CDC_ReadLine(input, sizeof(input)) >= 0) {
				screen_printf("%s\n",input);
				SYS_Printf(0,0,WHITE,BLACK,"%s",screen.buf);
				cur_offset = screen.offset;

				/* Shell */
				char *argv[SH_MAX_ARGS+2];
				int argc = shell_parse(input, argv, SH_MAX_ARGS+2);
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
				// ...
				else	// 外部命令 
				{ 
				
				}		
			}
		}
	}
}
