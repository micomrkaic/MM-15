#ifndef ANSI_COLORS_H
#define ANSI_COLORS_H

// Reset
#define RESET       "\033[0m"

// Text styles
#define BOLD        "\033[1m"
#define FAINT       "\033[2m"
#define ITALIC      "\033[3m"
#define UNDERLINE   "\033[4m"
#define BLINK       "\033[5m"
#define INVERSE     "\033[7m"
#define HIDDEN      "\033[8m"
#define STRIKE      "\033[9m"

// Foreground (text) colors
#define FG_BLACK    "\033[30m"
#define FG_RED      "\033[31m"
#define FG_GREEN    "\033[32m"
#define FG_YELLOW   "\033[33m"
#define FG_BLUE     "\033[34m"
#define FG_MAGENTA  "\033[35m"
#define FG_CYAN     "\033[36m"
#define FG_WHITE    "\033[37m"
#define FG_DEFAULT  "\033[39m"

// Background colors
#define BG_BLACK    "\033[40m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"
#define BG_MAGENTA  "\033[45m"
#define BG_CYAN     "\033[46m"
#define BG_WHITE    "\033[47m"
#define BG_DEFAULT  "\033[49m"

// RGB / 256-color mode
#define FG_256(n)   "\033[38;5;" #n "m"
#define BG_256(n)   "\033[48;5;" #n "m"

// Cursor control
#define CURSOR_UP(n)       "\033[" #n "A"
#define CURSOR_DOWN(n)     "\033[" #n "B"
#define CURSOR_FORWARD(n)  "\033[" #n "C"
#define CURSOR_BACK(n)     "\033[" #n "D"
#define CURSOR_COL(n)      "\033[" #n "G"
#define CURSOR_SAVE        "\033[s"
#define CURSOR_RESTORE     "\033[u"

// Screen control
#define CLEAR_SCREEN       "\033[2J"
#define CLEAR_LINE         "\033[K"

#endif // ANSI_COLORS_H
