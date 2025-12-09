GIT_VERSION := $(shell git describe --always --dirty 2>/dev/null)
CPPFLAGS += -DVERSION='"$(GIT_VERSION)"'


# --- portability knobs (Linux + macOS) ---
SHELL      := /bin/sh
APP        ?= mm_15

# Default to user-local install to avoid sudo (/usr/local still available via install-system)
PREFIX     ?= $(HOME)/.local
BINDIR     ?= $(PREFIX)/bin

# Respect XDG on Linux; fall back to ~/.config on macOS/Linux
CONFIG_HOME ?= $(if $(XDG_CONFIG_HOME),$(XDG_CONFIG_HOME),$(HOME)/.config)
APP_CONF_DIR := $(CONFIG_HOME)/$(APP)

INSTALL    ?= install
MKDIR_P    ?= mkdir -p

# Config files live in data/
DATA_DIR := data
CONFIG_FILES := config.txt predefined_macros.txt
CONFIG_SRC  := $(addprefix $(DATA_DIR)/,$(CONFIG_FILES))

# Compiler and flags
CC = gcc
CFLAGS = -g -std=c17 -Wall -Wextra -Werror -Wpedantic -Iinclude -fno-common
LDFLAGS =
LDLIBS = -lgsl -lgslcblas -lreadline -lm
CPPFLAGS += -DHOME_DIR='"$(HOME)"'

# Auto-deps: generate .d files per source
DEPFLAGS = -MMD -MP

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CFLAGS  += -I/opt/homebrew/opt/gsl/include
  LDFLAGS += -L/opt/homebrew/opt/gsl/lib
endif

# Directories
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin
OBJ_DIR = build

# Executable
TARGET = $(BIN_DIR)/$(APP)

# Sources / objects / deps
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# -------- Rules --------
.PHONY: all install install-system uninstall clean doc

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@$(MKDIR_P) "$(BIN_DIR)"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile (+ auto header deps)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_P) "$(OBJ_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Pull in auto-generated header dependencies (safe if missing)
-include $(DEPS)

# --- User-local install (default) ---
install: $(TARGET)
	@echo "Installing binary to $(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/$(APP)"

	@echo "Installing config to $(APP_CONF_DIR)"
	$(INSTALL) -d "$(APP_CONF_DIR)"
	@for f in $(CONFIG_SRC); do \
	  base=$$(basename "$$f"); \
	  if [ ! -f "$(APP_CONF_DIR)/$$base" ]; then \
	    echo "  -> $$base (new)"; \
	    $(INSTALL) -m 644 "$$f" "$(APP_CONF_DIR)/$$base"; \
	  else \
	    echo "  -> $$base exists; keeping user's copy"; \
	  fi; \
	done

# --- System-wide install (optional): `make install-system` ---
install-system: PREFIX=/usr/local
install-system: BINDIR=$(PREFIX)/bin
install-system: install

uninstall:
	@echo "Removing $(DESTDIR)$(BINDIR)/$(APP)"
	@rm -f "$(DESTDIR)$(BINDIR)/$(APP)"
	@echo "Left $(APP_CONF_DIR) in place (user data)."

clean:
	rm -rf "$(OBJ_DIR)" "$(BIN_DIR)"

doc:
	doxygen Doxyfile

