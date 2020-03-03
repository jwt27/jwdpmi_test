#!/bin/bash
#uname -o | grep Msys > /dev/null && cmd /C "MODE COM1: BAUD=115200 DATA=8 PARITY=N STOP=1"
i386-pc-msdosdjgpp-gdb -ex "set auto-load local-gdbinit" -ex "target remote $1"
