#!/bin/sh -x

./netflow_check.py -p per > out.log
./netflow_check.py -p ade >> out.log
./netflow_check.py -p bur >> out.log
./netflow_check.py -p mel >> out.log
./netflow_check.py -p can >> out.log
./netflow_check.py -p hay >> out.log
./netflow_check.py -p syd >> out.log
./netflow_check.py -p bri >> out.log
./netflow_check.py -p for >> out.log
