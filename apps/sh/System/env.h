#ifndef __ENV_H__
#define __ENV_H__

#include "errno.h"

#define ENV_MAX 8
#define ENV_NAME_MAX  16
#define ENV_VALUE_MAX 64

typedef struct {
    char name[ENV_NAME_MAX];
    char value[ENV_VALUE_MAX];
} env_var_t;

void env_init(void);
const char *env_getenv(const char *name);
err_t env_set(const char *name, const char *value);
err_t env_unset(const char *name);

#endif
