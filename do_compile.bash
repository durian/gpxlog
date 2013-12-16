make clean
./qmake gpxlog.pro PRIVATENAMESPACE=GPXLOG
make

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    echo "cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl"
    cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl 
elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Macintosh"
elif [[ "$OSTYPE" == "win32" ]]; then
        echo "Windows"
else
        echo "Unknown"
fi