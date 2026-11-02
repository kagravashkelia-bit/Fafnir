# ============================================================================
# FAFNIR ENGINE - MAKEFILE NATIVO MULTI-ARQUITECTURA
# Stockfish dev SFNNv17 NNUE probe + red unica embebida
#
# Estructura:
#   src/ui        -> capa interfaz (UCI, entry point)
#   src/engine    -> motor (busqueda, ordenacion, evaluacion)
#   src/bridges   -> puentes con codigo vendido (probe NNUE, JNI)
#   third_party   -> codigo vendido sin modificar
#   assets/nnue   -> red neuronal embebida en el binario
#   build/        -> objetos intermedios
#   bin/          -> binario final
#
# Uso habitual:
#   make                       # ARQUITECTURA SEGURA: x86-64 (SSE2, corre en
#                              # cualquier CPU x86_64 moderna)
#   make ARCH=x86-64-avx2      # AVX2 + BMI2 + POPCNT (x86-64-v3)
#   make ARCH=x86-64-avx512    # AVX-512 + VNNI + PEXT (x86-64-v4)
#   make ARCH=native           # autodetectada de la maquina de compilacion
#   make all-arches            # binarios sufijados de todas las arquitecturas
#
# Compilador: g++ por defecto; alternar con CXX=clang++ make ...
# ============================================================================

# ----------------------------------------------------------------------------
# COMPILADOR / HERRAMIENTAS
# ----------------------------------------------------------------------------
CXX ?= g++
CC  ?= gcc

IS_CLANG := $(findstring clang,$(shell $(CXX) --version 2>&1))
ifeq ($(IS_CLANG),)
    LTO_FLAG := -flto=auto
else
    LTO_FLAG := -flto
endif

STD := -std=c++17

# Directorio de build por arquitectura: cada ARCH tiene sus propios objetos
# y nunca se mezclan flags entre binarios.
BUILD_DIR := build/$(ARCH)
BIN_DIR   := bin

# ----------------------------------------------------------------------------
# ARQUITECTURA (defecto: x86-64 seguro, corre en cualquier x86_64)
# ----------------------------------------------------------------------------
ARCH ?= x86-64

OPTFLAGS_BASE := -O3 \
                 $(LTO_FLAG) \
                 -ffunction-sections \
                 -fdata-sections \
                 -funroll-loops \
                 -finline-functions \
                 -fomit-frame-pointer

ifeq ($(ARCH),x86-64)
    ARCH_FLAGS     := -msse2
    ARCH_DEFINES   := -DUSE_SSE2
else ifeq ($(ARCH),x86-64-ssse3)
    ARCH_FLAGS     := -mssse3
    ARCH_DEFINES   := -DUSE_SSE2 -DUSE_SSSE3
else ifeq ($(ARCH),x86-64-sse4.2)
    ARCH_FLAGS     := -msse4.2 -mpopcnt
    ARCH_DEFINES   := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_SSE41 -DUSE_SSE42 -DUSE_POPCNT
else ifeq ($(ARCH),x86-64-avx2)
    ARCH_FLAGS     := -mavx2 -mbmi -mbmi2 -mpopcnt -mlzcnt
    ARCH_DEFINES   := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_SSE41 -DUSE_SSE42 -DUSE_POPCNT \
                      -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2
else ifeq ($(ARCH),x86-64-avx512)
    ARCH_FLAGS     := -mavx512f -mavx512bw -mavx512dq -mavx512vl -mavx512vnni \
                      -mavx2 -mbmi -mbmi2 -mpopcnt -mlzcnt
    ARCH_DEFINES   := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_SSE41 -DUSE_SSE42 -DUSE_POPCNT \
                      -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2 -DUSE_AVX512 -DUSE_PEXT
else ifeq ($(ARCH),native)
    ARCH_FLAGS     := -march=native
    ARCH_DEFINES   := -DUSE_SSE2 -DUSE_SSSE3 -DUSE_SSE41 -DUSE_SSE42 -DUSE_POPCNT \
                      -DUSE_BMI1 -DUSE_BMI2 -DUSE_AVX2
else
    $(error ARQUITECTURA DESCONOCIDA: '$(ARCH)'. Opciones validas: \
            x86-64, x86-64-ssse3, x86-64-sse4.2, x86-64-avx2, x86-64-avx512, native)
endif

OPTFLAGS := $(OPTFLAGS_BASE) $(ARCH_FLAGS) $(ARCH_DEFINES)

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
           -pthread

# ----------------------------------------------------------------------------
# FUENTES Fafnir
# ----------------------------------------------------------------------------
SRC_ENGINE := src/ui/main.cpp \
              src/ui/uci.cpp \
              src/engine/SearchController.cpp \
              src/engine/movepicker.cpp \
              src/bridges/nnue_probe.cpp

# ----------------------------------------------------------------------------
# FUENTES DEL PROBE NNUE STOCKFISH DEV
# ----------------------------------------------------------------------------
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
NNUE_FILE := assets/nnue/nn-252f33942263.nnue

# ----------------------------------------------------------------------------
# OBJETOS
# ----------------------------------------------------------------------------
OBJ_ENGINE := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC_ENGINE))
OBJ_PROBE  := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRC_PROBE_CPP))

OBJ := $(OBJ_ENGINE) $(OBJ_PROBE)
DEP := $(OBJ:.o=.d)

