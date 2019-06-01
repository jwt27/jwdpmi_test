#!/bin/bash

cat << STOP > launch.vs.json
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
STOP

for file in $(ls src/*.cpp); do
    f=$(basename $file)
    f=${f%.*}
    cat << STOP >> launch.vs.json
    {
      "type": "cppdbg",
      "name": "${f} (local VM)",
      "project": "src/${f}.cpp",
      "cwd": "\${workspaceRoot}",
      "MIMode": "gdb",
      "miDebuggerPath": "\${workspaceRoot}/tools/gdb.bat",
      "program": "\${workspaceRoot}/bin/${f}-debug.exe",
      "setupCommands": [
        { "text": "set auto-load local-gdbinit" }
      ],
      "request": "attach",
      "miDebuggerServerAddress": ":3333",
      "launchCompleteCommand": "exec-continue"
    },
    {
      "type": "cppdbg",
      "name": "${f} (serial COM1)",
      "project": "src/${f}.cpp",
      "cwd": "\${workspaceRoot}/tools/",
      "MIMode": "gdb",
      "miDebuggerPath": "\${workspaceRoot}/tools/gdb.bat",
      "program": "\${workspaceRoot}/bin/${f}-debug.exe",
      "setupCommands": [
        { "text": "set auto-load local-gdbinit" },
        { "text": "set serial baud 115200" }
      ],
      "request": "attach",
      "miDebuggerServerAddress": "COM1",
      "launchCompleteCommand": "exec-continue"
    },
STOP
done

cat << STOP >> launch.vs.json
    {
      "name": "--> Regenerate VS project files",
      "type": "default",
      "project": "tools/generate-vs-tasks.sh"
    },
    {
      "name": "All",
      "type": "default",
      "project": "makefile"
    }
  ]
}
STOP

