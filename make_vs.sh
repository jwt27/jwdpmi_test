#!/bin/bash
export PATH=/$MSYS2_DJGPP/i586-pc-msdosdjgpp/bin:$PATH;
export GCC_EXEC_PREFIX=/$MSYS2_DJGPP/lib/gcc/;
unset CXXFLAGS
unset CFLAGS
make -Otarget -j`nproc --all` vs $@
