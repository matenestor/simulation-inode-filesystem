cmake_minimum_required(VERSION 3.15)
project(KIV_ZOS_sp C)

set(CMAKE_C_STANDARD 99)

add_executable(KIV_ZOS_sp
        src/inc/logger_api.h src/_logger.c src/_logger.h
        src/error.c src/error.h

        src/inc/_fs_varname.h
        src/inc/fs_cache.h
        src/fs_operations.h
        src/inc/fs_path.h
        src/inc/inode.h
        src/inc/return_codes.h

        src/main.c src/main.h
        src/simulator.c src/simulator.h
        src/fs_operations.c
        src/utils.c src/utils.h

        src/commands/format.c src/commands/format.h
        src/commands/ls.c
        src/commands/mkdir.c
        )
