make clean
./qmake gpxlog.pro PRIVATENAMESPACE=GPXLOG
make

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo "cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl"
    cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl 
elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "cp libmac.xpl.dylib /Users/pberck/priv/X-Plane\ 10/Resources/plugins/GPXlog/"
	cp libmac.xpl.dylib /Users/pberck/priv/X-Plane\ 10/Resources/plugins/GPXlog/
elif [[ "$OSTYPE" == "win32" ]]; then
        echo "Windows"
else
        echo "Unknown"
fi
