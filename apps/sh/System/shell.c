#include "main.h"

#include "GUI.h"
#include "ff.h"
#include "rtc_utils.h"
#include "usbd_cdc_if.h"

#include "bootloader_api.h"

#include "shell.h"
#include "system.h"
#include "screen.h"

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

void path_normalize(const char *src, char *dst) {
    char stack[SH_MAX_PATH]; 
    char tmp[SH_MAX_PATH];
    char *token;

    // 保留"0:"前缀
    if (strncmp(src, "0:", 2) != 0) return;
    dst[0] = '0'; dst[1] = ':';
    dst += 2;

    strcpy(tmp, src + 2);
    // 如果路径为空则设为"/"
    if (tmp[0] == '\0') {
        strcpy(tmp, "/");
    }

    stack[0] = '\0';
    token = strtok(tmp, "/");
    while (token) {
        if (strcmp(token, ".") == 0) {
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
        strcpy(dst - 2, "0:/");    // 回到根目录
    } else {
        snprintf(dst - 2, SH_MAX_PATH - (dst - 2 - dst), "0:/%s", stack);
    }
}
/**
 * @brief  展开用户输入的路径为规范绝对路径
 * @param  input: 路径字符串
 * @param  cur: 当前工作目录
 * @param  out: 输出规范路径，容量至少 MAX_PATH
 * @retval 是否成功
 */
void path_expand(const char *input, const char *cur, char *out) {
    char temp[SH_MAX_PATH];

    // 处理 home 简写 "~"
    if (input[0] == '~') {
        snprintf(temp, sizeof(temp), "%s%s", HOME_PATH, input + 1);
    }
    // 绝对路径
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

    // 规范化路径: 处理".",".."
    path_normalize(temp, out);
}
/**
 * @brief  解析命令行字符串
 * @param  cmdline: 输入的命令行字符串
* @param  argv: 输出参数数组, argv[0] 为命令, 后续为参数, 末尾为哨兵
 * @param  max_args: 最大参数个数 (包括命令和哨兵)
 * @retval 实际分割出的参数个数
 */
int shell_parse(char *cmd, char *argv[], int max_args) {
    int argc = 0;
    char *token = strtok(cmd, " \t\n");   // ' ','\t','\n'作为分隔符
    while (token != NULL && argc < max_args) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
	argv[argc] = NULL;  // 结束标记
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
				
				}
				// ...
				else	// 外部命令 
				{ 
				
				}		
			}
		}
	}
}
