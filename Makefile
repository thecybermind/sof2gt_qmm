# SoF2GT_QMM - Hook gametype dlls/qvms for Soldier of Fortune 2
# Copyright 2025-2026
# https://github.com/thecybermind/sof2gt_qmm/
# 3-clause BSD license: https://opensource.org/license/bsd-3-clause
# Created By: Kevin Masterson < k.m.masterson@gmail.com >

BIN := sof2gt_qmm
GAMES := SOF2MP

CC := gcc
CXXC := g++

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

SRC_C   := $(wildcard $(SRC_DIR)/*.c)
SRC_CXX := $(wildcard $(SRC_DIR)/*.cpp)

OBJ_FILES := $(SRC_CXX:$(SRC_DIR)/%.cpp=%.o) $(SRC_C:$(SRC_DIR)/%.c=%.o)

CPPFLAGS := -MMD -MP -I ./include -isystem ../qmm_sdks -isystem ../qmm2/include
CFLAGS   := -Wall -Werror -pipe -fPIC -std=gnu17
CXXFLAGS := -Wall -Werror -pipe -fPIC -std=c++17
LDFLAGS  := -shared -fPIC
LDLIBS   :=

REL_CPPFLAGS := $(CPPFLAGS) -DNDEBUG
DBG_CPPFLAGS := $(CPPFLAGS) -D_DEBUG

REL_CFLAGS := $(CFLAGS) -m32 -O2 -falign-functions=16 -fno-strict-aliasing -fstrength-reduce
DBG_CFLAGS := $(CFLAGS) -m32 -g -pg -fsanitize=undefined -fsanitize=address

REL_CXXFLAGS := $(CXXFLAGS) -m32 -O2 -falign-functions=16 -fno-strict-aliasing -fstrength-reduce
DBG_CXXFLAGS := $(CXXFLAGS) -m32 -g -pg -fsanitize=undefined -fsanitize=address

REL_LDFLAGS := $(LDFLAGS) -m32
DBG_LDFLAGS := $(LDFLAGS) -m32 -g -pg

.PHONY: help all clean release debug

help:
	@echo make targets:
	@echo all: release debug
	@echo release: [32-bit release build]
	@echo debug: [32-bit debug build]

all: release debug
release: $(BIN_DIR)/release/x86/$(BIN).so
debug: $(BIN_DIR)/debug/x86/$(BIN).so

$(BIN_DIR)/release/x86/$(BIN).so: $(addprefix $(OBJ_DIR)/release/x86/,$(OBJ_FILES))
	mkdir -p $(@D)
	$(CXXC) $(REL_LDFLAGS) -o $@ $(LDLIBS) $^

$(BIN_DIR)/debug/x86/$(BIN).so: $(addprefix $(OBJ_DIR)/debug/x86/,$(OBJ_FILES))
	mkdir -p $(@D)
	$(CXXC) $(DBG_LDFLAGS) -o $@ $(LDLIBS) $^

$(OBJ_DIR)/release/x86/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXXC) $(REL_CPPFLAGS) $(REL_CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/release/x86/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(REL_CPPFLAGS) $(REL_CFLAGS) -c $< -o $@

$(OBJ_DIR)/debug/x86/%.o: $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CXXC) $(DBG_CPPFLAGS) $(DBG_CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/debug/x86/%.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(DBG_CPPFLAGS) $(DBG_CFLAGS) -c $< -o $@

-include $(addprefix $(OBJ_DIR)/release/x86/,$(OBJ_FILES:.o=.d))
-include $(addprefix $(OBJ_DIR)/debug/x86/,$(OBJ_FILES:.o=.d))

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)
