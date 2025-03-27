# Define the directories
SRC_DIR := source
OBJ_DIR := obj

# Set default C++ compiler, but allow override from the command line
CXX ?= g++

# Set default executable name, but allow override from the command line
EXE ?= Eleanor

# Determine platform
ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    MKDIR := mkdir
    RM := rmdir /s /q
    DEL := del /q
    # Use all cores on Windows by getting the number of cores using wmic
    MAKEFLAGS += -j$(shell wmic cpu get NumberOfLogicalProcessors | findstr /r /v "^$" | tail -n 1)
else
    EXE_EXT :=
    MKDIR := mkdir -p
    RM := rm -rf
    DEL := rm -f
    # Use all cores on Linux by getting the number of cores using nproc
    MAKEFLAGS += -j$(shell nproc)
endif

# List of source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# List of object files
OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

# Target executable
$(EXE)$(EXE_EXT): $(OBJS)
	$(CXX) -std=c++23 -o $@ $^ -O3 -lpthread -march=native -flto -ftree-vectorize

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -std=c++23 -c -o $@ $< -O3 -lpthread -march=native -flto -ftree-vectorize

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

# Clean up build artifacts
.PHONY: clean
clean:
	$(RM) $(OBJ_DIR) || $(DEL) $(OBJ_DIR)\* $(OBJ_DIR)
	$(RM) $(EXE)$(EXE_EXT) || $(DEL) $(EXE)$(EXE_EXT)
	$(MAKE)
