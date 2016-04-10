#property copyright ""
#property link      ""
#property version   "1.00"
#property strict

//--- input parameters
input int learnDivisor;
input bool train;
input int sampleNum;
input bool useTilt;
input int barsBack;
input int tradeCandleLimit;
input bool useTrailingStop;
input string savefile;

#define numInputs (barsBack+2)
//#define period PERIOD_M15

string symbol = Symbol();

int lastTradeTicket = -1;
bool pendingLearn = false;
int waitTimer = 0;
bool useWaitTimer = true;

static datetime prevtime = 0;

double longYesTotal;
double longNoTotal;
double shortYesTotal;
double shortNoTotal;

int fileids[];
int filehandles[];
int numfiles = 0;

double stoplosses[];
double traildistances[];
int tradeids[];
int tradenum = 0;

#define GENERIC_READ                   0x80000000
#define GENERIC_WRITE                  0x40000000
#define OPEN_EXISTING                  3
#define INVALID_HANDLE_VALUE           -1
#define FILE_ATTRIBUTE_NORMAL          0x80

int pipe;
bool filesAreOpened = false;

string readStringFromPipe() {
    while(FileSize(pipe) <= 0)
        Sleep(1);
    string str = FileReadString(pipe, FileSize(pipe));
    FileFlush(pipe);
    FileSeek(pipe, 0, SEEK_SET);
    return str;
}

bool getOutputs(int &outputids[], double &longoutput[], double &shortoutput[]) {
    string str = readStringFromPipe();
    string outputs[];
    if(StringSplit(str, ' ', outputs) % 3 != 0) {
        Print("Output tokens not a multiple of 3!");
        return false;
    }

    int numOutputTriplets = ArraySize(outputs)/3;
    if(ArraySize(outputids) != numOutputTriplets) {
        ArrayResize(outputids, numOutputTriplets);
        ArrayResize(longoutput, numOutputTriplets);
        ArrayResize(shortoutput, numOutputTriplets);
    }
    for(int i=0; i<numOutputTriplets; i++) {
        outputids[i] = (int)StringToInteger(outputs[3*i]);
        longoutput[i] = StringToDouble(outputs[3*i+1]);
        shortoutput[i] = StringToDouble(outputs[3*i+2]);
    }

    if(!filesAreOpened) {
        numfiles = numOutputTriplets;
        ArrayResize(fileids, numfiles);
        ArrayResize(filehandles, numfiles);
        for(int i=0; i<numfiles; i++) {
            fileids[i] = outputids[i];
            filehandles[i] = FileOpen("genstockReports/testResults/" + savefile + IntegerToString(fileids[i]), FILE_WRITE | FILE_TXT);
            if(filehandles[numfiles-1] == INVALID_HANDLE_VALUE)
                Print("FAILED TO OPEN OUTPUT FILE");
        }
        filesAreOpened = true;
    }

    return true;
}

bool sendInputsToPipe(double &inputs[]) {
    if(pipe == INVALID_HANDLE_VALUE)
        return false;
    string output = "";
    if(numInputs == 0)
        return true;
    output += DoubleToString(inputs[0]);
    for(int i=1; i<numInputs; i++) {
        output += " ";
        output += DoubleToString(inputs[i]);
    }
    if(FileWriteString(pipe, output, StringLen(output)) < (double)StringLen(output)) {
        Print("Failed to write input string");
        return false;
    }
    FileFlush(pipe);
    FileSeek(pipe, 0, SEEK_SET);
    return true;
}

bool sendLearnToPipe(double pp, bool longTrade) {
    if(pipe == INVALID_HANDLE_VALUE)
        return false;
    int l;
    if(longTrade) l = 1;
    else l = 0;

    string output = "learn: " + DoubleToString(pp) + " " + IntegerToString(l);
    if(FileWriteString(pipe, output, StringLen(output)) < (double)StringLen(output)) {
        Print("Failed to write learn string");
        return false;
    }
    FileFlush(pipe);
    FileSeek(pipe, 0, SEEK_SET);
    return true;
}

