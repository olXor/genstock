#include "genstock.h"
#include <iostream>
#include <windows.h>
#include <string>
//#include "nvwa/debug_new.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include </usr/include/fenv.h>
#pragma STDC FENV_ACCESS ON
#include <pthread.h>

#include <float.h>
#include <unistd.h>

//only for the test main function
#define PARENTLAYERS 2
#define CHILDLAYERS 2
#define PARENTNODESPERLAYER 5
#define CHILDNODESPERLAYER 5
//------

WINDOW* mainwin;
WINDOW* networkwin;
WINDOW* errorwin;
WINDOW* statuswin;

EventLog* eventLog = new EventLog();
EventLog* networkLog = new EventLog();
EventLog* errorLog = new EventLog();
EventLog* statusLog = new EventLog();

const char* datapath;
const char* absdatapath;


const char* savestring = "savegenbot/";

#define RESULT_TYPE_TRAIN 0
#define RESULT_TYPE_TEST 1
#define RESULT_TYPE_SECONDARY_TEST 2

bool stopAfterNextMT4Run = false;
bool stopAfterNextTestSet = false;
bool pauseAfterNextMT4Run = false;

int roundNum = 0;
int currentTrainRun = 0;
int currentTestRun = 0;

void readCfg() {
    std::ifstream pathfile("path.cfg");
    std::string line;
    while(std::getline(pathfile, line)) {
        std::string variable;
        std::string value;
        int del;
        if((del = line.find("="))==std::string::npos)
            throw std::runtime_error("invalid path.cfg");
        variable = line.substr(0, del);
        value = line.substr(del+1, std::string::npos);
        if(!value.empty() && value.front()=='\"' && value.back()=='\"')
            value = value.substr(1,value.length()-2);

        if(variable == "datapath")
            datapath = value.c_str();
        else if(variable == "absdatapath")
            absdatapath = value.c_str();
    }
}

void deleteResults(std::string name) {
    std::ostringstream fname;
    fname << absdatapath << name << ".htm";
    remove(fname.str().c_str());
    fname.clear();
    fname.str("");
    fname << absdatapath << name << ".gif";
    remove(fname.str().c_str());
}

double checkResultsOld(int id, int resultType) {
    std::ostringstream fname;
    if(resultType == RESULT_TYPE_TEST)
        fname << absdatapath << id << "t.htm";
    else if(resultType == RESULT_TYPE_TRAIN)
        fname << absdatapath << id << "r.htm";
    else if(resultType == RESULT_TYPE_SECONDARY_TEST)
        fname << absdatapath << id << "t2.htm";
    else
        return -999999.;

    std::ifstream infile(fname.str().c_str());
    std::string line;
    bool profitfound = false;
    double profit = 0;
    std::string profstring;

    while(std::getline(infile, line)) {
        if(line.find("Total net profit") != std::string::npos) {
            profitfound = true;
            profstring = line.substr(56, line.find("<", 56)-56);
            profit = strtod(profstring.c_str(), NULL);
        }
    }

    if(!profitfound) {
        profit = -99999;
        std::ostringstream oss;
        oss << "Couldn't parse result file for bot id " << id;
        errorLog->addEvent(oss.str().c_str(), errorwin);
    }

    return profit;
}

double checkResults(int id, int resultType) {
    std::ostringstream fname;
    fname << absdatapath << "../tester/files/genstockReports/testResults/";
    if(resultType == RESULT_TYPE_TEST) {
        fname << "t-" << id;
    }
    else if(resultType == RESULT_TYPE_TRAIN) {
        fname << "r-" << id;
    }
    else if(resultType == RESULT_TYPE_SECONDARY_TEST) {
        fname << "t2-" << id;
    }
    std::ifstream infile(fname.str().c_str());
    double profit = 0;
    std::string line;

    while(std::getline(infile, line)) {
        line = line.substr(0, line.find(" "));
        std::istringstream ss(line);
        double nextprofit;
        ss >> nextprofit;
        profit += nextprofit;
    }

    return profit;
}

void printResults(const char* rstring) {
    std::ofstream outfile("savegenbot/results", std::ios::app);
    outfile << rstring << std::endl;
    outfile.close();
}

