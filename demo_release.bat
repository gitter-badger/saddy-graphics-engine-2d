IF not exist build\nul ( mkdir build )
cd makefiles
mingw32-make -f Makefile_Demo_Release.mingw
cd ..