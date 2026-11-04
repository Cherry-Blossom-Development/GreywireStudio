# PC simulation of the loop (see sim.c) and the bridge for the test tool (see bridge.c).
# Needs any C compiler: make -C test test
CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-function -O1 -g -I../core
CORE = ../core/lw_proto.c ../core/switchyard.c ../core/pedal.c

all: sim bridge

sim: sim.c world.h $(CORE) ../core/*.h
	$(CC) $(CFLAGS) -o $@ sim.c $(CORE)

bridge: bridge.c world.h $(CORE) ../core/*.h
	$(CC) $(CFLAGS) -o $@ bridge.c $(CORE)

test: sim
	./sim

clean:
	rm -f sim sim.exe bridge bridge.exe

.PHONY: all test clean