int newWaitTimer() {
    return MathRand()%barsBack;
}

int OnInit() {
    ArrayResize(fileids, 1);
    ArrayResize(filehandles, 1);
    ArrayInitialize(fileids, EMPTY_VALUE);
    ArrayInitialize(filehandles, EMPTY_VALUE);
    waitTimer = newWaitTimer();
    Print("Trying to open pipe");
    pipe = FileOpen("\\\\.\\pipe\\mt4pipe",FILE_READ|FILE_WRITE|FILE_BIN);
    if(pipe != INVALID_HANDLE_VALUE)
        Print("Client pipe opened");
    else {
        Print("Failed to open pipe!");
        return(INIT_FAILED);
    }

    return(INIT_SUCCEEDED);
}

void onDeinit(const int reason) {
    for(int i=0; i<ArraySize(filehandles); i++) {
        FileClose(filehandles[i]);
    }
    FileClose(pipe);
    ArrayFree(stoplosses);
    ArrayFree(traildistances);
    ArrayFree(tradeids);
}

void makeTrade(int type, int id) {
   double price = 0;
   double takeprofit = 0;
   double stoploss = 0;
   double traildistance = 0;
    if(type==OP_BUY) {
        price=Ask; 
        stoploss=NormalizeDouble(Bid-tradeCandleLimit*iATR(NULL,0,barsBack,0),Digits); 
        takeprofit=NormalizeDouble(Ask+tradeCandleLimit*iATR(NULL,0,barsBack,0),Digits);    
    }
    else if(type==OP_SELL) {
        price=Bid; 
        stoploss=NormalizeDouble(Ask+tradeCandleLimit*iATR(NULL,0,barsBack,0),Digits); 
        takeprofit=NormalizeDouble(Bid-tradeCandleLimit*iATR(NULL,0,barsBack,0),Digits);
    }
    if(useTrailingStop)
        traildistance=price-stoploss;
    //--- calculated SL and TP prices must be normalized 
    //--- place market order to buy 1 lot 
    ArrayResize(tradeids, tradenum+1, 1000);
    tradeids[tradenum] = id;
    if(!useTrailingStop)
        lastTradeTicket=OrderSend(Symbol(),type,1,price,3,stoploss,takeprofit, NULL, tradenum);   //right now test trades won't print properly without a trailing stop
    else {
        ArrayResize(stoplosses, tradenum+1, 1000);
        ArrayResize(traildistances, tradenum+1, 1000);
        stoplosses[tradenum] = stoploss;
        traildistances[tradenum] = traildistance;
        lastTradeTicket=OrderSend(Symbol(),type,1,price,3,0,0, IntegerToString(tradenum-1) + " " + DoubleToString(traildistance), tradenum);
        //Print(IntegerToString(numstoplosses-1), " ", DoubleToString(traildistance));
    }
    tradenum++;
    if(lastTradeTicket<0)
    { 
        Print("OrderSend failed with error #",GetLastError()); 
    } 
    else  {
        //Print("OrderSend placed successfully; price: ", price, " stoploss: "); 
        pendingLearn = true;
    }
    
    if(useWaitTimer)
        waitTimer = newWaitTimer();
}

bool tickInSample() {
    if(sampleNum == 0)
        return true;
    else if(sampleNum == 1)
        return Day()%3 == 0;
    else if(sampleNum == 2)
        return Day()%3 != 0;
    return true;
}

