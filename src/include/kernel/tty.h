#ifndef __TTY_H
#define __TTY_H

#include <kernel/types.h>

extern ssize_t tty_read(char *buf, int size);
extern ssize_t tty_write(char *buf, int size);

#endif /* __TTY_H */
