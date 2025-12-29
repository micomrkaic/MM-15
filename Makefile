# --- portability knobs (Linux + macOS) ---
SHELL      := /bin/sh
APP        ?= mm_15

# Build mode: release | asan | ubsan | harden
MODE       ?= release

# Default to user-local install to avoid sudo (/usr/local still available via install-system)
PREFIX     ?= $(HOME)/.local
BINDIR     ?= $(PREFIX)/bin

# Respect XDG on Linux; fall back to ~/.config on macOS/Linux
CONFIG_HOME  ?= $(if $(XDG_CONFIG_HOME),$(XDG_CONFIG_HOME),$(HOME)/.config)
APP_CONF_DIR := $(CONFIG_HOME)/$(APP)

INSTALL    ?= install
MKDIR_P    ?= mkdir -p

# Config files live in data/
DATA_DIR := data
CONFIG_FILES := config.txt predefined_macros.txt
CONFIG_SRC  := $(addprefix $(DATA_DIR)/,$(CONFIG_FILES))

# Compiler and base flags
CC       ?= gcc
CFLAGS   ?= -g -std=c17 -Wall -Wextra -Werror -Wpedantic -Iinclude -fno-common
LDFLAGS  ?=
LDLIBS   ?= -lgsl -lgslcblas -lreadline -lm
CPPFLAGS += -DHOME_DIR='"$(HOME)"'

# Auto-deps: generate .d files per source (write alongside .o)
DEPFLAGS = -MMD -MP -MF $(@:.o=.d) -MT $@

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  CFLAGS  += -I/opt/homebrew/opt/gsl/include
  LDFLAGS += -L/opt/homebrew/opt/gsl/lib
endif

# Directories (mode-specific outputs)
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin/$(MODE)
OBJ_DIR = build/$(MODE)

# --- Git version header (reliable + rebuilds when git state changes) ---
GIT_HEADER := $(OBJ_DIR)/git_version.h
GIT_HASH   := $(shell git rev-parse --short HEAD 2>/dev/null)
GIT_DIRTY  := $(shell git diff-index --quiet HEAD -- 2>/dev/null || echo -dirty)
GIT_VERSION:= $(GIT_HASH)$(GIT_DIRTY)

# Force-include the generated header in every compile
CPPFLAGS += -include $(GIT_HEADER)


# Executable (mode-specific)
TARGET = $(BIN_DIR)/$(APP)

# Sources / objects / deps
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# -------- Mode flags (applied only when MODE matches) --------
ifeq ($(MODE),asan)
  CFLAGS  += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
endif

ifeq ($(MODE),ubsan)
  CFLAGS  += -O1 -g3 -fno-omit-frame-pointer -fsanitize=undefined
  LDFLAGS += -fsanitize=undefined
endif

ifeq ($(MODE),harden)
  CFLAGS  += -O1 -g3 -D_FORTIFY_SOURCE=2 -fno-omit-frame-pointer \
             -Wformat=2 -Wformat-truncation -Wformat-overflow \
             -Wstringop-overflow=2 -Wstringop-truncation \
             -Warray-bounds=2 -Wshadow -Wcast-qual -Wwrite-strings \
             -Wstrict-prototypes -Wmissing-prototypes -Wvla
endif

# -------- Debug/safety convenience targets --------
.PHONY: asan ubsan harden scan-unsafe test-asan

.PHONY: default
default: MODE=release
default: all

asan:
	$(MAKE) MODE=asan all

ubsan:
	$(MAKE) MODE=ubsan all

harden:
	$(MAKE) MODE=harden all

# Quick triage: grep for unsafe string APIs (ripgrep required)
scan-unsafe:
	@echo "Scanning for high-risk string calls..."
	@rg -n '\b(strcpy|strcat|sprintf|vsprintf|gets)\b' $(SRC_DIR) $(INC_DIR) || true
	@echo
	@echo "Scanning for medium-risk calls (audit each use)..."
	@rg -n '\b(strncpy|strncat|snprintf|vsnprintf|memcpy|memmove)\b' $(SRC_DIR) $(INC_DIR) || true

# Abuse tests under ASan build
test-asan: asan
	@echo "Running ASan abuse tests..."
	@python3 -c 'import random,string; \
	[print("".join(random.choice(string.printable) for _ in range(random.randint(0,4000)))) for __ in range(2000)]' \
	| "bin/asan/$(APP)" >/dev/null || true

	@python3 -c 'import random,string; [print("".join(random.choice(string.printable) for _ in range(random.randint(0,4000)))) for __ in range(2000)]' \
	| "bin/asan/$(APP)" >/dev/null || true

	@echo "Done. If ASan found anything, you already saw the stack trace."

# -------- Rules --------
.PHONY: all install install-system uninstall clean doc

# Regenerate git version header when HEAD or index changes
$(GIT_HEADER): .git/HEAD .git/index
	@$(MKDIR_P) "$(@D)"
	@printf '%s\n' \
	  '#pragma once' \
	  '#define VERSION "$(GIT_VERSION)"' \
	  > "$@"


all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	@$(MKDIR_P) "$(@D)"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Compile (+ auto header deps)
# Compile (+ auto header deps)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(GIT_HEADER)
	@$(MKDIR_P) "$(@D)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

# Pull in auto-generated header dependencies (safe if missing)
-include $(DEPS)

# --- User-local install (default) ---
# Default installs release to avoid accidentally installing ASan builds.
install: MODE=release
install: $(TARGET)
	@echo "Installing $(APP) ($(MODE)) binary to $(DESTDIR)$(BINDIR)"
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
	rm -rf build bin

doc:
	doxygen Doxyfile
