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
#define BARSBACK 100
#define TRADECANDLELIMIT 15
#define TESTPERIOD "M5"

#define NUMGENBOTS 50
#define NUMWINBOTS 30
#define NUMSTATICTOPBOTS 15
#define NUMPARENTBOTS 15
#define NUMTRAINCYCLES 100
#define CHILD_INHERITS_PARENT_LEARNING 1

#define NUMOUTPUTS 2
#define LEARNDIVISOR 500
#define NUMINPUTS (BARSBACK+2)

#define SKIP_TRAIN_ON_ROUND_1 0
#define SKIP_TEST_ON_ROUND_1 0

#define CHECK_TEST 0
#define CHECK_TRAIN 1

#define USE_TRAILING_STOP 1

#define TEST1_TIMEPERIOD 6
#define TRAIN_TIMEPERIOD 6

#define TEST2_TESTSAMPLE 0
#define TEST2_TIMEPERIOD 1

#define NUM_TRAIN_THREADS 4
//--------------------------

static ConvolutionProperties defaultConvProp = {
    1, {1}, 2, NUMINPUTS-1, {NUMINPUTS-2}, 1, 1, 1, 1
};

#endif
