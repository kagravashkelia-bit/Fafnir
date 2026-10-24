# ============================================================================
# FLASHBIT ENGINE - MAKEFILE NATIVO
# Stockfish dev SFNNv17 NNUE probe + red unica embebida
# ============================================================================

CXX := g++
CC  := gcc

STD := -std=c++17

# ============================================================================
# OPTIMIZACIÓN
# ============================================================================
#
# ARCH=native (default) tunes the build for the machine running the compiler.
# Use one of the portable profiles to build binaries for other people:
#   make ARCH=x86-64-avx2            (Intel Haswell / AMD Zen or newer)
#   make ARCH=x86-64-sse41-popcnt    (older 64-bit CPUs, slower)
#   make ARCH=x86-64                 (baseline, SSE2 only)
# Always run "make clean" when switching ARCH, because object files are reused.
# ============================================================================

ARCH ?= native

ifeq ($(ARCH),native)
    ARCHFLAGS := -march=native
    ARCHDEFS  := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_POPCNT -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2
else ifeq ($(ARCH),x86-64-avx2)
    ARCHFLAGS := -mavx2 -mbmi -mbmi2 -mpopcnt -msse4.1 -mssse3
    ARCHDEFS  := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_POPCNT -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2
else ifeq ($(ARCH),x86-64-sse41-popcnt)
    ARCHFLAGS := -msse4.1 -mpopcnt -mssse3
    ARCHDEFS  := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_POPCNT -DUSE_SSE41
else ifeq ($(ARCH),x86-64)
    ARCHFLAGS := -msse2
    ARCHDEFS  := -DUSE_SSE2
else
    $(error Unknown ARCH '$(ARCH)'. Use native, x86-64-avx2, x86-64-sse41-popcnt or x86-64.)
endif

OPTFLAGS := -O3 \
            $(ARCHFLAGS) \
            $(ARCHDEFS) \
            -flto=auto \
            -ffunction-sections \
            -fdata-sections \
            -funroll-loops \
            -finline-functions \
            -fomit-frame-pointer \
            -Wa,-I.

INCLUDES := -Iinclude \
            -Isrc \
            -Istockfish_nnue_probe \
            -Istockfish_nnue_probe/sfdev/src

CXXFLAGS := $(STD) \
            $(OPTFLAGS) \
            -DNDEBUG \
            -Wall \
            -Wextra \
            -MMD \
            -MP \
            $(INCLUDES)

CFLAGS := -O3 \
          -DNDEBUG \
          -Wall \
          -Wextra \
          -MMD \
          -MP \
          $(INCLUDES)

LDFLAGS := $(OPTFLAGS) \
           -s \
           -Wl,--gc-sections \
           -pthread \
           -Wa,-I.

# ============================================================================
# FUENTES Fafnir
# ============================================================================

SRC_ENGINE := $(wildcard src/*.cpp)

# ============================================================================
# FUENTES DEL PROBE NNUE STOCKFISH DEV
# ============================================================================

SFDEV_SRC := stockfish_nnue_probe/sfdev/src
SRC_PROBE_CPP := stockfish_nnue_probe/sfdev_probe.cpp \
                 $(SFDEV_SRC)/attacks.cpp \
                 $(SFDEV_SRC)/bitboard.cpp \
                 $(SFDEV_SRC)/position.cpp \
                 $(SFDEV_SRC)/misc.cpp \
                 $(SFDEV_SRC)/nnue/network.cpp \
                 $(SFDEV_SRC)/nnue/nnue_accumulator.cpp \
                 $(SFDEV_SRC)/nnue/nnue_misc.cpp \
                 $(SFDEV_SRC)/nnue/features/full_threats.cpp \
                 $(SFDEV_SRC)/nnue/features/half_ka_v2_hm.cpp \
                 $(SFDEV_SRC)/nnue/features/pp_3wide.cpp
SRC_PROBE_C :=

# ============================================================================
# OBJETOS
# ============================================================================

OBJ := $(SRC_ENGINE:.cpp=.o) \
       $(SRC_PROBE_CPP:.cpp=.o) \
       $(SRC_PROBE_C:.c=.o)

DEP := $(OBJ:.o=.d)

# ============================================================================
# SISTEMA OPERATIVO
# ============================================================================

ifeq ($(OS),Windows_NT)

    TARGET  := Fafnir.exe
    RUN_CMD := .\$(TARGET)

    # Link the MinGW runtime statically so the executable is self-contained.
    LDFLAGS += -static

    FIXPATH = $(subst /,\,$1)
    RM_FILES = -del /f /q $(call FIXPATH,$1) 2>nul

else

    TARGET  := Fafnir
    RUN_CMD := ./$(TARGET)

    FIXPATH = $1
    RM_FILES = -rm -f $1 2>/dev/null

endif

# ============================================================================
# DEFAULT
# ============================================================================

.PHONY: all clean run rebuild

all: $(TARGET)

-include $(DEP)

# ============================================================================
# LINK
# ============================================================================

$(TARGET): $(OBJ)
	@echo Enlazando FlashBit + probe NNUE SFNNv17...
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo Compilacion completada con exito.

$(SFDEV_SRC)/nnue/network.o: nn-252f33942263.nnue

# ============================================================================
# C++
# ============================================================================

%.o: %.cpp
	@echo Compilando $<...
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# C
# ============================================================================

%.o: %.c
	@echo Compilando $<...
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# RUN
# ============================================================================

run: $(TARGET)
	$(RUN_CMD)

# ============================================================================
# CLEAN
# ============================================================================

clean:
	@echo Limpiando...
	@$(call RM_FILES,$(TARGET))
	@$(call RM_FILES,$(OBJ))
	@$(call RM_FILES,$(DEP))

# ============================================================================
# REBUILD
# ============================================================================

rebuild: clean all