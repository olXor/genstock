#!/bin/bash

source path.cfg
OPTIND=1
testSample=0
rYear=5
learnDivisor=100
timePeriod=1
save=0
train=0
testPeriod=M15
barsBack=30
tradeCandleLimit=4
useTrailingStop=0
useTilt=0

while getopts "c:b:l:t:rp:s:T:Su" opt; do
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
        t)
            testSample=$OPTARG
            ;;
        r)
            train=1
            ;;
        p)
            timePeriod=$OPTARG
#           0 = 2005-2014 
#           1 = 2015
#           2 = 2014
#           3 = 2013
#           4 = random year from 2005-2014
#           5 = random year from 2005-2015
#           6 = 2011-2014
            ;;
        s)
            save=1
            savefile=$OPTARG
            ;;
        T)
            testPeriod=$OPTARG
            ;;
        S)
            useTrailingStop=1
            ;;
        u)
            useTilt=1
            ;;
    esac
done

shift $((OPTIND-1))

if [ $timePeriod -eq "4" ]; then
    rYear=$(( ( RANDOM % 10 ) + 5 ))
elif [ $timePeriod -eq "5" ]; then
    rYear=$(( ( RANDOM % 11 ) + 5 ))
fi

if [ $save -eq "1" ]; then
    savestring="-s $savefile"
fi

if [ $train -eq "1" ]; then
    trainstring="-r"
fi

if [ $useTrailingStop -eq "1" ]; then
    trailstring="-S"
fi

if [ $useTilt -eq "1" ]; then
    tiltstring="-u"
fi

./parSave.sh -p "$timePeriod" -y "$rYear" -t "$testSample" -l "$learnDivisor" -b "$barsBack" -c "$tradeCandleLimit" -T "$testPeriod" $savestring $trainstring $trailstring $tiltstring "$datapath" "$absdatapath" genstock

wscript.exe nofocustest.vbs
