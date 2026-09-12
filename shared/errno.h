#ifndef __ERRNO_H__
#define __ERRNO_H__

typedef int	err_t;

#define EOK			0	/* No errors */
#define ENOTSET    	1	/* Variable not set */
#define ENOENT  	2	/* No such file or directory */
#define EINVAL     	4   /* Invalid parameter */
#define ENOSPC   	8   /* No space left on device */

#endif
