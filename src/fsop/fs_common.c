#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fs_common.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "fsop/fs_io.h"
#include "fsop/fs_bitmap.h"
#include "inode.h"
#include "utils.h"

#include "../include/logger.h"
#include "../include/errors.h"


void init_filesystem(const char* fsp, bool* is_formatted) {
	log_info("Loading filesystem [%s].", fsp);

	// if filesystem exists, load it
	if (access(fsp, F_OK) == 0) {
		// filesystem is ready to be loaded
		if ((filesystem = fopen(fsp, "rb+")) != NULL) {
			fs_read_superblock(&sb, 1);		// cache super block
			fs_seek_inodes(0);				// move to inodes location
			fs_read_inode(&in_actual, 1);	// cache root inode

			*is_formatted = true;
			puts("Filesystem loaded successfully.");

			log_info("Filesystem [%s] loaded.", fsp);
		}
		// filesystem file couldn't be open
		else {
			*is_formatted = false;
			puts("Filesystem corrupted or error while loading it. "
				"Restart simulation or format it again.");
			my_perror("System error:");
			reset_myerrno();
		}
	}
	// else notify about possible formatting
	else {
		*is_formatted = false;
		puts("No filesystem with this name found. "
	   		"You can format one with command 'format <size>'.");

		log_info("Filesystem [%s] not found.", fsp);
	}
}

void close_filesystem() {
	if (filesystem != NULL) {
		fclose(filesystem);
		log_info("Filesystem closed.");
	}
}