void addHistoryEntry(int id, const char* entry) {
    std::ostringstream fname;
    fname << "savegenbot/" << id << "/history";
    std::ofstream outfile(fname.str().c_str(), std::ios::app);
    outfile << entry << std::endl;
    outfile.close();
}

void saveStartInfo() {
    std::ofstream outfile("savegenbot/startinfo");
    outfile << "roundNum " << roundNum << std::endl;
    outfile << "currentTrainRun " << currentTrainRun << std::endl;
    outfile << "currentTestRun " << currentTestRun << std::endl;
    outfile.close();
}

void loadStartInfo() {
    roundNum = 1;
    currentTrainRun = 1;
    currentTestRun = 1;
    std::ifstream infile("savegenbot/startinfo");
    std::string line;
    while(std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string token1;
        int token2;
        if(!(iss>>token1>>token2)) continue;

        if(token1=="roundNum")
            roundNum = token2;
        if(token1=="currentTrainRun")
            currentTrainRun = token2;
        if(token1=="currentTestRun")
            currentTestRun = token2;
    }
    infile.close();
}

void swap(Genbot** genbots, double** profits, int i, int j, int numArrays) {
    Genbot* gentmp;
    double proftmp;
    gentmp = genbots[i];
    genbots[i] = genbots[j];
    genbots[j] = gentmp;
    for(int k=0; k<numArrays; k++) {
        proftmp = profits[k][i];
        profits[k][i] = profits[k][j];
        profits[k][j] = proftmp;
    }
}

void sortByProfit(Genbot** genbots, double** profits, int numArrays) {
    for(int i=1; i<NUMGENBOTS; i++) {
        for(int j=i-1; j>=0; j--) {
            if(profits[0][j+1] > profits[0][j])
                swap(genbots, profits, j+1, j, numArrays);
            else
                break;
        }
    }
}

void saveCurrentIDs(Genbot** genbots) {
    std::ofstream file("savegenbot/currentbots");
    for(int i=0; i<NUMGENBOTS; i++) {
        file << genbots[i]->getID() << std::endl;
    }
}

int findNextID(int id) {
    struct stat info;
    std::ostringstream fname;
    fname << "savegenbot/" << id;
    while(stat(fname.str().c_str(), &info) == 0) {
        id++;
        fname.clear();
        fname.str("");
        fname << "savegenbot/" << id;
    }

    return id;
}

bool loadCurrentGenbots(Genbot** genbots) {
    std::ifstream file("savegenbot/currentbots");
    if(file.is_open()) {
        std::string line;
        for(int i=0; i < NUMGENBOTS && getline(file, line); i++) {
            int id;
            if(!(std::istringstream(line) >> id))
                throw std::runtime_error("found something that wasn't an id in the current Genbots file");
            
            std::ostringstream genomefname;
            genomefname << savestring << id << "/genome";

            Genome* genome = new Genome(defaultConvProp);
            genome->loadGenome(genomefname.str().c_str());
            genbots[i] = new Genbot(genome, NUMINPUTS, NUMOUTPUTS, id);

            std::ostringstream fname;
            fname << savestring << id << "/bot";
            genbots[i]->loadBot(fname.str().c_str());
            genbots[i]->progressTurnsSaved();
        }
    }
    else {
        //create the first set of bots
        for(int i=0; i<NUMGENBOTS; i++) {
            Genome* genome = new Genome(defaultConvProp);
            genome->createRandomGenome();
            genbots[i] = new Genbot(genome, NUMINPUTS, NUMOUTPUTS, findNextID(1));

            std::ostringstream foldercall;
            foldercall << "mkdir " << savestring << genbots[i]->getID();
            system(foldercall.str().c_str());
            std::ostringstream fname;
            fname << savestring << genbots[i]->getID() << "/genome";
            genbots[i]->getGenome()->saveGenome(fname.str().c_str());

            fname.clear();
            fname.str("");
            fname << savestring << genbots[i]->getID() << "/start";
            genbots[i]->saveBot(fname.str().c_str());

            std::ostringstream entry;
            entry << "Round " << roundNum << ": Born in initial batch";
            addHistoryEntry(genbots[i]->getID(), entry.str().c_str());

            genbots[i]->progressTurnsSaved();
        }
    }
}

