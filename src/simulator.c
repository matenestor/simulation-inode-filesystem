#include <stdio.h>
#include <string.h>

#include "simulator.h"
#include "commands/commands.h"
#include "utils.h"

#include "../include/logger.h"
#include "../include/errors.h"


static void init_prompt() {
	// create initial pwd, 2 for null terminator
	strncpy(buff_pwd, SEPARATOR, 2);
	// create prompt
	snprintf(buff_prompt, BUFF_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);
}

static int handle_input(char* command, char* arg1, char* arg2) {
	int ret = RETURN_FAILURE;

	// discard char
	static int c = 0;
	// program buffer for user's inputs; 3x more for 'command', 'arg1' and 'arg2'
	static char buff_in[3 * BUFF_IN_LENGTH] = {0};

	// receive valid input from user, prevent empty of large input
	if ((fgets(buff_in, 3 * BUFF_IN_LENGTH, stdin) != NULL)) {
		// prevent overflowing input, so it doesn't go to next cycle
		if (isoverflow(buff_in[3 * BUFF_IN_LENGTH - 2])) {
			// discard everything in stdin and move on
			while ((c = fgetc(stdin)) != '\n' && c != EOF);
			puts("input too large");
		}
		// input from user is ok, so scan it
		else if (sscanf(buff_in, "%s %s %s", command, arg1, arg2) > 0) {
			// in case some word is longer than BUFF_IN_LENGTH
			command[BUFF_IN_LENGTH - 1] = '\0';
			arg1[BUFF_IN_LENGTH - 1] = '\0';
			arg2[BUFF_IN_LENGTH - 1] = '\0';
			ret = RETURN_SUCCESS;
		}
	}
	else {
	// empty input (EOF)
		puts("");
	}

	return ret;
}

static void sim_format(const char* arg1) {
	if (format_(arg1, fs_name) == RETURN_FAILURE) {
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

int init_simulation(int argc, char const **argv) {
	if (argc <= 1) {
		set_myerrno(Err_fs_name_missing);
		goto error;
	}
	if (strlen(argv[1]) >= STRLEN_FSPATH) {
		set_myerrno(Err_fs_name_long);
		fprintf(stderr, "Maximal length of path is %d characters!\n", STRLEN_FSPATH);
		goto error;
	}
	// parse filesystem name from given path
	if (parse_name(fs_name, argv[1], STRLEN_FSNAME) == RETURN_FAILURE) {
		set_myerrno(Err_fs_name_long);
		fprintf(stderr, "Maximal length of filesystem name is %d characters.\n", STRLEN_FSNAME);
		goto error;
	}
	// init filesystem
	init_filesystem(argv[1], &is_formatted);
	init_prompt();

	return RETURN_SUCCESS;
error:
	return RETURN_FAILURE;
}

int get_command_id(const char* command) {
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
	if (strcmp(command, CMD_LOAD) == 0)		return CMD_LOAD_ID;
	if (strcmp(command, CMD_FSCK) == 0)		return CMD_FSCK_ID;
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
	int err = RETURN_FAILURE;
	int cmd_id = 0;
	char command[BUFF_IN_LENGTH] = {0};
	char arg1[BUFF_IN_LENGTH] = {0};
	char arg2[BUFF_IN_LENGTH] = {0};
	is_running = true;

	while (is_running) {
		fputs(buff_prompt, stdout);

		// clear buffers, so command doesn't accidentally receive
		// wrong argument(s) from previous iteration
		BUFF_CLR(command, strlen(command));
		BUFF_CLR(arg1, strlen(arg1));
		BUFF_CLR(arg2, strlen(arg2));

		if (handle_input(command, arg1, arg2) == RETURN_SUCCESS) {
			cmd_id = get_command_id(command);

			// only basic commands allowed before formatting
			switch (cmd_id) {
				case CMD_FORMAT_ID: sim_format(arg1);	continue;
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
				case CMD_PWD_ID:	err = pwd_();				break;
				case CMD_CAT_ID:	err = cat_(arg1);			break; // TODO
				case CMD_LS_ID:		err = ls_(arg1);			break; // TODO
				case CMD_INFO_ID:	err = info_(arg1);			break;
				case CMD_MV_ID:		err = mv_(arg1, arg2);		break; // TODO
				case CMD_CP_ID:		err = cp_(arg1, arg2);		break; // TODO
				case CMD_RM_ID:		err = rm_(arg1);			break; // TODO
				case CMD_CD_ID:		err = cd_(arg1);			break;
				case CMD_MKDIR_ID:	err = mkdir_(arg1);			break;
				case CMD_RMDIR_ID:	err = rmdir_(arg1);			break;
				case CMD_INCP_ID:	err = incp_(arg1, arg2);	break; // TODO
				case CMD_OUTCP_ID:	err = outcp_(arg1, arg2);	break; // TODO
				case CMD_LOAD_ID:	err = load_(arg1);			break; // TODO
				case CMD_FSCK_ID:	err = fsck_();				break; // TODO
				case CMD_DEBUG_ID:	err = debug_(arg1);			break;
				default:
					puts("-zos: command not found");
			}

			if (err == RETURN_FAILURE && cmd_id != CMD_UNKNOWN_ID) {
				my_perror(command);
				reset_myerrno();
			}
		}
	}

	log_info("Simulation end.");
}
