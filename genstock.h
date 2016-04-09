#ifndef GENSTOCK_HEADER_FILE
#define GENSTOCK_HEADER_FILE


#include <ncurses.h>
#include <iostream>
#include "genbot/cluster.h"
#include "event.h"
#include <string>
#include <fstream>
#include "genbot/genbot.h"
#include "genbot/genome.h"
#include "mt4pipegen.h"

//---simulation parameters
int BARSBACK = 100;
#define NUMINPUTS BARSBACK+2
#define TESTPERIOD "M5"

int TRADECANDLELIMIT = 15;

int NUMGENBOTS = 3;
int NUMWINBOTS = 2;
int NUMSTATICTOPBOTS = 0;
int NUMPARENTBOTS = 2;
int NUMTRAINCYCLES = 5 ;
int CHILD_INHERITS_PARENT_LEARNING = 1;

#define NUMOUTPUTS 2
int LEARNDIVISOR = 500;

int CHECK_TEST = 0;
int CHECK_TRAIN = 1;

int USE_TRAILING_STOP = 1;

#define SKIP_TRAIN_ON_ROUND_1 0
#define SKIP_TEST_ON_ROUND_1 0

int TEST1_TIMEPERIOD = 6;
int TRAIN_TIMEPERIOD = 6;

int TEST2_TESTSAMPLE = 0;
int TEST2_TIMEPERIOD = 1;

int NUM_TRAIN_THREADS = 3;
//--------------------------

static ConvolutionProperties defaultConvProp = {
    1, {1}, 2, NUMINPUTS-1, {NUMINPUTS-2}, 1, 1, 1, 1
};

#endif
