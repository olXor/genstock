#!/bin/bash

termpath="/cygdrive/c/Program\ Files\ \(x86\)/MetaTrader\ 4/terminal.exe"
abstermpath="C:/Program Files (x86)/MetaTrader 4/terminal.exe"
datapath="/cygdrive/c/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702"
absdatapath="C:/Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702"
OPTIND=1
startnum=1
maketests=1
sparsetests=0
learnDivisor=100
timePeriod=1
rYear=5

while getopts "rtl:sp:" opt; do
    case "$opt" in
        r) 
            rm trainingResults.txt
            ./deleteSaves.sh
            #echo "TestReturn TestSharpe (TestTrades TestPercent) TrainReturn TrainSharpe (TrainTrades TrainPercent)" >> trainingResults.txt
            ;;
        t)
            maketests=0
            sparsetests=0
            ;;
        l)
            learnDivisor=$OPTARG
            ;;
        s)
            maketests=0
            sparsetests=1
            ;;
        p)
            timePeriod=$OPTARG
#           0 = 2005-2014 
#           1 = 2015
#           2 = 2014
#           3 = random year from 2005-2014
#           4 = random year from 2005-2015
            ;;
    esac
done

shift $((OPTIND-1))

for i in `seq 1 "$1"`;
do
    if [ -d "$datapath"/tester/files/GenStockBotData/"$i"/ ]; then
        startnum=$[ $i+1 ]
    else
        break
    fi
done

for i in `seq "$startnum" "$1"`;
do
    REM=$[ $i % 10 ]
    if [ $REM -eq "0" ]; then
        echo $i
    fi

    if [ $timePeriod -eq "3" ]; then
        rYear=$(( ( RANDOM % 10 ) + 5 ))
    elif [ $timePeriod -eq "4" ]; then
        rYear=$(( ( RANDOM % 11 ) + 5 ))
    fi

    ./parSave.sh -s "train/$i" -p "$timePeriod" -y "$rYear" -t 2 -rl "$learnDivisor" "$datapath" "$absdatapath" genstock
    #cmd /c start /min /wait "" "$abstermpath" genstock.txt
    wscript.exe nofocustest.vbs
    if [[ $maketests -eq "1" || ($sparsetests -eq "1" && $(( $i % 10 )) -eq "0") ]]; then
        ./parSave.sh -s "test/$i"t -p "$timePeriod" -y "$rYear" -t 1 -l "$learnDivisor" "$datapath" "$absdatapath" genstock
        #cmd /c start /min /wait "" "$abstermpath" genstock.txt
        wscript.exe nofocustest.vbs
        ./parSave.sh -s "test/$i"r -p "$timePeriod" -y "$rYear" -t 2 -l "$learnDivisor" "$datapath" "$absdatapath" genstock
        #cmd /c start /min /wait "" "$abstermpath" genstock.txt
        wscript.exe nofocustest.vbs
        ./parSave.sh -s "test/$i"p -p 1 -t 0 -l "$learnDivisor" "$datapath" "$absdatapath" genstock
        #cmd /c start /min /wait "" "$abstermpath" genstock.txt
        wscript.exe nofocustest.vbs
    fi
    mkdir "$datapath"/tester/files/GenStockBotData/"$i"
    cp "$datapath"/tester/files/GenStockBotData/*.txt "$datapath"/tester/files/GenStockBotData/"$i/"
done

