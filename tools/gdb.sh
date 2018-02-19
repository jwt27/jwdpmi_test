#!/bin/bash
i586-pc-msdosdjgpp-gdb -ex "set auto-load local-gdbinit" -ex "target remote $1"