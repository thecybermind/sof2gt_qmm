# STUB_QMM - Example QMM Plugin
# Copyright 2025-2026
# https://github.com/thecybermind/sof2gt_qmm/
# 3-clause BSD license: https://opensource.org/license/bsd-3-clause
# Created By: Kevin Masterson < k.m.masterson@gmail.com >

BIN_32 := sof2gt_qmm
GAMES := SOF2MP

CC := g++

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)

OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.cpp=%.o)

CPPFLAGS := -MMD -MP -I ./include -isystem ../qmm_sdks -isystem ../qmm2/include
CFLAGS   := -Wall -pipe -fPIC
LDFLAGS  := -shared -fPIC
LDLIBS   :=

REL_CPPFLAGS := $(CPPFLAGS)
DBG_CPPFLAGS := $(CPPFLAGS) -D_DEBUG

REL_CFLAGS_32 := $(CFLAGS) -m32 -O2 -ffast-math -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing -fstrength-reduce 
DBG_CFLAGS_32 := $(CFLAGS) -m32 -g -pg

REL_LDFLAGS_32 := $(LDFLAGS) -m32
DBG_LDFLAGS_32 := $(LDFLAGS) -m32 -g -pg

.PHONY: help all clean release debug release32 debug32 $(addprefix game-,$(GAMES)) $(addprefix release-,$(GAMES)) $(addprefix debug-,$(GAMES))

help:
	@echo make targets:
	@echo all: release debug
	@echo release: release32
	@echo release32: [32-bit release builds]
	@echo debug: debug32
	@echo debug32: [32-bit debug builds]
	@echo game-[GAME]: release-[GAME] debug-[GAME]
	@echo release-[GAME]: release32-[GAME]
	@echo debug-[GAME]: debug32-[GAME]
	@echo release32-[GAME]: [32-bit release build for GAME]
	@echo debug32-[GAME]: [32-bit debug build for GAME]

all: release debug
release: release32
release32: $(addprefix release32-,$(GAMES))
debug: debug32
debug32: $(addprefix debug32-,$(GAMES))

define gen_rules
game-$(1): release-$(1) debug-$(1)
release-$(1): release32-$(1)
debug-$(1): debug32-$(1)
release32-$(1): $(BIN_DIR)/release-$(1)/x86/$(BIN_32)_$(1).so
debug32-$(1): $(BIN_DIR)/debug-$(1)/x86/$(BIN_32)_$(1).so

$(BIN_DIR)/release-$(1)/x86/$(BIN_32)_$(1).so: $$(addprefix $(OBJ_DIR)/release-$(1)/x86/,$(OBJ_FILES))
	mkdir -p $$(@D)
	$(CC) $(REL_LDFLAGS_32) -o $$@ $(LDLIBS) $$^

$(BIN_DIR)/debug-$(1)/x86/$(BIN_32)_$(1).so: $$(addprefix $(OBJ_DIR)/debug-$(1)/x86/,$(OBJ_FILES))
	mkdir -p $$(@D)
	$(CC) $(DBG_LDFLAGS_32) -o $$@ $(LDLIBS) $$^

$(OBJ_DIR)/release-$(1)/x86/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $$(@D)
	$(CC) $(REL_CPPFLAGS) -DGAME_$(1) $(REL_CFLAGS_32) -c $$< -o $$@

$(OBJ_DIR)/debug-$(1)/x86/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $$(@D)
	$(CC) $(DBG_CPPFLAGS) -DGAME_$(1) $(DBG_CFLAGS_32) -c $$< -o $$@

-include $$(addprefix $(OBJ_DIR)/release-$(1)/x86/,$(OBJ_FILES:.o=.d))
-include $$(addprefix $(OBJ_DIR)/debug-$(1)/x86/,$(OBJ_FILES:.o=.d))
endef
$(foreach game,$(GAMES),$(eval $(call gen_rules,$(game))))

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)
