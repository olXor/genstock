C_FILES := $(wildcard *.c) $(wildcard *.cpp) #nvwa/debug_new.cpp
H_FILES := $(wildcard *.h)
.PHONY: all
all: genstock testbots readresults topbotsave evaluatetopbots
genstock: $(C_FILES) $(H_FILES)
	g++ -g -o $@ $(C_FILES) -I. -lncurses -std=gnu++0x -lm
	#g++ -g -shared -o $@.dll $(C_FILES) -I. -lncurses -std=gnu++0x
testbots: testc/testbots.c testc/testbots.h
	g++ -g -o $@ testc/testbots.c genome.c cluster.c genbot.c mt4pipegen.c -I. -lncurses -std=gnu++0x -lm
readresults: testc/readresults.c
	g++ -g -o $@ testc/readresults.c -I. -std=gnu++0x
topbotsave: testc/topbotsave.c
	g++ -g -o $@ testc/topbotsave.c -I. -std=gnu++0x
evaluatetopbots: testc/evaluatetopbots.c
	g++ -g -o $@ testc/evaluatetopbots.c genome.c cluster.c genbot.c mt4pipegen.c -I. -lncurses -std=gnu++0x
