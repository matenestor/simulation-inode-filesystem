#ifndef FSPATH_INFO_H
#define FSPATH_INFO_H


#define STRLEN_FSDIR  3
// note: length mentioned in doc comment of parse_fsname() in main.c
#define STRLEN_FSNAME 32
#define STRLEN_FSPATH (STRLEN_FSDIR+STRLEN_FSNAME)

#define FORMAT_FSDIR  "fs/%s"

#endif
