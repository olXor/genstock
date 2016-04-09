#!/bin/bash

OPTIND=1
learnDivisor=10000000
barsBack=30
train=0
save=0
sampleNum=0
timePeriod=1    #long
year=5
useTilt=0
testPeriod=M15
tradeCandleLimit=4
useTrailingStop=0

while getopts "l:b:c:rs:t:p:y:uT:S" opt; do
    case "$opt" in
        l)
            learnDivisor=$OPTARG
            ;;
        b)
            barsBack=$OPTARG
            ;;
        c)
            tradeCandleLimit=$OPTARG
            ;;
        r)
            train=1
            ;;
        s)
            save=1
            savefile=$OPTARG
            ;;
        t)
            sampleNum=$OPTARG
            ;;
        p)
            timePeriod=$OPTARG
#           0 = 2005-2014 
#           1 = 2015
#           2 = 2014
#           3 = 2013
#           4 = specific year (given by -y command)
#           5 = specific year (given by -y command)
#           6 = 2011-2013
            ;;
        y)
            year=$OPTARG
            ;;
        u)
            useTilt=1
            ;;
        T)
            testPeriod=$OPTARG
            ;;
        S)
            useTrailingStop=1
            ;;
    esac
done

shift $((OPTIND-1))

termfile="$1/genstock.txt"
abstermfile="$2/genstock.txt"
parfile="$1/tester/genstockPars.set"
absparfile="$2/tester/genstockPars.set"

echo "TestExpert=$3" > $termfile
echo "TestExpertParameters=genstockPars.set" >> $termfile
echo "TestPeriod=$testPeriod" >> $termfile
echo "TestSymbol=EURUSD" >> $termfile
echo "TestSpread=5" >> $termfile
echo "TestOptimization=false" >> $termfile
if [ $save -eq "1" ]; then
    echo "TestReport=genstockReports/$savefile" >> $termfile
fi
echo "TestReportReplace=false" >> $termfile
echo "TestOptimization=false" >> $termfile
echo "TestShutdownTerminal=true" >> $termfile
echo "TestDateEnable=true" >> $termfile
if [ $timePeriod -eq "0" ]; then
    echo "TestFromDate=2005.01.01" >> $termfile
    echo "TestToDate=2015.01.01" >> $termfile
elif [ $timePeriod -eq "1" ]; then
    echo "TestFromDate=2015.01.01" >> $termfile
    echo "TestToDate=2016.01.01" >> $termfile
elif [ $timePeriod -eq "2" ]; then
    echo "TestFromDate=2014.01.01" >> $termfile
    echo "TestToDate=2015.01.01" >> $termfile
elif [ $timePeriod -eq "3" ]; then
    echo "TestFromDate=2013.01.01" >> $termfile
    echo "TestToDate=2014.01.01" >> $termfile
elif [ $timePeriod -eq "4" ] || [ $timePeriod -eq "5" ]; then
    echo "TestFromDate=20$(printf "%0*d" 2 $year).01.01" >> $termfile
    echo "TestToDate=20$(printf "%0*d" 2 $(($year+1))).01.01" >> $termfile
elif [ $timePeriod -eq "6" ]; then
    echo "TestFromDate=2011.01.01" >> $termfile
    echo "TestToDate=2014.01.01" >> $termfile
fi

echo "learnDivisor=$learnDivisor" > $parfile
echo "barsBack=$barsBack" >> $parfile
echo "train=$train" >> $parfile
echo "sampleNum=$sampleNum" >> $parfile
echo "useTilt=$useTilt" >> $parfile
echo "tradeCandleLimit=$tradeCandleLimit" >> $parfile
echo "useTrailingStop=$useTrailingStop" >> $parfile
echo "savefile=$savefile" >> $parfile
