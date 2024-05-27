#!/bin/bash

NS3_DIR="${NS3_DIR:=$HOME/ns-3.37}"

cd $NS3_DIR
./ns3 configure --build-profile=optimized --disable-python --disable-tests --disable-examples
./ns3 build
