# ============================================================================
# FLASHBIT ENGINE - MAKEFILE NATIVO
# Stockfish Probe + Dual NNUE embebida desde evaluate.cpp
# ============================================================================

CXX := g++
CC  := gcc

STD := -std=c++17

# ============================================================================
# OPTIMIZACIÓN
# ============================================================================

OPTFLAGS := -O3 \
            -march=native \
            -flto=auto \
            -funroll-loops \
            -finline-functions \
            -fomit-frame-pointer \
            -DUSE_SSE2 \
            -DUSE_SSSE3 \
            -DUSE_POPCNT \
            -DUSE_BMI1 \
            -DUSE_BMI2 \
            -DUSE_AVX2 \
            -DUSE_AVX512 \
            -Wa,-I.

INCLUDES := -Iinclude \
            -Isrc \
            -Istockfish_nnue_probe \
            -Istockfish_nnue_probe/src \
            -Istockfish_nnue_probe/nnue

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
# FUENTES FLASHBIT
# ============================================================================

SRC_ENGINE := $(wildcard src/*.cpp)

# ============================================================================
# FUENTES STOCKFISH PROBE
# ============================================================================

SRC_PROBE_ALL := $(wildcard stockfish_nnue_probe/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*/*/*.cpp)

SRC_PROBE_CPP := $(filter-out \
                 stockfish_nnue_probe/NNUEBridge_NNUEBridge.cpp \
                 stockfish_nnue_probe/main.cpp \
                 stockfish_nnue_probe/tests.cpp \
                 stockfish_nnue_probe/incbin/%.cpp \
                 stockfish_nnue_probe/%/incbin/%.cpp, \
                 $(SRC_PROBE_ALL))

SRC_PROBE_C := $(filter-out \
                stockfish_nnue_probe/incbin/%.c \
                stockfish_nnue_probe/%/incbin/%.c, \
                $(wildcard stockfish_nnue_probe/*.c) \
                $(wildcard stockfish_nnue_probe/*/*.c) \
                $(wildcard stockfish_nnue_probe/*/*/*.c))

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

    TARGET  := FlashBit.exe
    RUN_CMD := .\$(TARGET)

    FIXPATH = $(subst /,\,$1)
    RM_FILES = -del /f /q $(call FIXPATH,$1) 2>nul

else

    TARGET  := FlashBit
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
	@echo Enlazando FlashBit + Stockfish Probe + Dual NNUE...
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo Compilacion completada con exito.

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