CC = gcc
FLAGS = -O2 -Wall -Wextra -pedantic -std=c89
FILES = main.c tokenize.c parse.c codegen.c

all:
	$(CC) $(FLAGS) $(FILES) -o pun