void *startKeyboardCheck(void *threadarg) {
    stopAfterNextMT4Run = false;
    stopAfterNextTestSet = false;
    statusLog->addEvent("Running.", statuswin);
    while(true) {
        int ch;
        while((ch = getch()) != ERR) {
            switch(ch) {
                case 'q':
                    pauseAfterNextMT4Run = false;
                    stopAfterNextMT4Run = true;
                    stopAfterNextTestSet = true;
                    statusLog->addEvent("Stopping immediately", statuswin);
                    break;
                case 's':
                    pauseAfterNextMT4Run = false;
                    stopAfterNextMT4Run = false;
                    stopAfterNextTestSet = true;
                    statusLog->addEvent("Stopping at the end of this round", statuswin);
                    break;
                case 'g':
                    pauseAfterNextMT4Run = false;
                    stopAfterNextMT4Run = false;
                    stopAfterNextTestSet = false;
                    statusLog->addEvent("Running.", statuswin);
                    break;
                case 'p':
                    pauseAfterNextMT4Run = true;
                    stopAfterNextMT4Run = false;
                    stopAfterNextTestSet = false;
                    statusLog->addEvent("Paused.", statuswin);
            }
        }
        sleep(1);
    }
}

std::string processNumSeconds(double t) {
    int minutes;
    int seconds;
    minutes = t/60;
    seconds = ((int)t)%60;
    std::ostringstream oss;
    oss << minutes << " m, " << seconds << " s";
    return oss.str();
}

void combineParentInitialConditions(Genbot* child, Genbot* parent1, Genbot* parent2) {
    Genbot* originalParent1 = new Genbot(parent1->getGenome()->copy(), NUMINPUTS, NUMOUTPUTS, -1);
    Genbot* originalParent2 = new Genbot(parent2->getGenome()->copy(), NUMINPUTS, NUMOUTPUTS, -1);

    std::string fhandle;
    if(CHILD_INHERITS_PARENT_LEARNING)
        fhandle = "/bot";
    else
        fhandle = "/start";

    std::ostringstream fname;
    fname << savestring << parent1->getID() << fhandle;
    originalParent1->loadBot(fname.str().c_str());

    fname.clear();
    fname.str("");
    fname << savestring << parent2->getID() << fhandle;
    originalParent2->loadBot(fname.str().c_str());

    if(rand()%2) {
        child->copyWeights(originalParent1, 1);
        child->copyWeights(originalParent2, 0.5);
    }
    else {
        child->copyWeights(originalParent2, 1);
        child->copyWeights(originalParent1, 0.5);
    }
    child->mutateWeights();

    delete originalParent1;
    delete originalParent2;
}

