# C compiler
CC = gcc
# C++ flags
CFLAGS = -std=c99 -pedantic -Wall -O

# name of executable
BIN = inode

# logging directory
DIR_LOG = log/

# source directory
DIR_SRC = src/
# headers directory
DIR_INC = $(DIR_SRC)inc/
# object directory
DIR_OBJ = obj/
# output directory
DIR_BIN = bin/

# include location of dependent header files
IDEPS = -I$(DIR_INC)

# each .h file in include folder
HDR = $(wildcard $(DIR_INC)*.h)
# each .c file in source folder
SRC = $(wildcard $(DIR_SRC)*.c)
# all object files
OBJ = $(patsubst $(DIR_SRC)%,$(DIR_OBJ)%.o,$(SRC))

RM = rm -rf


all: mkdirs $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(DIR_BIN)$@ $^

$(DIR_OBJ)%.o: $(DIR_SRC)% $(HDR)
	$(CC) $(CFLAGS) $(IDEPS) -c $< -o $@

.PHONY: all


mkdirs:
	mkdir -p $(DIR_LOG)
	mkdir -p $(DIR_BIN)
	mkdir -p $(DIR_OBJ)

.PHONY: mkdirs


clean:
	$(RM) $(DIR_OBJ)

.PHONY: clean
