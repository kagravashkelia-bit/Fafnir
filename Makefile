# ============================================================================
# FAFNIR ENGINE - MAKEFILE NATIVO
# Stockfish dev SFNNv17 NNUE probe + red unica embebida
#
# Estructura:
#   src/ui        -> capa interfaz (UCI, entry point)
#   src/engine    -> motor (busqueda, ordenacion, evaluacion)
#   src/bridges   -> puentes con codigo vendido (probe NNUE)
#   third_party   -> codigo vendido sin modificar
#   assets/nnue   -> red neuronale embebida en el binario
#   build/        -> objetos intermedios
#   bin/          -> binario final
# ============================================================================

CXX := g++
CC  := gcc

STD := -std=c++17

BUILD_DIR := build
BIN_DIR   := bin

# ============================================================================
# OPTIMIZACIÓN
# ============================================================================

OPTFLAGS := -O3 \
            -march=x86-64-v4 \
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

INCLUDES := -Isrc/ui \
            -Isrc/engine \
            -Isrc/engine/eval \
            -Isrc/core \
            -Isrc/bridges \
            -I. \
            -Ithird_party/chesslib \
            -Ithird_party/stockfish/sfdev/src \
            -Ithird_party/stockfish/probe \
            -Ithird_party

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

SRC_ENGINE := src/ui/main.cpp \
              src/ui/uci.cpp \
              src/engine/SearchController.cpp \
              src/engine/movepicker.cpp \
              src/bridges/nnue_probe.cpp

# ============================================================================
# FUENTES DEL PROBE NNUE STOCKFISH DEV
# ============================================================================

SFDEV_SRC := third_party/stockfish/sfdev/src
SRC_PROBE_CPP := $(SFDEV_SRC)/attacks.cpp \
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
NNUE_FILE := assets/nnue/nn-252f33942263.nnue

# ============================================================================
# OBJETOS
# ============================================================================

OBJ_ENGINE := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC_ENGINE))
OBJ_PROBE  := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC_PROBE_CPP)) \
              $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC_PROBE_C))

OBJ := $(OBJ_ENGINE) $(OBJ_PROBE)
DEP := $(OBJ:.o=.d)

# ============================================================================
# SISTEMA OPERATIVO
# ============================================================================

ifeq ($(OS),Windows_NT)

    TARGET  := $(BIN_DIR)/Fafnir.exe
    RUN_CMD := .\bin\Fafnir.exe
    MKDIR   := mkdir
    RM_CMD  := rmdir /s /q
    DEL_CMD := if exist $1 (del /f /q $(call FIXPATH,$1))
    FIXPATH = $(subst /,\,$1)

else

    TARGET  := $(BIN_DIR)/Fafnir
    RUN_CMD := ./bin/Fafnir
    MKDIR   := mkdir -p
    RM_CMD  := rm -rf
    DEL_CMD := rm -f $1
    FIXPATH = $1

endif

# ============================================================================
# DEFAULT
# ============================================================================

.PHONY: all clean run rebuild
.DEFAULT_GOAL := all

all: $(TARGET)

-include $(DEP)

# ============================================================================
# DIRECTORIES
# ============================================================================

$(BUILD_DIR) $(BIN_DIR):
	-@$(MKDIR) $@ 2>nul

# ============================================================================
# LINK
# ============================================================================

$(TARGET): $(OBJ) $(NNUE_FILE) | $(BIN_DIR)
	@echo Enlazando Fafnir + probe NNUE SFNNv17...
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)
	@echo Compilacion completada con exito.

$(BUILD_DIR)/$(SFDEV_SRC)/nnue/network.o: $(NNUE_FILE)

# ============================================================================
# C++
# ============================================================================

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	-@$(MKDIR) $(patsubst %/,%,$(call FIXPATH,$(dir $@))) 2>nul
	@echo Compilando $<...
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================================
# C
# ============================================================================

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
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
	-$(RM_CMD) $(BUILD_DIR)
	-$(call DEL_CMD,$(TARGET))

# ============================================================================
# REBUILD
# ============================================================================

rebuild: clean all
