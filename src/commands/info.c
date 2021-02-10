#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "fs_api.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


static int print_links(const char* type, const uint32_t* links, const size_t links_count) {
	printf("%s", type);

	for (size_t i = 0; i < links_count; ++i) {
		printf(" %d", links[i]);
	}
	puts("");
	return 0;
}

/*
 * Print information about given inode.
 */
int sim_info(const char* item) {
	int ret = RETURN_FAILURE;
	struct inode inode_item = {0};

	if (strlen(item) > 0) {
		if (get_inode(&inode_item, item) != RETURN_FAILURE) {
			printf(" type: %s\n", inode_item.inode_type == Inode_type_dirc ? "directory" : "file");
			printf(" size: %d B = %.2f kB = %.2f MB\n",
						inode_item.file_size,
						(double) inode_item.file_size / 1024,
						(double) inode_item.file_size / 1024 / 1024);
			printf(" inode id: %d\n", inode_item.id_inode);
			print_links(" direct links:", inode_item.direct, COUNT_DIRECT_LINKS);
			print_links(" indirect links lvl 1:", inode_item.indirect_1, COUNT_INDIRECT_LINKS_1);
			print_links(" indirect links lvl 2:", inode_item.indirect_2, COUNT_INDIRECT_LINKS_2);

			ret = RETURN_SUCCESS;
		}
	} else {
		set_myerrno(Err_arg_missing);
		log_warning("info: unable to print info [%s]", item);
	}

	return ret;
}
