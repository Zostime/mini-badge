#ifndef __ERRNO_H__
#define __ERRNO_H__

typedef int	err_t;

#define ENOTSET    	1	/* Variable not set */
#define ENOENT  	2	/* No such file or directory */
#define EIO			5	/* Input/output error */
#define ENOEXEC 	8 	/* Exec format error */
#define EBADF		9	/* Bad file descriptor */
#define EINVAL     	22  /* Invalid argument */
#define ENOSPC   	28  /* No space left on device */

#endif
