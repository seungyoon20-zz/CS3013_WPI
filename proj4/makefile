#Makefile for proj4 CS3013
# WRITTEN BY: Yixue Wang, Oct 3, 2017
# This makefile is used as follows to regenerate files for the mailbox program:
#   make mailbox   --  Regenerates the executable life file by compiling
#   make              --  Same as "make mailbox”
#

#CFLAGS = -pthread gcc $(CFLAGS) -Wall main.c -lm -o mailbox

proj4: proj4.c
	gcc proj4.c -o proj4 -lpthread

clean:
	rm -f proj4

all: proj4