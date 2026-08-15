#ifndef __SHELL_H__
#define __SHELL_H__

#define HOME_PATH "0:/root"

#define SH_CMD_SIZE 64
#define SH_MAX_ARGS 3
#define SH_MAX_PATH 84

void Shell_Init(void);
void Shell_Run(void);

#endif