int main() {
    feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
    srand(time(NULL));

    //ncurses stuff
    initscr();
    raw();
    keypad(stdscr, TRUE);
    //cbreak();
    curs_set(0);
    noecho();
    nodelay(stdscr, TRUE);

    refresh();

    statuswin = newwin(2, 160, 0, 0);
    mainwin = newwin(50, 80, 2, 0);
    networkwin = newwin(50, 80, 2, 80);
    errorwin = newwin(10, 160, 52, 0);

    readCfg();

    MT4PipeGen* mt4pipe = new MT4PipeGen(NUMINPUTS, NUMOUTPUTS, NUMGENBOTS, NUMSTATICTOPBOTS, NUM_TRAIN_THREADS);

    pthread_t statusthread;
    pthread_create(&statusthread, NULL, &startKeyboardCheck, NULL);

    Genbot* genbots[NUMGENBOTS];

    std::ostringstream fname;

    loadCurrentGenbots(genbots);
    loadStartInfo();

    //save initial ids and networks
    saveCurrentIDs(genbots);
    for(int i=0; i<NUMGENBOTS; i++) {
        fname.clear();
        fname.str("");
        fname << savestring << genbots[i]->getID() << "/bot";
        genbots[i]->saveBot(fname.str().c_str());
    }

    bool done = false;
    while(!done) {
        std::ostringstream oss;
        oss << "Starting round " << roundNum;
        eventLog->addEvent(oss.str().c_str(), mainwin);
        if(!(roundNum == 1 && SKIP_TRAIN_ON_ROUND_1)) {
            for(int i=0; i<NUMTRAINCYCLES; i++) {
                if(i+1 < currentTrainRun) continue;
                char* buf;
                asprintf(&buf, "starting train %d", i+1);
                networkLog->addEvent(buf, networkwin);
                time_t starttime = time(NULL);
                mt4pipe->runSim(genbots, 
                        -1,         //botnum (-1 means train all bots)
                        BARSBACK,   //barsBack
                        TRADECANDLELIMIT,   //tradeCandleLimit
                        LEARNDIVISOR,       //learnDivisor
                        true,       //train
                        2,          //testSample
                        TRAIN_TIMEPERIOD,          //timePeriod
                        TESTPERIOD, //testPeriod
                        USE_TRAILING_STOP,    //useTrailingStop
                        true,       //useTilt
                        NULL        //savename
                        );

                for(int j=0; j<NUMGENBOTS; j++) {
                    fname.clear();
                    fname.str("");
                    fname << savestring << genbots[j]->getID() << "/bot";
                    genbots[j]->saveBot(fname.str().c_str());
                }
                currentTrainRun++;
                saveStartInfo();
                time_t endtime = time(NULL);
                double secsElapsed = difftime(endtime, starttime);
                std::ostringstream ost;
                ost << "Done. (" << processNumSeconds(secsElapsed) << ")";
                networkLog->addEvent(ost.str().c_str(), networkwin);
                while(pauseAfterNextMT4Run) sleep(5);
                if(stopAfterNextMT4Run) return 1;
            }
        }

        if(!(roundNum == 1 && SKIP_TEST_ON_ROUND_1)) {
            deleteResults("t-");
            deleteResults("r-");
            deleteResults("t2-");
            if(1 == currentTestRun) {
                char* buf;
                asprintf(&buf, "starting test t");
                networkLog->addEvent(buf, networkwin);
                time_t starttime = time(NULL);

                mt4pipe->runSim(genbots,
                        -1,                  //botnum
                        BARSBACK,           //barsBack
                        TRADECANDLELIMIT,   //tradeCandleLimit
                        LEARNDIVISOR,       //learnDivisor
                        false,              //train
                        1,                  //testSample
                        TEST1_TIMEPERIOD,   //timePeriod
                        TESTPERIOD,         //testPeriod
                        USE_TRAILING_STOP,  //useTrailingStop
                        true,               //useTilt
                        "t-",               //savename
                        SEPARATE_BOT);      //outputType

                time_t endtime = time(NULL);
                double secsElapsed = difftime(endtime, starttime);
                std::ostringstream ost;
                ost << "Done. (" << processNumSeconds(secsElapsed) << ")";
                networkLog->addEvent(ost.str().c_str(), networkwin);

                currentTestRun++;
                saveStartInfo();
                while(pauseAfterNextMT4Run) sleep(5);
                if(stopAfterNextMT4Run) return 1;
            }
            
            if(2 == currentTestRun) {
                char* buf;
                asprintf(&buf, "starting test r");
                networkLog->addEvent(buf, networkwin);

                time_t starttime = time(NULL);

                mt4pipe->runSim(genbots,
                        -1,                  //botnum
                        BARSBACK,           //barsBack
                        TRADECANDLELIMIT,   //tradeCandleLimit
                        LEARNDIVISOR,       //learnDivisor
                        false,              //train
                        2,                  //testSample
                        TRAIN_TIMEPERIOD,   //timePeriod
                        TESTPERIOD,         //testPeriod
                        USE_TRAILING_STOP,  //useTrailingStop
                        true,               //useTilt
                        "r-",               //savename
                        SEPARATE_BOT);      //outputType
                time_t endtime = time(NULL);
                double secsElapsed = difftime(endtime, starttime);
                std::ostringstream ost;
                ost << "Done. (" << processNumSeconds(secsElapsed) << ")";
                networkLog->addEvent(ost.str().c_str(), networkwin);

                currentTestRun++;
                saveStartInfo();
                while(pauseAfterNextMT4Run) sleep(5);
                if(stopAfterNextMT4Run) return 1;
            }

            if(3 == currentTestRun) {
                char* buf;
                asprintf(&buf, "starting test t2");
                networkLog->addEvent(buf, networkwin);

                time_t starttime = time(NULL);

                mt4pipe->runSim(genbots,
                        -1,                  //botnum
                        BARSBACK,           //barsBack
                        TRADECANDLELIMIT,   //tradeCandleLimit
                        LEARNDIVISOR,       //learnDivisor
                        false,              //train
                        TEST2_TESTSAMPLE,   //testSample
                        TEST2_TIMEPERIOD,   //timePeriod
                        TESTPERIOD,         //testPeriod
                        USE_TRAILING_STOP,  //useTrailingStop
                        true,               //useTilt
                        "t2-",              //savename
                        SEPARATE_BOT);      //outputType
                time_t endtime = time(NULL);
                double secsElapsed = difftime(endtime, starttime);
                std::ostringstream ost;
                ost << "Done. (" << processNumSeconds(secsElapsed) << ")";
                networkLog->addEvent(ost.str().c_str(), networkwin);

                currentTestRun++;
                saveStartInfo();
                while(pauseAfterNextMT4Run) sleep(5);
                if(stopAfterNextMT4Run) return 1;
            }
        }
        system("wait");
        system("ps | grep wscript.exe | awk '{print $1}' | wait");
        sleep(5);
        double testprofits[NUMGENBOTS];
        double trainprofits[NUMGENBOTS];
        double test2profits[NUMGENBOTS];
        for(int i=0; i<NUMGENBOTS; i++) {
            testprofits[i] = checkResults(genbots[i]->getID(), RESULT_TYPE_TEST);
            trainprofits[i] = checkResults(genbots[i]->getID(), RESULT_TYPE_TRAIN);
            test2profits[i] = checkResults(genbots[i]->getID(), RESULT_TYPE_SECONDARY_TEST);
        }
        double* profits[4];
        profits[0] = new double[NUMGENBOTS];
        profits[1] = testprofits;
        profits[2] = trainprofits;
        profits[3] = test2profits;

        for(int i=0; i<NUMGENBOTS; i++) {
            profits[0][i] = CHECK_TEST*testprofits[i] + CHECK_TRAIN*trainprofits[i];
        }
        sortByProfit(genbots, profits, 4);

        /*
        //-----Now do post-ranking tests
        std::ostringstream secoss;
        secoss << "Doing post-ranking tests";
        networkLog->addEvent(secoss.str().c_str(), networkwin);
        time_t starttime = time(NULL);
        //only test the static bots on the secondary test sample
        for(int i=0; i<NUMSTATICTOPBOTS; i++) {
            fname.clear();
            fname.str("");
            fname << genbots[i]->getID() << "t2";
            mt4pipe->runSim(genbots,
                    i,                  //botnum
                    BARSBACK,           //barsBack
                    TRADECANDLELIMIT,   //tradeCandleLimit
                    LEARNDIVISOR,       //learnDivisor
                    false,              //train
                    TEST2_TESTSAMPLE,   //testSample
                    TEST2_TIMEPERIOD,   //timePeriod
                    TESTPERIOD,         //testPeriod
                    USE_TRAILING_STOP,  //useTrailingStop
                    true,               //useTilt
                    fname.str().c_str());
            while(pauseAfterNextMT4Run) sleep(5);
        }

        system("wait");
        system("ps | grep wscript.exe | awk '{print $1}' | wait");
        sleep(5);

        double test2profits[NUMSTATICTOPBOTS];
        for(int i=0; i<NUMSTATICTOPBOTS; i++) {
            test2profits[i] = checkResults(genbots[i]->getID(), RESULT_TYPE_SECONDARY_TEST);
        }
        time_t endtime = time(NULL);
        double secsElapsed = difftime(endtime, starttime);
        std::ostringstream ost;
        ost << "Done. (" << processNumSeconds(secsElapsed) << ")";
        networkLog->addEvent(ost.str().c_str(), networkwin);
        //-----------------
          */

        for(int i=0; i<NUMGENBOTS; i++) {
            std::ostringstream entry;
            entry << "Round " << roundNum << ": rank " << i+1 << " of " << NUMGENBOTS << " with test=" << testprofits[i] << ", train=" << trainprofits[i] << ", test2=" << test2profits[i];
            if(i < NUMSTATICTOPBOTS)
                entry << " (static)";
            if(i >= NUMWINBOTS)
                entry << " (eliminated)";
            addHistoryEntry(genbots[i]->getID(), entry.str().c_str());
        }

        std::ostringstream pss;
        pss << "Profits for round " << roundNum << ": ";
        for(int i=0; i<NUMGENBOTS; i++) {
            pss << genbots[i]->getID() << "=" << testprofits[i] << "(" << trainprofits[i] << "," << test2profits[i] << ")";
            if(i != NUMGENBOTS-1)
                pss << ", ";
        }
        eventLog->addEvent(pss.str().c_str(), mainwin);
        printResults(pss.str().c_str());
        pss.clear();
        pss.str("");
        pss << "Winning bots: ";
        for(int i=0; i<NUMWINBOTS; i++) {
            pss << genbots[i]->getID();
            if(i != NUMWINBOTS-1)
                pss << ", ";
        }
        eventLog->addEvent(pss.str().c_str(), mainwin);
        //replace losing genbots
        for(int i=NUMWINBOTS; i<NUMGENBOTS; i++) {
            delete genbots[i];

            int parent1 = rand() % NUMPARENTBOTS;
            int parent2 = rand() % NUMPARENTBOTS;
            Genome* genome = genbots[parent1]->getGenome()->mate(genbots[parent2]->getGenome());
            genbots[i] = new Genbot(genome, NUMINPUTS, NUMOUTPUTS, findNextID(1));
            combineParentInitialConditions(genbots[i], genbots[parent1], genbots[parent2]);

            std::ostringstream foldercall;
            foldercall << "mkdir " << savestring << genbots[i]->getID();
            system(foldercall.str().c_str());

            fname.clear();
            fname.str("");
            fname << savestring << genbots[i]->getID() << "/genome";
            genbots[i]->getGenome()->saveGenome(fname.str().c_str());

            fname.clear();
            fname.str("");
            fname << savestring << genbots[i]->getID() << "/start";
            genbots[i]->saveBot(fname.str().c_str());

            std::ostringstream entry;
            entry << "Round " << roundNum << ": Born to parents " << genbots[parent1]->getID() << " and " << genbots[parent2]->getID();
            addHistoryEntry(genbots[i]->getID(), entry.str().c_str());

            genbots[i]->progressTurnsSaved();
        }

        saveCurrentIDs(genbots);
        for(int i=0; i<NUMGENBOTS; i++) {
            fname.clear();
            fname.str("");
            fname << savestring << genbots[i]->getID() << "/bot";
            genbots[i]->saveBot(fname.str().c_str());
        }

        char* buf;
        asprintf(&buf, "Losing bots replaced.");
        eventLog->addEvent(buf, mainwin);

        roundNum++;
        currentTrainRun = 1;
        currentTestRun = 1;
        saveStartInfo();
        if(stopAfterNextTestSet) break;
        while(pauseAfterNextMT4Run) sleep(5);
    }

    for(int i=0; i<NUMGENBOTS; i++) {
        delete genbots[i];
    }

    return 0;
}

