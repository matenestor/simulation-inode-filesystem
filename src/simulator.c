#include <stdio.h>
#include <string.h>
#include <libgen.h>

#include "simulator.h"
#include "fs_api.h"
#include "commands/commands.h"

#include "logger.h"
#include "errors.h"


static void init_prompt() {
	// create initial pwd, 2 for null terminator
	strncpy(buff_pwd, SEPARATOR, 2);
	// create prompt
	snprintf(buff_prompt, BUFFER_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);
}

static int handle_input(char* command, char* arg1, char* arg2) {
	int ret = RETURN_FAILURE;

	// discard char
	static int c = 0;
	// program buffer for user's inputs; 3x more for 'command', 'arg1' and 'arg2'
	static char buff_in[3 * BUFFER_INPUT_LENGTH] = {0};

	// receive valid input from user, prevent empty of large input
	if ((fgets(buff_in, 3 * BUFFER_INPUT_LENGTH, stdin) != NULL)) {
		// prevent overflowing input, so it doesn't go to next cycle
		if (isoverflow(buff_in[3 * BUFFER_INPUT_LENGTH - 2])) {
			// discard everything in stdin and move on
			while ((c = fgetc(stdin)) != '\n' && c != EOF);
			puts("input too large");
		}
		// input from user is ok, so scan it
		else if (sscanf(buff_in, "%s %s %s", command, arg1, arg2) > 0) {
			// in case some word is longer than BUFFER_INPUT_LENGTH
			command[BUFFER_INPUT_LENGTH - 1] = '\0';
			arg1[BUFFER_INPUT_LENGTH - 1] = '\0';
			arg2[BUFFER_INPUT_LENGTH - 1] = '\0';
			ret = RETURN_SUCCESS;
		}
	}
	else {
		// empty input (EOF)
		puts("");
	}

	return ret;
}

static void sim_format_(const char* arg1) {
	if (sim_format(arg1, fs_name) == RETURN_FAILURE) {
		is_formatted = false;
		my_perror(CMD_FORMAT);
		reset_myerrno();
	} else {
		is_formatted = true;
		init_prompt();
	}
}

static void sim_help() {
	puts(PR_USAGE);
}

static void sim_exit() {
	close_filesystem();
	is_running = false;
}

int init_simulation(const char* path) {
	int ret = RETURN_FAILURE;
	char fs_path[strlen(path) + 1];
	char* p_basename = NULL;
	// basename modifies its argument -- puts \0 after end of the base name
	// in path and returns pointer on position where the base name starts...
	strncpy(fs_path, path, strlen(path) + 1);
	p_basename = basename(fs_path);

	strncpy(fs_name, p_basename, strlen(p_basename) + 1);

	if (strlen(p_basename) < STRLEN_FS_NAME) {
		init_prompt();
		ret = init_filesystem(fs_path, &is_formatted);
	}
	else {
		set_myerrno(Err_fs_name_long);
		fprintf(stderr, "! Maximal length of filesystem name is %d characters.\n", STRLEN_FS_NAME);
	}
	return ret;
}

static int get_command_id(const char* command) {
	if (strcmp(command, CMD_PWD) == 0)		return CMD_PWD_ID;
	if (strcmp(command, CMD_CAT) == 0)		return CMD_CAT_ID;
	if (strcmp(command, CMD_LS) == 0)		return CMD_LS_ID;
	if (strcmp(command, CMD_INFO) == 0)		return CMD_INFO_ID;
	if (strcmp(command, CMD_MV) == 0)		return CMD_MV_ID;
	if (strcmp(command, CMD_CP) == 0)		return CMD_CP_ID;
	if (strcmp(command, CMD_RM) == 0)		return CMD_RM_ID;
	if (strcmp(command, CMD_CD) == 0)		return CMD_CD_ID;
	if (strcmp(command, CMD_MKDIR) == 0)	return CMD_MKDIR_ID;
	if (strcmp(command, CMD_RMDIR) == 0)	return CMD_RMDIR_ID;
	if (strcmp(command, CMD_INCP) == 0)		return CMD_INCP_ID;
	if (strcmp(command, CMD_OUTCP) == 0)	return CMD_OUTCP_ID;
	if (strcmp(command, CMD_DU) == 0)		return CMD_DU_ID;
	if (strcmp(command, CMD_LOAD) == 0)		return CMD_LOAD_ID;
	if (strcmp(command, CMD_FSCK) == 0)		return CMD_FSCK_ID;
	if (strcmp(command, CMD_CORRUPT) == 0)	return CMD_CORRUPT_ID;
	if (strcmp(command, CMD_FORMAT) == 0)	return CMD_FORMAT_ID;
	if (strcmp(command, CMD_HELP) == 0)		return CMD_HELP_ID;
	if (strcmp(command, CMD_EXIT) == 0)		return CMD_EXIT_ID;
	if (strcmp(command, CMD_DEBUG) == 0)	return CMD_DEBUG_ID;
	return CMD_UNKNOWN_ID;
}

