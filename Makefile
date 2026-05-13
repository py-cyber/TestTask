CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -pedantic -O2 -g -pthread -Iinclude -DDEBUG
LDFLAGS = -pthread
GTEST = -lgtest -lgtest_main

BIN = build
OBJ = obj
APP_TARGET = $(BIN)/tape_sort_app
TEST_TARGET = $(BIN)/tape_sort_tests

ALL_SRC = $(wildcard src/*.cpp) $(wildcard tests/*.cpp)

APP_SRC = $(filter-out tests/%, $(ALL_SRC))
TEST_SRC = $(filter-out src/main.cpp, $(ALL_SRC))

APP_OBJ = $(patsubst %.cpp,$(OBJ)/%.o,$(APP_SRC))
TEST_OBJ = $(patsubst %.cpp,$(OBJ)/%.o,$(TEST_SRC))

all: $(APP_TARGET) $(TEST_TARGET)

$(APP_TARGET): $(APP_OBJ)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_TARGET): $(TEST_OBJ)
	@mkdir -p $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST)

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(BIN) tmp

run: $(APP_TARGET)
	./$(APP_TARGET) input.bin output.bin config.cfg

test: $(TEST_TARGET)
	./$(TEST_TARGET)

valgrind_test: $(TEST_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_TARGET)

valgrind_app: $(APP_TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(APP_TARGET) input.bin output.bin config.cfg

check: valgrind_test valgrind_app

.PHONY: all clean run test valgrind_test valgrind_app check
