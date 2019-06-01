#!/bin/bash

cat << STOP > tasks.vs.json
{
  "version": "0.2.1",
  "tasks": [
    {
      "appliesTo": "tools/generate-vs-tasks.sh",
      "type": "launch",
      "taskLabel": "generate-tasks",
      "contextType": "build",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "-B"
      ],
      "output": "tasks.vs.json"
    },
STOP

for file in $(ls src/*.cpp); do
    f=$(basename $file)
    f=${f%.*}
    cat << STOP >> tasks.vs.json
    {
      "appliesTo": "/src/${f}.cpp",
      "type": "launch",
      "taskLabel": "build-${f}",
      "contextType": "build",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "${f}",
        "/a/${f}.exe"
      ],
      "output": "bin/${f}.exe"
    },
    {
      "appliesTo": "/src/${f}.cpp",
      "type": "launch",
      "taskLabel": "rebuild-${f}",
      "contextType": "rebuild",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "-B",
        "${f}",
        "/a/${f}.exe"
      ],
      "output": "bin/${f}.exe"
    },
STOP
done

cat << STOP >> tasks.vs.json
{
      "appliesTo": "makefile",
      "type": "launch",
      "taskLabel": "build-all",
      "contextType": "build",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "all"
      ]
    },
    {
      "appliesTo": "makefile",
      "type": "launch",
      "taskLabel": "rebuild-all",
      "contextType": "rebuild",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "-B",
        "all"
      ]
    },
    {
      "appliesTo": "makefile",
      "type": "launch",
      "taskLabel": "clean-all",
      "contextType": "clean",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "clean"
      ]
    },
    {
      "appliesTo": "/src/*.cpp",
      "type": "launch",
      "taskLabel": "clean-all-cpp",
      "contextType": "clean",
      "workingDirectory": "\${workspaceRoot}",
      "command": "\${workspaceRoot}/make_vs.bat",
      "args": [
        "\${workspaceRoot}",
        "clean"
      ]
    }
  ]
}
STOP
