@echo off
tools/run.cmd nice -10 make -j$(( $(nproc --all) * 2 )) -Otarget %*
