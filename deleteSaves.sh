source path.cfg
genstockBotDataPath="$datapath"/tester/files/GenStockBotData

rm "$genstockBotDataPath"/*.txt
rm -r "$genstockBotDataPath"/*/
rm "$genstockBotDataPath"/../../../genstockReports/train/*
rm "$genstockBotDataPath"/../../../genstockReports/test/*
rm "$genstockBotDataPath"/../../../genstockReports/test2014/*
rm "$genstockBotDataPath"/../../../genstockReports/*
rm "$genstockBotDataPath"/../genstockReports/testResults/*
rm -r savegenbot/*
