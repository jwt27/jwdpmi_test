@echo off
set MSYSTEM=MINGW64
set CHERE_INVOKING=1
%MSYS2_ROOT%/usr/bin/bash -lc "./tools/run.sh %*"
