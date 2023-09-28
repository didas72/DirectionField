CC=gcc
C_FLAGS=-Wall -Wextra -ggdb -Wno-unknown-pragmas

SRC=src
TEST=tests
OBJ=build/obj
BIN=build/bin
INC=include
LIB=libs
DEPS=raylib

OUTBIN=$(BIN)/dfv

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
DEP_LST=$(foreach lib,$(DEPS),-l$(lib))


all: $(OUTBIN)
build: $(OUTBIN)

release: C_FLAGS=-Wall -Wextra -O2 -Wno-unknown-pragmas
release: clean
release: $(OUTBIN)

run: $(OUTBIN)
	./$(OUTBIN)

$(OUTBIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) $^ -L$(LIB) $(DEP_LST) -lm -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@ -I$(INC)



clean:
	$(RM) -r $(OBJ) $(BIN)

loc:
	scc -s lines --no-cocomo --no-gitignore -w --size-unit binary --exclude-ext md,makefile --exclude-dir tests/framework
