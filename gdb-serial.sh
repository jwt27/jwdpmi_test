#!/bin/bash
/c/windows/system32/mode COM1: BAUD=115200 PARITY=N DATA=8 STOP=1
i586-pc-msdosdjgpp-gdb -ex "set serial baud 115200" -ex "target remote $1" bin/dpmitest-debug.exe
