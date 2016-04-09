C_FILES := $(wildcard *.c) $(wildcard *.cpp) #nvwa/debug_new.cpp
GENBOT_C := $(wildcard genbot/*.c) $(wildcard genbot/*.cpp)
H_FILES := $(wildcard *.h)
GENBOT_H := $(wildcard genbot/*.h)
.PHONY: all
all: genstock testbots readresults topbotsave
genstock: $(C_FILES) $(H_FILES) $(GENBOT_C) $(GENBOT_H)
	g++ -g -Wall -Wextra -o $@ $(C_FILES) $(GENBOT_C) -I. -lncurses -std=gnu++0x -lm
	#g++ -g -shared -o $@.dll $(C_FILES) -I. -lncurses -std=gnu++0x
testbots: testc/testbots.c testc/testbots.h
	g++ -g -o $@ testc/testbots.c genbot/genome.c genbot/cluster.c genbot/genbot.c mt4pipegen.c -I. -lncurses -std=gnu++0x -lm
readresults: testc/readresults.c
	g++ -g -o $@ testc/readresults.c -I. -std=gnu++0x
topbotsave: testc/topbotsave.c
	g++ -g -o $@ testc/topbotsave.c -I. -std=gnu++0x
evaluatetopbots: testc/evaluatetopbots.c
	g++ -g -o $@ testc/evaluatetopbots.c genbot/genome.c genbot/cluster.c genbot/genbot.c mt4pipegen.c -I. -lncurses -std=gnu++0x
