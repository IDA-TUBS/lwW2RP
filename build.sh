#!/bin/bash

# Install script for resource management library

# declare default parameters
threads="6"
dlog="ON"
dconsole="ON"
dfile="ON"
ddebug="ON"

while getopts "j:l:c:f:d" opt; do
  case $opt in
    j)
      threads=$OPTARG
      ;;
    l)
      dlog="OFF"
      ;;
    c)
      dconsole="OFF"
      ;;
    f)
      dfile="OFF"
      ;;
    d)
      ddebug="OFF"
      ;;
  esac
done

if [ ! -d "build" ]; then
    mkdir build
fi
cd build

# Execute the cmake with the stored parameters
cmake .. -DLOG=$dlog -DCONSOLE=$dconsole -DFILE=$dfile -DDEBUG=$ddebug
sudo cmake --build . --target install -j$threads
