#Makefile for proj3 CS3013
# WRITTEN BY: Yixue Wang, Sept 28, 2017
# This makefile is used as follows to regenerate files for the mailbox program:
#   make mailbox   --  Regenerates the executable life file by compiling
#   make              --  Same as "make mailbox”
#

CFLAGS = -pthread

mailbox: main.c
	gcc $(CFLAGS) -Wall main.c -lm -o mailbox

clean:
	rm -f mailbox

all: mailbox