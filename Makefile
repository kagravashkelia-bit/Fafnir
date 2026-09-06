# ============================================================================
# FLASHBIT ENGINE - MAKEFILE OPTIMIZADO (SOLO STOCKFISH PROBE + DUAL NNUE)
# ============================================================================

CXX      := g++
CC       := gcc
STD      := -std=c++17

# ======= CONFIGURACIÓN DE LAS REDES NEURONALES (BIG & SMALL) =======
NNUE_BIG   := nn-b1a57edbea57.nnue
NNUE_SMALL := nn-baff1ede1f90.nnue

OBJ_BIG    := embed_nnue_big.o
OBJ_SMALL  := embed_nnue_small.o
# ====================================================================

# Opciones de optimización e inclusión
OPTFLAGS := -O3 -march=native -flto=auto -funroll-loops -finline-functions -fomit-frame-pointer -DUSE_SSE2 -DUSE_SSSE3 -DUSE_POPCNT -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2 -DUSE_AVX512 -Wa,-I.
INCLUDES := -Iinclude -Isrc -Istockfish_nnue_probe -Istockfish_nnue_probe/src -Istockfish_nnue_probe/nnue

CXXFLAGS := $(STD) $(OPTFLAGS) -DNDEBUG -Wall -Wextra -MMD -MP $(INCLUDES)
CFLAGS   := -O3 -DNDEBUG -Wall -Wextra -MMD -MP $(INCLUDES)
LDFLAGS  := $(OPTFLAGS) -s -Wl,--gc-sections -pthread -Wa,-I.

# Búsqueda automática de fuentes de FlashBit
SRC_ENGINE  := $(wildcard src/*.cpp)

# Búsqueda recursiva profunda en las subcarpetas de stockfish_nnue_probe
SRC_PROBE_ALL := $(wildcard stockfish_nnue_probe/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*/*.cpp) \
                 $(wildcard stockfish_nnue_probe/*/*/*/*.cpp)

SRC_PROBE_CPP := $(filter-out stockfish_nnue_probe/NNUEBridge_NNUEBridge.cpp \
                              stockfish_nnue_probe/main.cpp \
                              stockfish_nnue_probe/tests.cpp, $(SRC_PROBE_ALL))

SRC_PROBE_C   := $(wildcard stockfish_nnue_probe/*.c) \
                 $(wildcard stockfish_nnue_probe/*/*.c) \
                 $(wildcard stockfish_nnue_probe/*/*/*.c)

# Lista de Objetos
OBJ           := $(SRC_ENGINE:.cpp=.o) \
                 $(SRC_PROBE_CPP:.cpp=.o) \
                 $(SRC_PROBE_C:.c=.o) \
                 $(OBJ_BIG) \
                 $(OBJ_SMALL)

DEP           := $(filter %.d, $(OBJ:.o=.d))

# Detección de Sistema Operativo
ifeq ($(OS),Windows_NT)
    TARGET   := FlashBit.exe
    RUN_CMD  := .\$(TARGET)
    FIXPATH   = $(subst /,\,$1)
    RM_FILES  = -del /f /q $(call FIXPATH,$1) 2>nul
else
    TARGET   := FlashBit
    RUN_CMD  := ./$(TARGET)
    FIXPATH   = $1
    RM_FILES  = -rm -f $1 2>/devnull
endif

all: $(TARGET)

-include $(DEP)

$(TARGET): $(OBJ)
	@echo "Enlazando $(TARGET) con Stockfish Probe y Dual NNUE..."
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "¡Compilación completada con éxito!"

# Quemado de ambas redes en binarios independientes
$(OBJ_BIG): $(NNUE_BIG)
	@echo "Quemando $(NNUE_BIG) en el binario..."
	ld -r -b binary -o $@ $<

$(OBJ_SMALL): $(NNUE_SMALL)
	@echo "Quemando $(NNUE_SMALL) en el binario..."
	ld -r -b binary -o $@ $<

%.o: %.cpp
	@echo "Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	@echo "Compilando $<..."
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	$(RUN_CMD)

clean:
	@echo "Limpiando archivos objeto y ejecutables..."
	@$(call RM_FILES,$(TARGET))
	@$(call RM_FILES,$(OBJ))
	@$(call RM_FILES,$(DEP))

.PHONY: all clean run