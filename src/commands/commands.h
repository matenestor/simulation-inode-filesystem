#ifndef COMMANDS_H
#define COMMANDS_H

#define CMD_PWD			"pwd"
#define CMD_CAT			"cat"
#define CMD_LS			"ls"
#define CMD_INFO		"info"
#define CMD_MV			"mv"
#define CMD_CP			"cp"
#define CMD_RM			"rm"
#define CMD_CD			"cd"
#define CMD_MKDIR		"mkdir"
#define CMD_RMDIR		"rmdir"
#define CMD_INCP		"incp"
#define CMD_OUTCP		"outcp"
#define CMD_DU			"du"
#define CMD_LOAD		"load"
#define CMD_FSCK		"fsck"
#define CMD_CORRUPT		"corrupt"
#define CMD_FORMAT		"format"
#define CMD_HELP		"help"
#define CMD_EXIT		"exit"
#define CMD_DEBUG		"d"

#define CMD_UNKNOWN_ID	0
#define CMD_PWD_ID		1
#define CMD_CAT_ID		2
#define CMD_LS_ID		3
#define CMD_INFO_ID		4
#define CMD_MV_ID		5
#define CMD_CP_ID		6
#define CMD_RM_ID		7
#define CMD_CD_ID		8
#define CMD_MKDIR_ID	9
#define CMD_RMDIR_ID	10
#define CMD_INCP_ID		11
#define CMD_OUTCP_ID	12
#define CMD_DU_ID		13
#define CMD_LOAD_ID		14
#define CMD_FSCK_ID		15
#define CMD_CORRUPT_ID	16
#define CMD_FORMAT_ID	17
#define CMD_HELP_ID		18
#define CMD_EXIT_ID		19
#define CMD_DEBUG_ID	20

extern int sim_pwd();
extern int sim_cat(const char*);
extern int sim_ls(const char*);
extern int sim_info(const char*);
extern int sim_mv(const char*, const char*);
extern int sim_cp(const char*, const char*);
extern int sim_rm(const char*);
extern int sim_cd(const char*);
extern int sim_mkdir(const char*);
extern int sim_rmdir(const char*);
extern int sim_incp(const char*, const char*);
extern int sim_outcp(const char*, const char*);
extern int sim_du();
extern int sim_load(const char*);
extern int sim_fsck();
extern int sim_corrupt();
extern int sim_format(const char*, const char*);
extern int sim_debug(const char*, const char*);

#endif
