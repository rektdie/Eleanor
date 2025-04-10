# Define directories
SRC_DIR := source
OBJ_DIR := obj

CXX ?= clang++
EXE ?= Eleanor
EVALFILE := ./nnue.bin

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

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Final binary
$(EXE)$(EXE_EXT): $(OBJS)
	$(CXX) $(CXXFLAGS) -DEVALFILE=\"$(EVALFILE)\" $^ ./external/fmt/format.cc -o $@ -O3 -fuse-ld=lld -march=native -flto -static

# Compile .cpp -> .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) -std=c++20 -c -o $@ $< -O3 -march=native -fuse-ld=lld -flto -static

# Create object directory
$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

# Clean up and rebuild
.PHONY: clean
clean:
ifeq ($(OS),Windows_NT)
	@if exist $(OBJ_DIR) $(RM) $(OBJ_DIR)
	@if exist $(EXE)$(EXE_EXT) $(DEL) $(EXE)$(EXE_EXT)
else
	@$(RM) $(OBJ_DIR)
	@$(DEL) $(EXE)$(EXE_EXT)
endif
	$(MAKE)