//Note: must have called OrderSelect before calling this function
void printOrderResult() {
    int id = tradeids[OrderMagicNumber()];
    int fileloc = -1;
    for(int i=0; i<numfiles; i++) {
        if(fileids[i] == id) {
            fileloc = i;
            break;
        }
    }

    if(fileloc < 0) {
        Print("COULD NOT FIND THE CORRECT ORDER OUTPUT FILE");
        return;
    }

    string tradetype;
    if(OrderType() == OP_BUY)
        tradetype = "LONG";
    else if(OrderType() == OP_SELL)
        tradetype = "SHORT";
    else
        tradetype = "ERROR";

    if(FileWriteString(filehandles[fileloc], DoubleToString(OrderProfit()) + " " + tradetype + " " + (string)OrderOpenTime() + " " + (string)TimeCurrent() + " " + DoubleToString(OrderOpenPrice()) + " " + DoubleToString(OrderClosePrice()) + "\n") == 0)
        Print("FAILED TO WRITE TO OUTPUT FILE");
}

void updateStop() {
    if(!useTrailingStop) return;
    for(int i=0; i<OrdersTotal(); i++) {
        if(OrderSelect(i, SELECT_BY_POS)) {
            int stopnum = OrderMagicNumber();
            if(OrderType() == OP_BUY) {
                stoplosses[stopnum] = MathMax(stoplosses[stopnum], NormalizeDouble(Bid-traildistances[stopnum],Digits));
                if(Bid <= stoplosses[stopnum]) {
                    if(!OrderClose(OrderTicket(), 1, Bid, 3))
                        Print("Failed to close order");
                    else if(!train)
                        printOrderResult();
                }
            }
            else {
                stoplosses[stopnum] = MathMin(stoplosses[stopnum], NormalizeDouble(Ask-traildistances[stopnum],Digits));
                if(Ask >= stoplosses[stopnum]) {
                    if(!OrderClose(OrderTicket(), 1, Ask, 3))
                        Print("Failed to close order");
                    else if(!train)
                        printOrderResult();
                }
            }
        }
        else
            Print("Failed to update order stop: OrderSelect failed");
    }
}

