# Define directories
SRC_DIR := source
OBJ_DIR := obj
FMT_DIR := external/fmt

CXX := clang++
EXE ?= Eleanor
EVALFILE := ./nnue.bin

ifeq ($(OS),Windows_NT)
    UNAME_S := Windows
    EXE_EXT := .exe
    MKDIR := mkdir
    RM := rmdir /s /q
    DEL := del /q
    PLATFORM := windows
else
    UNAME_S := $(shell uname -s)
    ARCH := $(shell uname -m)
    EXE_EXT :=
    MKDIR := mkdir -p
    RM := rm -rf
    DEL := rm -f
    ifeq ($(UNAME_S),Darwin)
        PLATFORM := mac
    else
        PLATFORM := unix
    endif
endif

CXXFLAGS := -std=c++20 -O3 -flto -Wall
LDFLAGS := -O3 -flto

ifeq ($(PLATFORM),mac)
    ifeq ($(ARCH),arm64)
        ARCH_FLAGS :=
    else
        ARCH_FLAGS := -march=native
    endif
else
    ARCH_FLAGS := -march=native
    LDFLAGS += -fuse-ld=lld -static
endif

CXXFLAGS += $(ARCH_FLAGS)
LDFLAGS += $(ARCH_FLAGS)

MAKEFLAGS += -j

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

FMT_SRC := $(FMT_DIR)/format.cc
FMT_OBJ := $(OBJ_DIR)/format.o

$(EXE)$(EXE_EXT): $(OBJS) $(FMT_OBJ)
	$(CXX) $(CXXFLAGS) -DEVALFILE=\"$(EVALFILE)\" $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(FMT_OBJ): $(FMT_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

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
