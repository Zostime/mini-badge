#include "ff.h"
#include "env.h"
#include <string.h>
#include "sys_path.h"

static env_var_t env_table[ENV_MAX];
static int env_count = 0;

/**
 * @brief  从指定文件加载环境变量(格式：KEY="value",支持 #注释)
 * @param  path: 文件路径，例如 "0:/etc/environment"
 * @retval 成功加载的变量数, 负数表示错误
 */
int env_load(const char *path)
{
    FIL file;
    char line[128];
    int loaded = 0;

    if (f_open(&file, path, FA_READ) != FR_OK) {
        return -1;   // 文件打开失败
    }

    while (f_gets(line, sizeof(line), &file)) {
        // 去除行尾换行符
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        // 跳过空行和注释
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#') continue;

        // 查找 '='
        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';          // 分割 KEY 和 value 部分
        char *key = p;
        char *value = eq + 1;

        len = strlen(key);
        while (len > 0 && (key[len-1] == ' ' || key[len-1] == '\t')) {
            key[--len] = '\0';
        }

        while (*value == ' ' || *value == '\t') value++;

        if (*value == '"') {
            value++;
            char *end_quote = strchr(value, '"');
            if (end_quote) *end_quote = '\0';
        }

        // 存储到环境变量
        if (*key && *value) {
            if (env_set(key, value) == 0) {
                loaded++;
            }
        }
    }

    f_close(&file);
    return loaded;
}

void env_init(void)
{
    memset(env_table, 0, sizeof(env_table));
    env_count = 0;

	env_set("USER", "root");
    env_set("HOME", PATH_HOME);
    env_set("TERM", "Mini-Badge");
	env_set("PWD", 	PATH_HOME);
	env_load("0:/etc/environment");	// PATH
}

const char *env_getenv(const char *name)
{
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_table[i].name, name) == 0) {
            return env_table[i].value;
        }
    }
    return NULL;
}

err_t env_set(const char *name, const char *value)
{
    // 如果变量已存在, 更新其值
    for (int i = 0; i < env_count; i++) {
        if (strcmp(env_table[i].name, name) == 0) {
            strncpy(env_table[i].value, value, ENV_VALUE_MAX - 1);
            env_table[i].value[ENV_VALUE_MAX - 1] = '\0';
            return 0;
        }
    }

    // 变量不存在,添加新项
    if (env_count >= ENV_MAX) {
        return ENOSPC;   // 表满
    }

    strncpy(env_table[env_count].name, name, ENV_NAME_MAX - 1);
    env_table[env_count].name[ENV_NAME_MAX - 1] = '\0';

    strncpy(env_table[env_count].value, value, ENV_VALUE_MAX - 1);
    env_table[env_count].value[ENV_VALUE_MAX - 1] = '\0';

    env_count++;
    return 0;
}

/**
 * @brief  删除环境变量
 * @param  name: 变量名
 * @retval 0 = 成功, ENOTSET = 变量不存在
 */
err_t env_unset(const char *name)
{
    for(int i=0; i < env_count; i++) {
        if(strcmp(env_table[i].name, name) == 0) {
            // 将后续元素前移
            for(int j=i; j < env_count-1; j++) {
                env_table[j] = env_table[j+1];
            }
            env_count--;
            // 清空最后一个元素
            memset(&env_table[env_count], 0, sizeof(env_var_t));
            return 0;
        }
    }
    return ENOTSET;
}
