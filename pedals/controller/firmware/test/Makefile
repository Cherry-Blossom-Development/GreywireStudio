# PC simulation of the loop (see sim.c). Needs any C compiler: make -C test
CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -O1 -g -I../core

sim: sim.c ../core/lw_proto.c ../core/switchyard.c ../core/pedal.c ../core/*.h
	$(CC) $(CFLAGS) -o $@ sim.c ../core/lw_proto.c ../core/switchyard.c ../core/pedal.c

test: sim
	./sim

clean:
	rm -f sim sim.exe

.PHONY: test clean
