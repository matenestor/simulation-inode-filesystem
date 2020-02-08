cmake_minimum_required(VERSION 3.15)
project(KIV_ZOS_sp C)

set(CMAKE_C_STANDARD 99)

add_executable(KIV_ZOS_sp
        src/inc/logger_api.h src/_logger.c src/inc/_logger.h
        src/error.c src/inc/error.h

        src/main.c src/inc/main.h
        src/inc/fspath_info.h
        src/inc/return_codes.h

        src/simulator.c src/inc/simulator.h
        src/inc/inode.h
        src/commands.c src/inc/commands.h)
