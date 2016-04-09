#include "testbots.h"

#define BARSBACK 100
#define TRADECANDLELIMIT 15
#define TESTPERIOD "M5"
#define NUMOUTPUTS 2
#define NUMINPUTS (4*BARSBACK+1)
#define USE_TRAILING_STOP 1
#define NUM_THREADS 4

const char* absdatapath;
const char* absdatapathtrunk;

double checkResultsOld(int id) {

    std::ostringstream fname;
    fname << absdatapath << "test2014/" << id << ".htm";

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
        std::cout << "Couldn't parse result file for bot #" << id << std::endl;
    }

    return profit;
}

double checkResults(int id) {
    std::ostringstream fname;
    fname << absdatapathtrunk << "/tester/files/genstockReports/testResults/test2014/all" << id;

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

int main() {
    absdatapath = "C:/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702/genstockReports/";
    absdatapathtrunk = "C:/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702/";
    std::vector<Genbot*> genbots;

    int id;
    int timePeriod;
    int testSample;
    std::cout << "Enter the id of the bot you want to test" << std::endl;
    std::cin >> id;

    std::cout << "Enter the time period you want to test (2015=1, 2014=2, 2005-2014=0)" << std::endl;
    std::cin >> timePeriod;

    std::cout << "Enter the sample num you want (test=1, train=2, all=0)" << std::endl;
    std::cin >> testSample;

    std::ostringstream savename;

    int testType = SEPARATE_BOT;

    int numGenbots = 1;
    int botnum = 0;

    if(id!=0) {
        std::ostringstream genomefname;
        genomefname << "savegenbot/" << id << "/genome";

        Genome* genome = new Genome();
        genome->loadGenome(genomefname.str().c_str());
        genbots.push_back(new Genbot(genome, NUMINPUTS, NUMOUTPUTS, id));

        std::ostringstream fname;
        fname << "savegenbot/" << id << "/bot";
        genbots[0]->loadBot(fname.str().c_str());
        genbots[0]->progressTurnsSaved();
        
        savename << id;
        testType = SINGLE_BOT;
        numGenbots = 1;
        botnum = 0;
    }
    else {
        std::ifstream file("savegenbot/currentbots");
        std::string line;
        for(int i=0; std::getline(file,line); i++) {
            if(!(std::istringstream(line) >> id))
                throw std::runtime_error("found something that wasn't an id in the current Genbots file");

            std::ostringstream genomefname;
            genomefname << "savegenbot/" << id << "/genome";

            Genome* genome = new Genome();
            genome->loadGenome(genomefname.str().c_str());
            genbots.push_back(new Genbot(genome, NUMINPUTS, NUMOUTPUTS, id));

            std::ostringstream fname;
            fname << "savegenbot/" << id << "/bot";
            genbots[i]->loadBot(fname.str().c_str());
            genbots[i]->progressTurnsSaved();

            numGenbots = i+1;
        }
        savename << "all";
        botnum = -1;
        testType = SEPARATE_BOT;
    }

    MT4PipeGen* mt4pipe = new MT4PipeGen(NUMINPUTS, NUMOUTPUTS, numGenbots, 0, NUM_THREADS);

    std::stringstream fname;
    fname << absdatapath << "test2014/" << savename.str().c_str() << ".htm";
    std::remove(fname.str().c_str());
    
    fname.clear();
    fname.str("");
    fname << absdatapath << "test2014/" << savename.str().c_str() << ".gif";
    std::remove(fname.str().c_str());

    fname.clear();
    fname.str("");
    fname << "test2014/" << savename.str().c_str();
    mt4pipe->runSim(&genbots[0],
            botnum,                  //botnum
            BARSBACK,           //barsBack
            TRADECANDLELIMIT,   //tradeCandleLimit
            500,       //learnDivisor
            false,              //train
            testSample,         //testSample
            timePeriod,         //timePeriod
            TESTPERIOD,         //testPeriod
            USE_TRAILING_STOP,  //useTrailingStop
            true,               //useTilt
            fname.str().c_str(),
            testType);

    system("wait");
    system("ps | grep wscript.exe | awk '{print $1}' | wait");
    sleep(5);

    if(testType == SINGLE_BOT) {
        std::cout << "Profit for bot #" << id << ": " << checkResultsOld(id) << std::endl;
    }
    else if(testType == SEPARATE_BOT) {
        for(int i=0; i<numGenbots; i++) {
            std::cout << "Profit for bot #" << genbots[i]->getID() << ": " << checkResults(genbots[i]->getID()) << std::endl;
        }
    }
}