/*
int main() {
    srand(time(NULL));

    ClusterParameters* pars = new ClusterParameters();
    pars->numInputs = 2;
    pars->numOutputs = 1;
    pars->numLayers = PARENTLAYERS;
    pars->nodesPerLayer = 5;
    pars->randomWeights = 1;
    pars->numTurnsSaved = 15;
    pars->stepfactor = 1;
    pars->memfactor = 0.2;
    pars->memnorm = 0.98;
    pars->useBackWeights = 0;
    pars->backPropBackWeights = 0;
    pars->useBackMems = 0;
    pars->useForwardMems = 0;
    pars->propThresh = 0.0;
    pars->learnStyleSide = LEARNSTYLEALT;
    pars->tlevel = 0;
    pars->bpMemStrengths = false;
    pars->copyInputsToFirstLevel = false;
    pars->lockMaxMem = false;

    Genome* genome = new Genome();
    genome->pars[0] = pars;
    genome->pars[1] = pars->copy();

    Genbot* genbot = new Genbot(genome, 2, 1);
    saveBot(genbot, "testsave/test");
    Genbot* genbot2 = new Genbot(genome->copy(), 2, 1);
    loadBot(genbot2, "testsave/test");
    saveBot(genbot2, "testsave/testB");
}
*/

/*
int main(int argc, char *argv[]) {
    srand(time(NULL));

    initscr();
    //raw();
    curs_set(0);

    refresh();

    mainwin = newwin(40, 80, 0, 0);
    networkwin = newwin(60, 80, 0, 80);

    eventLog = new EventLog();

    ClusterParameters* pars = new ClusterParameters();
    pars->numInputs = 2;
    pars->numOutputs = 1;
    pars->numLayers = PARENTLAYERS;
    pars->nodesPerLayer = PARENTNODESPERLAYER;
    pars->randomWeights = 1;
    pars->numTurnsSaved = 15;
    pars->stepfactor = 1;
    pars->memfactor = 0.2;
    pars->memnorm = 0.98;
    pars->useBackWeights = 0;
    pars->backPropBackWeights = 0;
    pars->useBackMems = 0;
    pars->useForwardMems = 0;
    pars->propThresh = 0.0;
    pars->learnStyleSide = LEARNSTYLENONE;
    pars->tlevel = 0;
    pars->bpMemStrengths = false;
    pars->copyInputsToFirstLevel = false;
    pars->lockMaxMem = false;
    Cluster* cluster = new Cluster(pars);

    ClusterParameters* childPars;
    Cluster*** childClusters = new Cluster**[pars->numLayers];
    if(CHILDLAYERS > 0) {
        for(int i=0; i<pars->numLayers; i++) {
            childClusters[i] = new Cluster*[pars->nodesPerLayer];
            for(int j=0; j<pars->nodesPerLayer; j++) {
                childPars = new ClusterParameters();
                childPars->numInputs = cluster->getInputNumber(i, j, BLANKWEIGHT, 0);
                childPars->numOutputs = 1;
                childPars->numLayers = CHILDLAYERS;
                childPars->nodesPerLayer = CHILDNODESPERLAYER;
                childPars->randomWeights = 1;
                childPars->numTurnsSaved = 15;
                childPars->stepfactor = 1;
                childPars->memfactor = 0.2;
                childPars->memnorm = 0.98;
                childPars->useBackWeights = 0;
                childPars->backPropBackWeights = 0;
                childPars->useBackMems = 0;
                childPars->useForwardMems = 0;
                childPars->propThresh = 0.0;
                childPars->learnStyleSide = LEARNSTYLENONE;
                childPars->tlevel = 0;
                childPars->bpMemStrengths = false;
                childPars->copyInputsToFirstLevel = false;
                childPars->lockMaxMem = false;
                childClusters[i][j] = new Cluster(childPars);
                cluster->addCluster(i, j, childClusters[i][j]);
            }
        }
    }

    double inputs[2] = {0};
    double blankinputs[2] = {0};
    double outputs[1] = {0};
    char* buf;
    while(true) {
        inputs[0] = rand() % 2;
        inputs[1] = rand() % 2;

        for(int i=0; i<PARENTLAYERS*(CHILDLAYERS+1) + 1; i++) {
            if(i==0)
                cluster->setInputs(inputs);
            else
                cluster->setInputs(blankinputs);
            cluster->calculate();
        }
        cluster->getOutputs(outputs);
        if(outputs[0] > 0.5 == (((int)(inputs[0] + inputs[1])) % 2)) {
            asprintf(&buf, "CORRECT! inputs: %f %f, output: %f", inputs[0], inputs[1], outputs[0]);
            eventLog->addEvent(buf, mainwin);
            cluster->learn(1);
        }
        else {
            asprintf(&buf, "WRONG! inputs: %f %f, output: %f", inputs[0], inputs[1], outputs[0]);
            eventLog->addEvent(buf, mainwin);
            cluster->learn(-1);
        }
        werase(networkwin);
        cluster->printWeights(networkwin, 0);
        if(CHILDLAYERS > 0) {
            childClusters[0][0]->printWeights(networkwin, 0);
            if(pars->numLayers > 1)
                childClusters[1][0]->printWeights(networkwin, 0);
            if(pars->numLayers > 2)
                childClusters[2][0]->printWeights(networkwin, 0);
        }
        //wrefresh(mainwin);
        wrefresh(networkwin);
    }
}
*/
