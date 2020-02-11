#ifndef _FS_VARNAME_H
#define _FS_VARNAME_H


// this exists, because i use the variable name in macros in fs_operations.h
// if this was in fs_operations.h, which is included in simulator.c,
// then simulator would be able to reach for extern variables with same name
// as those, which are defined inside simulator.h (sb, in_actual, in_distant)
// (i think it would be bad design for bigger projects.. this is not big project,
// but i am trying to acquire good habits as soon as possible --
// -- if you have better solution, feel free to let me know)
#define FS_VARIABLE_NAME filesystem


#endif
