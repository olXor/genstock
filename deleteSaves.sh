datapath="/cygdrive/c//Users/Thomas/AppData/Roaming/MetaQuotes/Terminal/50CA3DFB510CC5A8F28B48D1BF2A5702/tester/files/GenStockBotData"

rm "$datapath"/*.txt
rm -r "$datapath"/*/
rm "$datapath"/../../../genstockReports/train/*
rm "$datapath"/../../../genstockReports/test/*
rm "$datapath"/../../../genstockReports/test2014/*
rm "$datapath"/../../../genstockReports/*
rm "$datapath"/../genstockReports/testResults/*
rm -r savegenbot/*
