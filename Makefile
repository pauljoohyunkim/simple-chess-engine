PKG_DEPS=gtest
INCLUDES=-Iinclude

CC=gcc
CFLAGSEXTRA=
OPTIMIZATION?=-O3
CFLAGS=-g -Wall -Wextra $(OPTIMIZATION) -pedantic -MMD -MP $(INCLUDES) -flto -march=native
CFLAGS+=`pkg-config --cflags $(PKG_DEPS)`
CFLAGS+=$(CFLAGSEXTRA)
CXX=g++
CXXFLAGS=$(CFLAGS)
LDLIBS=`pkg-config --libs $(PKG_DEPS)`

BIN=bin
OBJ=obj
SRC=src
TESTS=tests
HTML=html
SRCS=$(wildcard $(SRC)/*.c)
EVAL_SRCS = $(wildcard $(SRC)/eval/*.c)
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS)) \
       $(patsubst $(SRC)/eval/%.c, $(OBJ)/eval_%.o, $(EVAL_SRCS))
TEST_SRCS=$(wildcard $(TESTS)/*.cpp)
TEST_EVAL_SRCS=$(wildcard $(TESTS)/eval/*.cpp)
TEST_OBJS=$(patsubst $(TESTS)/%.cpp,$(OBJ)/%.o, $(TEST_SRCS)) \
		  $(patsubst $(TESTS)/eval/%.c, $(OBJ)/eval_%.o, $(TEST_EVAL_SRCS))
DEPS=$(OBJS:.o=.d)

.PHONY: all bin doc clean test

bin: $(BIN)/sce_play

all: bin doc

doc:
	doxygen

test: $(BIN)/test

$(BIN)/sce_play: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/test_%.o: $(TESTS)/test_%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@



$(OBJ)/eval_%.o: $(SRC)/eval/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

OBJS_UNITTEST=$(filter-out obj/sce_play.o, $(OBJS)) $(TEST_OBJS)
$(BIN)/test: $(OBJS_UNITTEST)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

clean:
	$(RM) -r $(OBJ)/*.{o,d} $(BIN)/* $(HTML)
