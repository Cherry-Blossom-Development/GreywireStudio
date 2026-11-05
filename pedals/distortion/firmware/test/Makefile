# PC simulation of Flashover on a loop with Switchyard and a stand-in pedal (see fo_sim.c).
# Needs any C compiler: make -C test test
ifeq ($(origin CC),default)
CC = gcc
endif
LW = ../../../../libraries/linework
SY = ../../../controller/firmware/core
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-function -O1 -g -I../core -I$(LW) -I$(SY)
SRC = ../core/flashover.c $(LW)/lw_ui.c $(LW)/lw_proto.c $(LW)/pedal.c $(LW)/pd_store.c $(SY)/switchyard.c

all: fo_sim

fo_sim: fo_sim.c $(SRC) ../core/*.h $(LW)/*.h $(SY)/*.h
	$(CC) $(CFLAGS) -o $@ fo_sim.c $(SRC)

test: fo_sim
	./fo_sim

clean:
	rm -f fo_sim fo_sim.exe

.PHONY: all test clean
