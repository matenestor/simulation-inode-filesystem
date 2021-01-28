cmake_minimum_required(VERSION 3.15)
project(KIV_ZOS_sp C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(KIV_ZOS_sp
#        ~/tmp/tmp.c
        src/logger.c
        src/errors.c

        src/main.c
        src/simulator.c
        src/utils.c

        src/commands/pwd.c
        src/commands/cat.c
        src/commands/ls.c
        src/commands/info.c
        src/commands/mv.c
        src/commands/cp.c
        src/commands/rm.c
        src/commands/cd.c
        src/commands/mkdir.c
        src/commands/rmdir.c
        src/commands/incp.c
        src/commands/outcp.c
        src/commands/load.c
        src/commands/fsck.c
        src/commands/format.c
        src/commands/debug.c

        src/fsop/fs_bitmap.c
        src/fsop/fs_block_op.c
        src/fsop/fs_common.c
        src/fsop/fs_inode_op.c
        src/fsop/fs_io.c
        )
