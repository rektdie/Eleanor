# Define directories
SRC_DIR := source
OBJ_DIR := obj

EXE ?= Eleanor
EVALFILE := ./nnue.bin

CXX := clang++

ifeq ($(OS),Windows_NT)
	UNAME_S := Windows
	ARCH := x86_64
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

CXX_BASE := $(notdir $(firstword $(CXX)))
CXX_DRIVER_FLAGS :=
CXXFLAGS := -std=c++20 -O3 -flto -Wall
LDFLAGS := -O3 -flto

ifeq ($(PLATFORM),mac)
	ifeq ($(ARCH),arm64)
	    ARCH_FLAGS := -mcpu=apple-m1
	else
	    ARCH_FLAGS := -march=native
	endif
else ifeq ($(PLATFORM),windows)
	ARCH_FLAGS := -march=native
	CXX_DRIVER_FLAGS += --target=$(ARCH)-w64-windows-gnu
	CXXFLAGS += -pthread
	LDFLAGS += -pthread
	LDFLAGS += -static -Wl,--stack,8388608
else
	ifeq ($(ARCH),aarch64)
	    ARCH_FLAGS := -mcpu=native
	else
	    ARCH_FLAGS := -march=native
	    LDFLAGS += -fuse-ld=lld -static
	endif
endif

CXXFLAGS += $(ARCH_FLAGS)
LDFLAGS += $(ARCH_FLAGS)

MAKEFLAGS += -j

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

$(EXE)$(EXE_EXT): $(OBJS)
	$(CXX) $(CXX_DRIVER_FLAGS) $(CXXFLAGS) -DEVALFILE=\"$(EVALFILE)\" $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXX_DRIVER_FLAGS) $(CXXFLAGS) -c -o $@ $<

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