# ----------------------------------------------------------------------------
# SISTEMA OPERATIVO
# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
# SISTEMA OPERATIVO + DIALECTO DE SHELL
#   En Windows los recipes de GNU make pueden ejecutarse bajo cmd.exe
#   (GnuWin32) o bajo sh (msys2/Git-for-Windows). Detectamos el dialecto:
#   $(shell echo $$SHELL) devuelve "$SHELL" literal en cmd y una ruta /bin/...
#   al expandirse bajo sh.
# ----------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    # Detección de dialecto: "echo -n ok" imprime "ok" bajo sh y "-n ok"
    # bajo cmd.exe.
    SHELL_DETECT := $(shell echo -n ok 2>&1)
    ifeq ($(SHELL_DETECT),ok)
        # --- sh (msys2 / Git Bash): sintaxis POSIX
        SHELL_POSIX := 1
    endif
endif

ifeq ($(OS),Windows_NT)
    TARGET   := $(BIN_DIR)/Fafnir.exe
    RUN_CMD  := .\bin\Fafnir.exe
    RMDIR    := rmdir /s /q
    DEL_CMD  := del /f /q
    FIXPATH  = $(subst /,\,$1)
    EXE_SUFFIX := .exe
else
    SHELL_POSIX := 1
    TARGET   := $(BIN_DIR)/Fafnir
    RUN_CMD  := ./bin/Fafnir
    RMDIR    := rm -rf
    DEL_CMD  := rm -f
    FIXPATH  = $1
    EXE_SUFFIX :=
endif

ifneq ($(SHELL_POSIX),)
    MKDIR_CMD = mkdir -p $(1)
else
    # cmd mkdir aborta si existe; "if not exist" lo vuelve idempotente
    MKDIR_CMD = if not exist $(1) mkdir $(1)
endif

# ----------------------------------------------------------------------------
# DEFAULT
# ----------------------------------------------------------------------------
.PHONY: all clean run rebuild all-arches help
.DEFAULT_GOAL := all

all: $(TARGET)

help:
	@echo "make                       -> $(ARCH) (defecto: binario seguro x86-64)"
	@echo "make ARCH=x86-64           -> SSE2 (cualquier x86_64)"
	@echo "make ARCH=x86-64-ssse3     -> SSSE3"
	@echo "make ARCH=x86-64-sse4.2    -> SSE4.2 + POPCNT (x86-64-v2)"
	@echo "make ARCH=x86-64-avx2      -> AVX2 + BMI2 + POPCNT (x86-64-v3)"
	@echo "make ARCH=x86-64-avx512    -> AVX-512 + VNNI + PEXT (x86-64-v4)"
	@echo "make ARCH=native           -> optimizada para ESTA maquina"
	@echo "make all-arches            -> bin/Fafnir-<arch> para todas"
	@echo "make run / clean / rebuild"

-include $(DEP)

# ----------------------------------------------------------------------------
# DIRECTORIES
# ----------------------------------------------------------------------------
$(BUILD_DIR) $(BIN_DIR):
	@$(call MKDIR_CMD,$(call FIXPATH,$@))

# ----------------------------------------------------------------------------
# LINK
# ----------------------------------------------------------------------------
$(TARGET): $(OBJ) $(NNUE_FILE) | $(BIN_DIR)
	@echo Enlazando Fafnir + probe NNUE SFNNv17 [ARCH=$(ARCH)]...
	$(CXX) $(CXXFLAGS) $(OBJ) -o $@ $(LDFLAGS)
	@echo Compilacion completada con exito.

$(BUILD_DIR)/$(SFDEV_SRC)/nnue/network.o: $(NNUE_FILE)

# ----------------------------------------------------------------------------
# C++ / C
# ----------------------------------------------------------------------------
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@$(call MKDIR_CMD,$(patsubst %/,%,$(call FIXPATH,$(dir $@))))
	@echo Compilando $<...
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@$(call MKDIR_CMD,$(patsubst %/,%,$(call FIXPATH,$(dir $@))))
	@echo Compilando $<...
	$(CC) $(CFLAGS) -c $< -o $@

# ----------------------------------------------------------------------------
# BINARIOS SUFIJADOS (distribucion): bin/Fafnir-<arch>[.exe]
#   Cada arquitectura compila en su propio build/<ARCH>/ y enlaza a un
#   binario sufijado. Sin bucles POSIX: cada regla invoca MAKE directamente,
#   de modo que funciona igual en sh (Linux/macOS) que en cmd.exe (Windows).
# ----------------------------------------------------------------------------
ARCH_LIST := x86-64 x86-64-ssse3 x86-64-sse4.2 x86-64-avx2 x86-64-avx512

all-arches: $(foreach ARCH_X,$(ARCH_LIST),arch-$(ARCH_X))
	@echo "Binarios generados en $(BIN_DIR):"

arch-%:
	+$(MAKE) --no-print-directory ARCH=$* TARGET=$(BIN_DIR)/Fafnir-$*$(EXE_SUFFIX)

# ----------------------------------------------------------------------------
# RUN
# ----------------------------------------------------------------------------
run: $(TARGET)
	$(RUN_CMD)

# ----------------------------------------------------------------------------
# CLEAN
# ----------------------------------------------------------------------------
clean:
	@echo Limpiando...
	-$(RMDIR) $(call FIXPATH,$(BUILD_DIR))
	-$(RMDIR) $(call FIXPATH,$(BIN_DIR))

# ----------------------------------------------------------------------------
# REBUILD
# ----------------------------------------------------------------------------
rebuild: clean all
