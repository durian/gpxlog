LINPATH="/x/games/X-Plane10/Resources/plugins"
MACPATH="/Users/pberck/priv/X-Plane\ 10/Resources/plugins"
WINPATH="HELP ME"
FOLDER="gpxlog"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
    mkdir -p ${LINPATH}/${FOLDER}/64
    
    make clean
    ./qmake gpxlog.pro 
    make

    echo "cp liblin.xpl.so /x/games/X-Plane10/Resources/plugins/gpxlog/64/lin.xpl"
    cp liblin.xpl.so ${LINPATH}/${FOLDER}/64/lin.xpl 

    make clean
    ./qmake -spec linux-g++-32 gpxlog.pro 
    make

    mkdir -p ${LINPATH}/${FOLDER}/32
    echo "cp liblin.xpl.so ${LINPATH}/${FOLDER}/32/lin.xpl"
    cp liblin.xpl.so ${LINPATH}/${FOLDER}/32/lin.xpl

elif [[ "$OSTYPE" == "darwin"* ]]; then
    mkdir -p ${MACPATH}/${FOLDER}

    make clean
    ./qmake gpxlog.pro 
    make

    echo "cp libmac.xpl.dylib ${MACPATH}/${FOLDER}/mac.xpl"
    cp libmac.xpl.dylib ${MACPATH}/${FOLDER}/mac.xpl

elif [[ "$OSTYPE" == "win32" ]]; then
    echo "Windows"
else
    echo "Unknown"
fi
