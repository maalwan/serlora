# Compiler and flags
CXX = gcc
CPPFLAGS = -Wall -g -Iinclude

# Directories
SRC_DIR = src
OBJ_DIR = build

# Source files and corresponding object files, excluding wioe.c
SRCS = $(filter-out $(SRC_DIR)/wioe.c, $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# wioe.c and its object file
WIOE_SRC = $(SRC_DIR)/wioe.c
WIOE_OBJ = $(OBJ_DIR)/wioe.o

# Executable and headers
EXE = wio
HEADERS = $(wildcard include/*.h)

# Default target - build the executable
$(EXE): $(OBJS) $(WIOE_OBJ)
	$(CXX) $(CPPFLAGS) -o $(EXE) $(OBJS) $(WIOE_OBJ) -lsodium

# Rule for compiling .c files to .o files, excluding wioe.c
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

# Special rule for compiling wioe.c (link with libsodium)
$(WIOE_OBJ): $(WIOE_SRC) $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) -c $(WIOE_SRC) -o $(WIOE_OBJ)

# Phony target - remove generated files and backups
clean:
	rm -rf $(EXE) $(OBJ_DIR)/*.o *~ *.dSYM