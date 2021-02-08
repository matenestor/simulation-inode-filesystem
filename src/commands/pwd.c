#include <stdio.h>

#include "fs_cache.h"
#include "errors.h"


/*
 * Print current working directory.
 */
int sim_pwd() {
	puts(buff_pwd);

	return RETURN_SUCCESS;
}
