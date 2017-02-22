#!/bin/bash
mkdir build
cd build
cmake .. -DSIEMENS_DEV_ROOT="$HOME/dev/siemens/"
make VERBOSE=1
