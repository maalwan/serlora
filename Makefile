# Compiler and flags
CXX = gcc
CPPFLAGS = -Wall -g -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = build

# Source files and corresponding object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executable and headers
EXE = wio
HEADERS = $(wildcard include/*.h)

# Default target
$(EXE): $(OBJS)
	$(CXX) $(CPPFLAGS) -o $(EXE) $(OBJS)

# Rule for compiling .c files to .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# Phony target - remove generated files and backups
clean:
	rm -rf $(EXE) $(OBJ_DIR)/*.o *~ *.dSYM