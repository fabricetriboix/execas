SHELL := /bin/sh
CC = gcc
CFLAGS = -O -Wall

all: execas test

execas: execas.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f execas
