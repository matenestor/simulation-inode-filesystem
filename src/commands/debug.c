#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"


static int print_links(const char* type, const uint32_t* links, const size_t links_count) {
	printf("%s", type);
	for (size_t i = 0; i < links_count; ++i) {
		printf(" %d", links[i]);
	}
	puts("");
	return 0;
}

static void inode(const uint32_t id) {
	struct inode inode_item;
	fs_read_inode(&inode_item, 1, id);
	printf(" type: %s\n", inode_item.inode_type == Inode_type_dirc ? "directory"
						  : inode_item.inode_type == Inode_type_file ? "file" : "free");
	printf(" size: %d B = %.2f kB = %.2f MB\n",
		   inode_item.file_size,
		   (double) inode_item.file_size / 1024,
		   (double) inode_item.file_size / 1024 / 1024);
	printf(" inode id: %d\n", inode_item.id_inode);
	print_links(" direct links:", inode_item.direct, COUNT_DIRECT_LINKS);
	print_links(" indrct links lvl 1:", inode_item.indirect_1, COUNT_INDIRECT_LINKS_1);
	print_links(" indrct links lvl 2:", inode_item.indirect_2, COUNT_INDIRECT_LINKS_2);
}

static void block_dirs(const uint32_t id) {
	struct directory_item block[sb.count_dir_items];
	fs_read_directory_item(block, sb.count_dir_items, id);
	for (size_t i = 0; i < sb.count_dir_items; ++i) {
		if (strcmp(block[i].item_name, "") != 0) {
			printf("%d\t%s\n", block[i].id_inode, block[i].item_name);
		}
	}
}

static void block_data(const uint32_t id) {
	char block[sb.block_size + 1];
	fs_read_data(block, sb.block_size, id);
	block[sb.block_size] = '\0';
	printf("%s", block);
}

static void block_links(const uint32_t id) {
	uint32_t block[sb.count_links];
	fs_read_link(block, sb.count_links, id);
	for (size_t i = 0; i < sb.count_links; ++i) {
		if (block[i] != FREE_LINK) {
			printf("%d ", block[i]);
		}
	}
	puts("");
}

int sim_debug(const char* flag, const char* str_id) {
	long id = strtol(str_id, NULL, 10);

	if (strcmp(flag, "i") == 0) {
		inode(id);
	}
	else if (strcmp(flag, "b") == 0) {
		block_dirs(id);
	}
	else if (strcmp(flag, "d") == 0) {
		block_data(id);
	}
	else if (strcmp(flag, "l") == 0) {
		block_links(id);
	}
	else {
		puts("i -- inodes\n"
			"b -- directories\n"
			"d -- data\n"
			"l -- links");
	}

	return 0;
}
