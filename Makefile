# Define the directories
SRC_DIR := source
OBJ_DIR := obj

# Determine platform
ifeq ($(OS),Windows_NT)
	EXE_EXT := .exe
else
	EXE_EXT :=
endif

# List of source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# List of object files
OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

# Target executable
Chess-Test$(EXE_EXT): $(OBJS)
	g++ -o $@ $^

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ -c -o $@ $<

# Dependencies
main.o: $(SRC_DIR)/main.cpp $(SRC_DIR)/types.h $(SRC_DIR)/board.h $(SRC_DIR)/movegen.h
bitboards.o: $(SRC_DIR)/bitboards.cpp $(SRC_DIR)/bitboards.h
board.o: $(SRC_DIR)/board.cpp $(SRC_DIR)/board.h
movegen.o: $(SRC_DIR)/movegen.cpp $(SRC_DIR)/movegen.h

# Create the object directory if it doesn't exist
$(shell mkdir $(OBJ_DIR))
