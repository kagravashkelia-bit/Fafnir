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

OPTFLAGS := -O3 \
            -march=native \
            -flto=auto \
            -ffunction-sections \
            -fdata-sections \
            -funroll-loops \
            -finline-functions \
            -fomit-frame-pointer \
            -DUSE_SSE2 \
            -DUSE_SSSE3 \
            -DUSE_POPCNT \
            -DUSE_BMI1 \
            -DUSE_BMI2 \
            -DUSE_AVX2 \
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
# FUENTES FLASHBIT
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