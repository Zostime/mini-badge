#ifndef __ENV_H__
#define __ENV_H__

#define ENV_MAX 8

#define ENV_NAME_MAX  16
#define ENV_VALUE_MAX 64

typedef struct {
    char name[ENV_NAME_MAX];
    char value[ENV_VALUE_MAX];
} env_var_t;

void env_init(void);
const char *env_getenv(const char *name);
int env_set(const char *name, const char *value);
int env_unset(const char *name);

#endif
