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

CONFIG_FILES := config.txt predefined_macros.txt

# Compiler and flags
CC = clang
CFLAGS = -g -std=c11 -Wall -Wextra -Werror -Wpedantic -Iinclude
LDFLAGS =
LDLIBS = -lgsl -lgslcblas -lreadline -lm
CPPFLAGS += -DHOME_DIR='"$(HOME)"'

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

# Sources / objects
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# -------- Rules --------
.PHONY: all install install-system uninstall clean doc

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@$(MKDIR_P) "$(BIN_DIR)"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKDIR_P) "$(OBJ_DIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# --- User-local install (default) ---
install: $(TARGET)
	@echo "Installing binary to $(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 755 "$(TARGET)" "$(DESTDIR)$(BINDIR)/$(APP)"

	@echo "Installing config to $(APP_CONF_DIR)"
	$(INSTALL) -d "$(APP_CONF_DIR)"
	@for f in $(CONFIG_FILES); do \
	  if [ ! -f "$(APP_CONF_DIR)/$$f" ]; then \
	    echo "  -> $$f (new)"; \
	    $(INSTALL) -m 644 "$$f" "$(APP_CONF_DIR)/$$f"; \
	  else \
	    echo "  -> $$f exists; keeping user's copy"; \
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
