CC=gcc
OPTIMIZATION?=-O0
INCLUDES=-Iinclude
CFLAGS=-g -Wall -Wextra $(OPTIMIZATION) $(CFLAGSEXTRA) -pedantic -MMD -MP $(INCLUDES) -flto=$(shell nproc) -march=native
CXX=g++

# GTest dependencies
GTEST_DEPS=gtest
GTEST_CFLAGS=`pkg-config --cflags $(GTEST_DEPS)`
GTEST_LIBS=`pkg-config --libs $(GTEST_DEPS)`

CXXFLAGS=$(CFLAGS)
LDLIBS=-lm

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
          $(patsubst $(TESTS)/eval/%.cpp,$(OBJ)/test_eval_%.o,$(TEST_EVAL_SRCS))
DEPS=$(OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Benchmark settings
BENCH_DEPTH ?= 5
BENCH_GAMES ?= 10
BENCH_ELO ?= 2200
BENCH_CONCURRENCY ?= 1
BENCH_EVAL_FUNC ?= 1
BENCH_THREADS ?= 4
BENCH_STOCKFISH_THREADS ?= 2

.PHONY: all bin release clean test benchmark

bin: $(BIN)/sce_play $(BIN)/sce_uci_engine $(BIN)/dev_magic_compute

all: bin doc

release: OPTIMIZATION=-O3
release: CFLAGS+=-DNDEBUG
release: bin

doc:
	doxygen

test: $(BIN)/test

$(BIN)/sce_play: $(OBJS) $(OBJ)/sce_play.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BIN)/sce_uci_engine: $(OBJS) $(OBJ)/sce_uci_engine.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BIN)/dev_magic_compute: $(OBJS) $(OBJ)/dev_magic_compute.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(OBJ)/test_%.o: $(TESTS)/test_%.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -c $< -o $@

$(OBJ)/eval_%.o: $(SRC)/eval/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/test_eval_%.o: $(TESTS)/eval/%.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) -c $< -o $@

$(OBJ)/sce_play.o: $(SRC)/bin/sce_play.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/sce_uci_engine.o: $(SRC)/bin/sce_uci_engine.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/dev_magic_compute.o: $(SRC)/bin/dev_magic_compute.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

OBJS_UNITTEST=$(filter-out obj/sce_play.o, $(OBJS)) $(TEST_OBJS)
$(BIN)/test: $(OBJS_UNITTEST)
	$(CXX) $(CXXFLAGS) $(GTEST_CFLAGS) $^ -o $@ $(LDLIBS) $(GTEST_LIBS)

clean:
	$(RM) -r $(OBJ)/*.{o,d} $(BIN)/* $(HTML)

benchmark: release
	@echo "Running benchmark: depth=$(BENCH_DEPTH) games=$(BENCH_GAMES) ELO=$(BENCH_ELO) concurrency=$(BENCH_CONCURRENCY) eval_func=$(BENCH_EVAL_FUNC) sce_threads=$(BENCH_THREADS) stockfish_threads=$(BENCH_STOCKFISH_THREADS)"
	cutechess-cli -engine name="SCE" cmd=./bin/sce_uci_engine option.DynamicDeepening=true option.Threads=$(BENCH_THREADS) option.EvalFunc=$(BENCH_EVAL_FUNC) depth=$(BENCH_DEPTH) tc=inf -engine name="Stockfish" cmd=stockfish option.UCI_LimitStrength=true option.UCI_Elo=$(BENCH_ELO) option.Threads=$(BENCH_STOCKFISH_THREADS) depth=18 tc=60+0.6 -each proto=uci -games $(BENCH_GAMES) -repeat -concurrency $(BENCH_CONCURRENCY)