void OnTick() {
    if(OrdersTotal() > 0)
        updateStop();

    if(prevtime == Time[0]) return;
    prevtime = Time[0];
    if(!tickInSample())
        return;

    if(train && OrdersTotal() > 0)
        return;
    
    if(useWaitTimer && waitTimer > 0) {
        waitTimer--;
        return;
    }
    
    if(!train && useWaitTimer) {
        waitTimer = newWaitTimer();
    }
    
    
    /* ---- set up inputs ---- */
    double rawPriceHighs[];
    ArrayResize(rawPriceHighs, barsBack);
    double rawPriceLows[];
    ArrayResize(rawPriceLows, barsBack);
    double rawPriceOpens[];
    ArrayResize(rawPriceOpens, barsBack);
    double rawPriceCloses[];
    ArrayResize(rawPriceCloses, barsBack);
    double rawPriceAverages[];
    ArrayResize(rawPriceAverages, barsBack);
    long rawVolumes[];
    ArrayResize(rawVolumes, barsBack);
    double inputs[];
    ArrayResize(inputs, numInputs);
    int outputids[];
    ArrayResize(outputids, 1);
    double longoutput[];
    ArrayResize(longoutput, 1);
    double shortoutput[];
    ArrayResize(shortoutput, 1);
    for(int i=0; i<barsBack; i++) {
        rawPriceHighs[i] = iHigh(symbol, 0, i+1);
        rawPriceLows[i] = iLow(symbol, 0, i+1);
        rawPriceAverages[i] = (rawPriceHighs[i] + rawPriceLows[i])/2;
        rawPriceOpens[i] = iOpen(symbol, 0, i+1);
        rawPriceCloses[i] = iClose(symbol, 0, i+1);
        rawVolumes[i] = iVolume(symbol, 0, i+1);
    }

    int semilocal = 10*barsBack;
    double maxPriceLocal = iHigh(symbol, 0, iHighest(symbol, 0, MODE_HIGH, barsBack, 1));
    double maxPriceSemiLocal = iHigh(symbol, 0, iHighest(symbol, 0, MODE_HIGH, semilocal, 1));
    double maxPriceGlobal = iHigh(symbol, 0, iHighest(symbol, 0, MODE_HIGH, WHOLE_ARRAY, 1));
    double minPriceLocal = iLow(symbol, 0, iLowest(symbol, 0, MODE_LOW, barsBack, 1));
    double minPriceSemiLocal = iLow(symbol, 0, iLowest(symbol, 0, MODE_LOW, semilocal, 1));
    double minPriceGlobal = iLow(symbol, 0, iLowest(symbol, 0, MODE_LOW, WHOLE_ARRAY, 1));
    long maxVolumeLocal = iVolume(symbol, 0, iHighest(symbol, 0, MODE_VOLUME, barsBack, 1));
    long maxVolumeSemiLocal = iVolume(symbol, 0, iHighest(symbol, 0, MODE_VOLUME, semilocal, 1));
    long maxVolumeGlobal = iVolume(symbol, 0, iHighest(symbol, 0, MODE_VOLUME, WHOLE_ARRAY, 1));
    long minVolumeLocal = iVolume(symbol, 0, iLowest(symbol, 0, MODE_VOLUME, barsBack, 1));
    long minVolumeSemiLocal = iVolume(symbol, 0, iLowest(symbol, 0, MODE_VOLUME, semilocal, 1));
    long minVolumeGlobal = iVolume(symbol, 0, iLowest(symbol, 0, MODE_VOLUME, WHOLE_ARRAY, 1));
    
    if(maxPriceSemiLocal-minPriceSemiLocal > 0)
        inputs[0] = (((maxPriceLocal+minPriceLocal)/2)-minPriceSemiLocal)/(maxPriceSemiLocal-minPriceSemiLocal);
    else
        inputs[0] = 1;

    if(maxPriceSemiLocal-minPriceSemiLocal > 0)
        inputs[1] = (maxPriceLocal-minPriceLocal)/(maxPriceSemiLocal-minPriceSemiLocal);
    else
        inputs[1] = 1;

    /*
    if(maxVolumeSemiLocal > 0)
        inputs[2] = ((double)maxVolumeLocal)/maxVolumeSemiLocal;
    else
        inputs[2] = 1;
    */

    for(int i=2; i<barsBack+2; i++) {
        if(maxPriceLocal == minPriceLocal)
            inputs[i] = 0;
        else
            inputs[i] = 2*(rawPriceAverages[i-2]-minPriceLocal)/(maxPriceLocal-minPriceLocal)-1;
    }
    /*
    for(int i=barsBack+3; i<2*barsBack+3; i++) {
        if(maxVolumeLocal == minVolumeLocal)
            inputs[i] = 0.5;
        else
            inputs[i] = ((double)(rawVolumes[i-barsBack-3]))/(maxVolumeLocal);
    }
    for(int i=2*barsBack+3; i<3*barsBack+2; i++) {
        if(rawPriceAverages[i-2*barsBack-3] > rawPriceHighs[i-2*barsBack-2])
            inputs[i] = 1;
        else if(rawPriceAverages[i-2*barsBack-3] < rawPriceLows[i-2*barsBack-2])
            inputs[i] = -1;
        else
            inputs[i] = 0;
    }
    for(int i=3*barsBack+2; i<4*barsBack+1; i++) {
        if(rawPriceAverages[i-3*barsBack-2] > rawPriceAverages[i-3*barsBack-1])
            inputs[i] = 1;
        else if(rawPriceAverages[i-3*barsBack-2] < rawPriceAverages[i-3*barsBack-1])
            inputs[i] = -1;
        else
            inputs[i] = 0;
    }
    */
    
    /* ---- done setting up inputs ---- */

    if(train && OrdersTotal() == 0) {
        if(pendingLearn && lastTradeTicket >= 0) {
            if(OrderSelect(lastTradeTicket, SELECT_BY_TICKET)) {
                //Print("OrderProfit: ", OrderProfit()/Point);
                double learnVal = (OrderProfit()-OrderCommission())/Point/learnDivisor/100000;
                Print("learnVal: ", learnVal);
                if(OrderType() == OP_BUY) {
                    if(learnVal > 0) {
                        if(useTilt && GlobalVariableGet("longYes") != 0 && GlobalVariableGet("longNo") != 0)
                            sendLearnToPipe(learnVal*(GlobalVariableGet("longYes")+GlobalVariableGet("longNo"))/GlobalVariableGet("longYes"), true);
                        else
                            sendLearnToPipe(learnVal, true);
                        longYesTotal += learnVal;
                    }
                    else {
                        if(useTilt && GlobalVariableGet("longYes") != 0 && GlobalVariableGet("longNo") != 0)
                            sendLearnToPipe(learnVal*(GlobalVariableGet("longYes")+GlobalVariableGet("longNo"))/GlobalVariableGet("longNo"), true);
                        else
                            sendLearnToPipe(learnVal, true);
                        longNoTotal -= learnVal;
                    }
                }
                else if(OrderType() == OP_SELL) {
                    if(learnVal > 0) {
                        if(useTilt && GlobalVariableGet("shortYes") != 0 && GlobalVariableGet("shortNo") != 0)
                            sendLearnToPipe(learnVal*(GlobalVariableGet("shortYes")+GlobalVariableGet("shortNo"))/GlobalVariableGet("shortYes"), false);
                        else
                            sendLearnToPipe(learnVal, false);
                        shortYesTotal += learnVal;
                    }
                    else {
                        if(useTilt && GlobalVariableGet("shortYes") != 0 && GlobalVariableGet("shortNo") != 0)
                            sendLearnToPipe(learnVal*(GlobalVariableGet("shortYes")+GlobalVariableGet("shortNo"))/GlobalVariableGet("shortNo"), false);
                        else
                            sendLearnToPipe(learnVal, false);
                        shortNoTotal -= learnVal;
                    }
                }
            }
            else {
                Print("OrderSelect failed!");
            }
        }
        pendingLearn = false;

        sendInputsToPipe(inputs);
        readStringFromPipe();   //we don't actually have to do anything with these results
        //Print("long: ", longoutput[0], " short: ", shortoutput[0]);

        if(MathRand()%2==0) {
            makeTrade(OP_BUY, 0);
        }
        else {
            makeTrade(OP_SELL, 0);
        }
    }
    else {
        sendInputsToPipe(inputs);
        getOutputs(outputids, longoutput, shortoutput);
        for(int i=0; i<ArraySize(outputids); i++) {
            //Print("outputid: ", outputids[i], " long: ", longoutput[i], " short: ", shortoutput[i]);
            if(longoutput[i] > 0 && shortoutput[i] < 0) {
                makeTrade(OP_BUY, outputids[i]);
            }
            else if(longoutput[i] < 0 && shortoutput[i] > 0) {
                makeTrade(OP_SELL, outputids[i]);
            }
        }
    }
}

double OnTester() {
    if(train) {
        Print("prevLongYes: ", GlobalVariableGet("longYes"), " prevLongNo: ", GlobalVariableGet("longNo"), " prevShortYes: ", GlobalVariableGet("shortYes"), " prevShortNo: ", GlobalVariableGet("shortNo"));
        Print("longYes: ", longYesTotal, " longNo: ", longNoTotal, " shortYes: ", shortYesTotal, " shortNo: ", shortNoTotal);
        if(longYesTotal != 0 && longNoTotal != 0) {
            GlobalVariableSet("longYes", longYesTotal);
            GlobalVariableSet("longNo", longNoTotal);
        }
        if(shortYesTotal != 0 && shortNoTotal != 0) {
            GlobalVariableSet("shortYes", shortYesTotal);
            GlobalVariableSet("shortNo", shortNoTotal);
        }
            
    }
    return 1;
}