/*
 *  In simulation cycle handles input from user, if in correct form,
 *  function checks if given command is known and calls it.
 *  If filesystem was not formatted yet, user is allowed to use only
 *  'format', 'help' and 'exit' commands. After formatting, user may use
 *  all other commands.
 */
void run() {
	int error = RETURN_FAILURE;
	int cmd_id = 0;
	char command[BUFFER_INPUT_LENGTH] = {0};
	char arg1[BUFFER_INPUT_LENGTH] = {0};
	char arg2[BUFFER_INPUT_LENGTH] = {0};
	is_running = true;

	while (is_running) {
		fputs(buff_prompt, stdout);

		// clear buffers, so command doesn't accidentally receive
		// wrong argument(s) from previous iteration
		BUFFER_CLEAR(command);
		BUFFER_CLEAR(arg1);
		BUFFER_CLEAR(arg2);

		if (handle_input(command, arg1, arg2) == RETURN_SUCCESS) {
			cmd_id = get_command_id(command);

			// only basic commands allowed before formatting
			switch (cmd_id) {
				case CMD_FORMAT_ID: sim_format_(arg1);	continue;
				case CMD_HELP_ID:	sim_help();			continue;
				case CMD_EXIT_ID:	sim_exit();			continue;
				default:
					if (!(is_formatted || cmd_id == CMD_UNKNOWN_ID)) {
						puts("Filesystem not formatted. You can format "
							 "one with command 'format <size>'.");
						continue;
					}
			}

			// all commands allowed, when filesystem is formatted
			switch (cmd_id) {
				case CMD_PWD_ID:	error = sim_pwd();				break;
				case CMD_CAT_ID:	error = sim_cat(arg1);			break;
				case CMD_LS_ID:		error = sim_ls(arg1);			break;
				case CMD_INFO_ID:	error = sim_info(arg1);			break;
				case CMD_MV_ID:		error = sim_mv(arg1, arg2);		break;
				case CMD_CP_ID:		error = sim_cp(arg1, arg2);		break;
				case CMD_RM_ID:		error = sim_rm(arg1);			break;
				case CMD_CD_ID:		error = sim_cd(arg1);			break;
				case CMD_MKDIR_ID:	error = sim_mkdir(arg1);		break;
				case CMD_RMDIR_ID:	error = sim_rmdir(arg1);		break;
				case CMD_INCP_ID:	error = sim_incp(arg1, arg2);	break;
				case CMD_OUTCP_ID:	error = sim_outcp(arg1, arg2);	break;
				case CMD_DU_ID:		error = sim_du(arg1, arg2);		break; // TODO
				case CMD_LOAD_ID:	error = sim_load(arg1);			break;
				case CMD_FSCK_ID:	error = sim_fsck();				break; // TODO
				case CMD_CORRUPT_ID:error = sim_corrupt();			break; // TODO
				case CMD_DEBUG_ID:	error = sim_debug(arg1, arg2);	break;
				default:
					puts("-zos: command not found");
			}

			if (error == RETURN_FAILURE && cmd_id != CMD_UNKNOWN_ID) {
				my_perror(command);
				reset_myerrno();
			}
		}
	}

	log_info("Simulation end.");
}
