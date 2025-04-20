CXX = g++
# Stderr logging can be enabled with submission so i will leave the -DDEBUG_PRINT
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -I src -lpcap -g -DDEBUG_PRINT
SRC_DIR := src
OBJ_DIR := obj
TEST_DIR := tests

# Had to wrote the source files and headers by hand since there is file containing main in the src directory and tests create their own main.
TEST_SRC := $(TEST_DIR)/tests.cpp
TEST_DEP := $(SRC_DIR)/arguments.hpp $(SRC_DIR)/fsm.hpp $(SRC_DIR)/tcpclient.hpp $(TEST_DIR)/catch.hpp
TEST_SRCS := $(SRC_DIR)/arguments.cpp $(SRC_DIR)/fsm.cpp $(SRC_DIR)/tcpclient.cpp
TEST_BIN := $(TEST_DIR)/tests

# Sources and objects
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Target binary
TARGET := ipk25chat-client

# Default target
all: $(TARGET)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(TEST_SRCS) $(TEST_DEP)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Link the final binary
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean target
clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(TEST_BIN)

.PHONY: all clean
