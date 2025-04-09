# Define the directories
SRC_DIR := source
OBJ_DIR := obj

# Set default C++ compiler, but allow override from the command line
CXX ?= g++

# Set default executable name, but allow override from the command line
EXE ?= Eleanor

EVALFILE = ./nnue.bin

# Determine platform
ifeq ($(OS),Windows_NT)
    EXE_EXT := .exe
    MKDIR := mkdir
    RM := rmdir /s /q
    DEL := del /q
else
    EXE_EXT :=
    MKDIR := mkdir -p
    RM := rm -rf
    DEL := rm -f
endif

MAKEFLAGS += -j

# List of source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# Link the executable
$(EXE)$(EXE_EXT): $(SRCS)
	$(CXX) $(CXXFLAGS) -DEVALFILE=\"$(EVALFILE)\" $(SRCS) ./external/fmt/format.cc -o $@ -O3 -lpthread -march=native -flto -ftree-vectorize

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -std=c++23 -c -o $@ $< -O3 -lpthread -march=native -flto -ftree-vectorize

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

# Clean up build artifacts
.PHONY: clean
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJ_DIR) $(RM) $(OBJ_DIR)
	@if exist $(EXE)$(EXE_EXT) $(DEL) $(EXE)$(EXE_EXT)
else
	@rm -rf $(OBJ_DIR)
	@rm -f $(EXE)$(EXE_EXT)
endif
	$(MAKE)
