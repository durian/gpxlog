make clean
./qmake gpxlog.pro PRIVATENAMESPACE=GPXLOG
make

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo "cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl"
    mkdir -p /x/games/X-Plane10/Resources/plugins/gpxlog/64
    cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl 
    make clean
    ./qmake -spec linux-g++-32 gpxlog.pro PRIVATENAMESPACE=GPXLOG
    make
    mkdir -p /x/games/X-Plane10/Resources/plugins/gpxlog/32
    cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/32/lin.xpl

elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "cp libmac.xpl.dylib /Users/pberck/priv/X-Plane\ 10/Resources/plugins/GPXlog/mac.xpl"
	cp libmac.xpl.dylib /Users/pberck/priv/X-Plane\ 10/Resources/plugins/GPXlog/mac.xpl
elif [[ "$OSTYPE" == "win32" ]]; then
        echo "Windows"
else
        echo "Unknown"
fi
