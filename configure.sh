#!/bin/bash
echo 'termpath="/cygdrive/c/Program\ Files\ \(x86\)/MetaTrader\ 4/terminal.exe"' > path.cfg
echo 'abstermpath="C:/Program Files (x86)/MetaTrader 4/terminal.exe"' >> path.cfg
echo 'datapath="/cygdrive/c/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702"' >> path.cfg
echo 'absdatapath="C:/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702"' >> path.cfg

echo 'BARSBACK 100' > sim.cfg
echo 'TRADECANDLELIMIT 15' >> sim.cfg
echo 'NUMGENBOTS 3' >> sim.cfg
echo 'NUMWINBOTS 2' >> sim.cfg
echo 'NUMSTATICTOPBOTS 0' >> sim.cfg
echo 'NUMPARENTBOTS 2' >> sim.cfg
echo 'NUMTRAINCYCLES 5' >> sim.cfg
echo 'CHILD_INHERITS_PARENT_LEARNING 1' >> sim.cfg
echo 'LEARNDIVISOR 500' >> sim.cfg
echo 'CHECK_TEST 0' >> sim.cfg
echo 'CHECK_TRAIN 1' >> sim.cfg
echo 'USE_TRAILING_STOP 1' >> sim.cfg
echo 'TEST1_TIMEPERIOD 6' >> sim.cfg
echo 'TRAIN_TIMEPERIOD 6' >> sim.cfg
echo 'TEST2_TESTSAMPLE 0' >> sim.cfg
echo 'TEST2_TIMEPERIOD 1' >> sim.cfg
echo 'NUM_TRAIN_THREADS 3' >> sim.cfg

echo 'CHILDDEPTH 1' > genbot/gen.cfg
echo 'ALLOW_SIDE_WEIGHTS 0' >> genbot/gen.cfg
echo 'ALLOW_SIDE_MEMS 0' >> genbot/gen.cfg
echo 'MAX_NODESPERLAYER 200' >> genbot/gen.cfg
echo 'MAX_LAYERS 2' >> genbot/gen.cfg
echo 'MAX_NUMPERTURBS 0' >> genbot/gen.cfg
echo 'MAX_CONVOLUTION_LEVELS 2' >> genbot/gen.cfg
echo 'MIN_CONVOLUTION_LEVELS 2' >> genbot/gen.cfg
echo 'MAX_CONVOLUTIONS 3' >> genbot/gen.cfg
echo 'MIN_CONVOLUTIONS 1' >> genbot/gen.cfg
echo 'MAX_CONVOLUTION_NODE_LAYERS 1' >> genbot/gen.cfg
echo 'MAX_CONVOLUTION_NODESPERLAYER 10' >> genbot/gen.cfg
echo 'MAX_CONVOLUTION_DIMENSION 10' >> genbot/gen.cfg
echo 'MIN_CONVOLUTION_DIMENSION 2' >> genbot/gen.cfg
echo 'CONVOLUTION_DIMENSION_LAYER_MULTIPLIER 2.0' >> genbot/gen.cfg
echo 'NUM_TURNS_SAVED 15' >> genbot/gen.cfg
