# C compiler
CC = gcc
# C++ flags
CFLAGS = -std=c99 -pedantic -Wall -O

# name of executable
BIN = inode

# logging directory
DIR_LOG = log/

# source directories
ROOT = src/
DIR_SRC = $(ROOT) $(ROOT)commands/
# headers directory
DIR_INC = $(ROOT)inc/
# object directory
DIR_OBJ = obj/

# filesystems directory
DIR_FS = fs/

# include location of dependent header files
IDEPS = -I$(DIR_INC)

# each .c file in source folder
SRC = $(foreach DSRC, $(DIR_SRC), $(wildcard $(DSRC)*.c))
# all object files
OBJ = $(patsubst $(ROOT)%.c, $(DIR_OBJ)%.c.o, $(SRC))

RM = rm -rf


all: mkdirs $(BIN)

.PHONY: all


$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(DIR_OBJ)%.o: $(ROOT)%
	$(CC) $(CFLAGS) $(IDEPS) -c $< -o $@


mkdirs:
	mkdir -p $(patsubst $(ROOT)%, $(DIR_OBJ)%, $(DIR_SRC))
	mkdir -p $(DIR_LOG)
	mkdir -p $(DIR_FS)

.PHONY: mkdirs


clean:
	$(RM) $(DIR_OBJ)

.PHONY: clean
