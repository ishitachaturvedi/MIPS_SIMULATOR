#!/bin/bash

echo "Enter SIMULATOR"
read SIMULATOR

echo "I Am $SIMULATOR"

bin/testbench  bin/$SIMULATOR > output.txt

python outputToCsv.py $SIMULATOR