#include "kernel.h"
#include "bootloader_api.h"
#include "sys_path.h"
#include "ff.h"
#include <stdio.h>

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

    char line[KERNEL_LINE_MAX];
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
