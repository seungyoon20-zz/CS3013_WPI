# Make file for doit
# WRITTEN BY: Yixue Wang
# This makefile is used as follows to regenerate files for the life program:
#   make Board.o   --  Regenerates Board.o by compiling
#   make Game.o   --  Regenerates Game.o by compiling
#   make Life.o   --  Regenerates Life.o by compiling
#   make life    --  Regenerates the executable life file by compiling
#   make              --  Same as "make Life”
#

CFLAGS = -g

doit: doit.c
	gcc $(CFLAGS) -Wall doit.c -lm -o doit
clean:
	rm -f doit
all: doit
