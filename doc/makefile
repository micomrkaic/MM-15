CC = clang
#CC = gcc

CFLAGS = -g -gdwarf-4 -std=c11 -Wall -Wextra -Wpedantic -Werror -Werror=incompatible-pointer-types
#CFLAGS = -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -Werror=incompatible-pointer-types

LIBS = -lgsl -lgslcblas -lm -lreadline -lhistory

TARGET = mmrpn

SRC = 	main.c  \
	lexer.c \
	stack.c \
	string_fun.c \
	math_helpers.c \
	math_parsers.c \
	matrix_fun.c \
	linear_algebra.c \
	binary_fun.c \
	unary_fun.c \
	eval_fun.c \
	stat_fun.c \
	registers.c \
	poly_fun.c \
	splash_and_help.c \
	function_list.c \
	tab_completion.c \
	my_date_fun.c \
	print_fun.c \
	globals.c \
	spec_fun.c \
	compare_fun.c \
	words.c \
	run_machine.c

OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

