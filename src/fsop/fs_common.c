#include <stdbool.h>
#include <unistd.h>

#include "fs_api.h"
#include "fs_cache.h"

#include "logger.h"
#include "errors.h"


// filesystem file, which is being worked with
FILE* filesystem;

extern size_t fs_read_superblock(struct superblock*);


int init_filesystem(const char* fsp, bool* is_formatted) {
	int ret = RETURN_FAILURE;
	log_info("Loading filesystem [%s].", fsp);

	// if filesystem exists, load it
	if (access(fsp, F_OK) == 0) {
		// filesystem is ready to be loaded
		if ((filesystem = fopen(fsp, "rb+")) != NULL) {
			fs_read_superblock(&sb);			// cache super block
			fs_read_inode(&in_actual, 1, 1);	// cache root inode

			*is_formatted = true;
			puts("Filesystem loaded successfully.");

			log_info("Filesystem [%s] loaded.", fsp);
			ret = RETURN_SUCCESS;
		}
		// filesystem file couldn't be open
		else {
			*is_formatted = false;
			printf("Error while loading filesystem [%s].", fsp);
			my_perror("System error");
			reset_myerrno();
			log_critical("Filesystem [%s] invalid.", fsp);
		}
	}
	// else notify about possible formatting
	else {
		*is_formatted = false;
		puts("No filesystem with this name found. "
	   		"You can format one with command 'format <size>'.");

		log_info("Filesystem [%s] not found.", fsp);
		ret = RETURN_SUCCESS;
	}
	return ret;
}

void close_filesystem() {
	if (filesystem != NULL) {
		fclose(filesystem);
		log_info("Filesystem closed.");
	}
}
