#!/bin/bash
export PATH=/$MSYS2_DJGPP/i586-pc-msdosdjgpp/bin:$PATH;
export GCC_EXEC_PREFIX=/$MSYS2_DJGPP/lib/gcc/;
gdb --data-directory=/usr/local/djgpp/share/gdb -ex "target remote :3333" bin/dpmitest.exe
