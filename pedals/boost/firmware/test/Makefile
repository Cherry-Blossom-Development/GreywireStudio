# PC simulation of Tap Changer on a loop with Switchyard and a stand-in pedal (see tc_sim.c).
# Needs any C compiler: make -C test test
ifeq ($(origin CC),default)
CC = gcc
endif
LW = ../../../../libraries/linework
SY = ../../../controller/firmware/core
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-function -O1 -g -I../core -I$(LW) -I$(SY)
SRC = ../core/tapchanger.c $(LW)/lw_ui.c $(LW)/lw_proto.c $(LW)/pedal.c $(LW)/pd_store.c $(SY)/switchyard.c

all: tc_sim

tc_sim: tc_sim.c $(SRC) ../core/*.h $(LW)/*.h $(SY)/*.h
	$(CC) $(CFLAGS) -o $@ tc_sim.c $(SRC) -lm

test: tc_sim
	./tc_sim

clean:
	rm -f tc_sim tc_sim.exe

.PHONY: all test clean
