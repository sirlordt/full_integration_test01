#!/bin/sh

#/usr/bin/cmake --build /home/dsistemas/Desktop/CPP/projects/sand_box/build/debug --config Debug --target main -j 10 --

# *****************************************
# You must has installed conan wide system:
#  conan --version
# To install:
#  pip conan
# *****************************************

if [ ! -d "build" ]; then

  if [ $1 = 'make' ]; then
    echo "*** Generating with make *** "
    /usr/bin/cmake -DCMAKE_BUILD_TYPE:STRING=Debug -H. -B./build/debug -G 'Unix Makefiles'
  else
    echo "*** Generating with ninja ***"
    /usr/bin/cmake -DCMAKE_BUILD_TYPE:STRING=Debug -H. -B./build/debug -G 'Ninja'
  fi

fi

rm -f ./build/debug/application/main

/usr/bin/cmake --build ./build/debug --config Debug --target main -j 10 --clean-first --

echo ""

echo "*** checking deps from executable ***"
ldd ./build/debug/application/main
echo "*** checking deps from executable ***"

echo ""

echo "*** run executable ***"
./build/debug/application/main
echo "*** run executable ***"

echo ""

# -s --leak-check=full
