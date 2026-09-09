#ifndef __SHELL_H__
#define __SHELL_H__

#define HOME_PATH "0:/root"

#define SH_CMD_SIZE 164
#define SH_MAX_ARGS 3
#define SH_MAX_PATH 84

#define SCROLL_UP 0
#define SCROLL_DOWN 1

#define SH_SCROLL_HISTORY_PATH "0:/tmp/.scroll_history"

void Shell_Init(void);
void Shell_Run(void);

#endif
