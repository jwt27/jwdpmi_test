#!/bin/bash
export PATH=/$MSYS2_DJGPP/i586-pc-msdosdjgpp/bin:$PATH;
export GCC_EXEC_PREFIX=/$MSYS2_DJGPP/lib/gcc/;
/c/windows/system32/mode COM1: BAUD=115200 PARITY=N DATA=8 STOP=1 to=on
gdb --data-directory=/usr/local/djgpp/share/gdb -ex "target remote COM1" bin/dpmitest.exe
