# Makefile for shader_compiler
# Fabjan Sukalia <fsukalia@gmail.com>
# 2016-03-26

.PHONY: all clean

PROG=shader_compiler

ifeq ($(CC),)
CC=gcc
endif

all: $(PROG)

$(PROG): main.c
	$(CC) -std=c99 -Wall -Wextra -lEGL -lGLESv2 -lX11 -o $@ $^

clean:
	rm -f $(PROG)
