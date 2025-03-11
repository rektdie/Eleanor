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
else
    EXE_EXT :=
    MKDIR := mkdir -p
    RM := rm -rf
    DEL := rm -f
endif

# List of source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# List of object files
OBJS := $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

# Target executable
$(EXE)$(EXE_EXT): $(OBJS)
	$(CXX) -o $@ $^ -O3 -lpthread

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -c -o $@ $< -O3 -lpthread

# Create the object directory if it doesn't exist
$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

# Clean up build artifacts
.PHONY: clean
clean:
	$(RM) $(OBJ_DIR) || $(DEL) $(OBJ_DIR)\* $(OBJ_DIR)
	$(RM) $(EXE)$(EXE_EXT) || $(DEL) $(EXE)$(EXE_EXT)
	$(MAKE)
