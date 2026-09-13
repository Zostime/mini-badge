#include "kernel.h"
#include "bootloader_api.h"
#include "spi_sdcard.h"
#include "sys_path.h"
#include "ff.h"
#include <stdio.h>
#include <stdlib.h>

screeninfo_t screen_info = {
    .xres = 240,
    .yres = 135,
};

FATFS sSDCARD_FatFs;

err_t execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (pathname == NULL || pathname[0] == '\0') return EINVAL;
    if (argv == NULL) return EINVAL;

    FIL file;
    if (f_open(&file, pathname, FA_READ) != FR_OK) {
        return ENOENT;
    }
    f_close(&file);

    int argc = 0;
    while (argv[argc] != NULL) {
        argc++;
    }

    char line[EXEC_ARG_MAX];
    UINT bw;
    int n;

    if (f_open(&file, PATH_RUN_ARGS, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        return ENOENT;
    }

    // PATH_RUN_ARGS.first_line -> argc
    n = snprintf(line, sizeof(line), "%d\n", argc);
    if (f_write(&file, line, n, &bw) != FR_OK || bw != (UINT)n) {
        f_close(&file);
        f_unlink(PATH_RUN_ARGS);
        return EIO;
    }

    // \n argv[i]
    for (int i = 0; i < argc; i++) {
        n = snprintf(line, sizeof(line), "%s\n", argv[i]);
        if (f_write(&file, line, n, &bw) != FR_OK || bw != (UINT)n) {
            f_close(&file);
            f_unlink(PATH_RUN_ARGS);
            return EIO;
        }
    }
    f_close(&file);

    // envp -> PATH_RUN_ENV
    if (f_open(&file, PATH_RUN_ENV, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
        f_unlink(PATH_RUN_ARGS);  
        return ENOENT;
    }

    if (envp != NULL) {
        for (int i = 0; envp[i] != NULL; i++) {
            n = snprintf(line, sizeof(line), "%s\n", envp[i]);
            if (f_write(&file, line, n, &bw) != FR_OK || bw != (UINT)n) {
                f_close(&file);
                f_unlink(PATH_RUN_ARGS);
                f_unlink(PATH_RUN_ENV);
                return EIO;
            }
        }
    }
    f_close(&file);

    BOOTLOADER_REQUEST_APP(pathname);
	
	// JMP FAIL
	f_unlink(PATH_RUN_ARGS);
	f_unlink(PATH_RUN_ENV);
    return ENOEXEC;
}
err_t execve_load(int *argc, char *argv[], char *envp[])
{
	SD_Init();
	
	FRESULT SD_res;
    SD_res = f_mount(&sSDCARD_FatFs, "0:", 0);
    if (SD_res != FR_OK) {
		BYTE work[512];
        SD_res = f_mkfs("0:", 0, work, sizeof(work));
        if (SD_res == FR_OK) {
            SD_res = f_mount(&sSDCARD_FatFs, "0:", 1);
        }
    }	
	
    static char arg_str_buf[EXEC_MAX_ARGS][EXEC_ARG_MAX];
    static char env_str_buf[EXEC_MAX_ENVS][EXEC_ENV_MAX];

    FIL file;
    char line[EXEC_ARG_MAX];

    *argc = 0;
    argv[0] = NULL;

    if (f_open(&file, PATH_RUN_ARGS, FA_READ) != FR_OK) {
        if (envp) envp[0] = NULL;
        return ENOENT;
    }

    // first_line -> argc
    if (!f_gets(line, sizeof(line), &file)) {
        f_close(&file);
        if (envp) envp[0] = NULL;
        return EBADF;
    }

    int n = atoi(line);
    if (n < 0 || n > EXEC_MAX_ARGS) {
        f_close(&file);
        if (envp) envp[0] = NULL;
        return EINVAL;
    }

    for (int i = 0; i < n; i++) {
        if (!f_gets(line, sizeof(line), &file)) {
            *argc = i;
            argv[i] = NULL;
            f_close(&file);
            if (envp) envp[0] = NULL;
            return EBADF;
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        strncpy(arg_str_buf[i], line, EXEC_ARG_MAX - 1);
        arg_str_buf[i][EXEC_ARG_MAX - 1] = '\0';
        argv[i] = arg_str_buf[i];
    }
    *argc = n;
    argv[n] = NULL;
    f_close(&file);

    if (envp == NULL) return 0;
    envp[0] = NULL;

    if (f_open(&file, PATH_RUN_ENV, FA_READ) != FR_OK) {
        return 0;
    }

    int ec = 0;
    while (f_gets(line, sizeof(line), &file) && ec < EXEC_MAX_ENVS - 1) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        if (len == 0) continue;

        strncpy(env_str_buf[ec], line, EXEC_ENV_MAX - 1);
        env_str_buf[ec][EXEC_ENV_MAX - 1] = '\0';
        envp[ec] = env_str_buf[ec];
        ec++;
    }
    envp[ec] = NULL;

    f_close(&file);
    return 0;
}
