# Define the directories
SRC_DIR := source
OBJ_DIR := obj

CXX := g++
EXE := Engine-Eleanor

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
$(EXE)$(EXE_EXT): $(OBJS)
	$(CXX) -o $@ $^ -O3

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(SRC_DIR)/types.h | $(OBJ_DIR)
	$(CXX) -c -o $@ $< -O3

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	mkdir $@
