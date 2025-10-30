#!/bin/sh

mkdir -p ./bin
cd Widget; make clean; make; cd ..
cd ControlChannel; make clean; make; cd ..
