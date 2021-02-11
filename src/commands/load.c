#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"

#include "errors.h"
#include "logger.h"

// TODO from simulator.h -- put it to fs_config.h together with macros from format.c
#define BUFFER_INPUT_LENGTH		2048


int perform_command(const char* command, const char* arg1, const char* arg2) {
	// allowed commands
	if (strcmp(command, CMD_PWD) == 0)		return sim_pwd();
	if (strcmp(command, CMD_CAT) == 0)		return sim_cat(arg1);
	if (strcmp(command, CMD_LS) == 0)		return sim_ls(arg1);
	if (strcmp(command, CMD_INFO) == 0)		return sim_info(arg1);
	if (strcmp(command, CMD_MV) == 0)		return sim_mv(arg1, arg2);
	if (strcmp(command, CMD_CP) == 0)		return sim_cp(arg1, arg2);
	if (strcmp(command, CMD_RM) == 0)		return sim_rm(arg1);
	if (strcmp(command, CMD_CD) == 0)		return sim_cd(arg1);
	if (strcmp(command, CMD_MKDIR) == 0)	return sim_mkdir(arg1);
	if (strcmp(command, CMD_RMDIR) == 0)	return sim_rmdir(arg1);
	if (strcmp(command, CMD_INCP) == 0)		return sim_incp(arg1, arg2);
	if (strcmp(command, CMD_OUTCP) == 0)	return sim_outcp(arg1, arg2);
	if (strcmp(command, CMD_FSCK) == 0)		return sim_fsck();

	// forbidden or unknown commands
	if (strcmp(command, CMD_LOAD) == 0
			|| strcmp(command, CMD_FORMAT) == 0
			|| strcmp(command, CMD_HELP) == 0
			|| strcmp(command, CMD_EXIT) == 0) {
		printf("FORBIDDEN ");
	} else {
		printf("UNKNOWN ");
	}
	return RETURN_FAILURE;
}


/*
 * Load and process given file with simulation commands.
 */
int sim_load(const char* path) {
	log_info("load: [%s]", path);

	int ret;
	FILE* fp = NULL;
	char* line = NULL;
	size_t len = 0;
	ssize_t read = 0;
	char command[BUFFER_INPUT_LENGTH] = {0};
	char arg1[BUFFER_INPUT_LENGTH] = {0};
	char arg2[BUFFER_INPUT_LENGTH] = {0};

	if (strlen(path) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	if ((fp = fopen(path, "r")) == NULL) {
		set_myerrno(Err_os_open_file);
		goto fail;
	}

	// read lines from file
	while (getline(&line, &len, fp) != -1) {
		if (sscanf(line, "%s %s %s", command, arg1, arg2) > 0) {
			// in case some word is longer than BUFFER_INPUT_LENGTH
			command[BUFFER_INPUT_LENGTH - 1] = '\0';
			arg1[BUFFER_INPUT_LENGTH - 1] = '\0';
			arg2[BUFFER_INPUT_LENGTH - 1] = '\0';

			printf("performing: %s %s %s\n", command, arg1, arg2);
			ret = perform_command(command, arg1, arg2);
			printf("...%s\n", ret == RETURN_SUCCESS ? "OK" : "FAIL");

			memset(command, '\0', sizeof(command));
			memset(arg1, '\0', sizeof(arg1));
			memset(arg2, '\0', sizeof(arg2));
		}
	}

	fclose(fp);
	if (line)
		free(line);
	return RETURN_SUCCESS;

fail:
	fclose(fp);
	if (line)
		free(line);
	log_warning("load: unable to perform operations from [%s]", path);
	return RETURN_FAILURE;
}
