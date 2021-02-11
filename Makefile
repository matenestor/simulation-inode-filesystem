# C compiler
CC = gcc
# C flags
CFLAGS = -Wall -pedantic -O

# name of executable
BIN = inode

# source directories
ROOT = src/
DIR_SRC = $(ROOT) $(ROOT)commands/ $(ROOT)fsop/

# headers directory
DIR_INC = include

# object directory
DIR_OBJ = obj/

# include location of dependent header files
IDEPS = -I$(DIR_INC)

# all .c files in source folder
SRC = $(foreach DSRC, $(DIR_SRC), $(wildcard $(DSRC)*.c))
# all object files
OBJ = $(patsubst $(ROOT)%.c, $(DIR_OBJ)%.c.o, $(SRC))

RM = rm -r


all: mkdirs $(BIN)

.PHONY: all


$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(DIR_OBJ)%.o: $(ROOT)%
	$(CC) $(CFLAGS) $(IDEPS) -c $< -o $@


mkdirs:
	mkdir -p $(patsubst $(ROOT)%, $(DIR_OBJ)%, $(DIR_SRC))

.PHONY: mkdirs


clean:
	$(RM) $(DIR_OBJ)

.PHONY: clean
