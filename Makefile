PKG_DEPS=ncurses
INCLUDES=-Iinclude

CC=g++
CFLAGSEXTRA=
CFLAGS=-g -Wall -Wextra -O0 -pedantic -MMD -MP $(INCLUDES)
CFLAGS+=`pkg-config --cflags $(PKG_DEPS)`
CFLAGS+=$(CFLAGSEXTRA)
LDLIBS=`pkg-config --libs $(PKG_DEPS)`

BIN=bin
OBJ=obj
SRC=src
TESTS=tests
HTML=html
SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o, $(SRCS))
DEPS=$(OBJS:.o=.d)

.PHONY: all bin doc clean

bin: $(BIN)/sce_play

all: bin doc

doc:
	doxygen

$(BIN)/sce_play: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	$(RM) -r $(OBJ)/*.{o,d} $(BIN)/* $(HTML)
