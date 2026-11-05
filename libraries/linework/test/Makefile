# PC simulation of a firmware update over the Linework loop (see fw_sim.c).
# Needs any C compiler: make -C test test
ifeq ($(origin CC),default)
CC = gcc
endif
LW = ..
SY = ../../../pedals/controller/firmware/core
FO = ../../../pedals/distortion/firmware/core
CFLAGS = -std=c11 -Wall -Wextra -Wno-unused-function -O1 -g -I$(LW) -I$(SY) -I$(FO)
SRC = $(LW)/lw_fw.c $(LW)/lw_ui.c $(LW)/lw_proto.c $(LW)/pedal.c $(LW)/pd_store.c $(SY)/switchyard.c $(FO)/flashover.c

all: fw_sim

fw_sim: fw_sim.c $(SRC) $(LW)/*.h $(SY)/*.h $(FO)/*.h
	$(CC) $(CFLAGS) -o $@ fw_sim.c $(SRC)

test: fw_sim
	./fw_sim

clean:
	rm -f fw_sim fw_sim.exe

.PHONY: all test clean
