#!/usr/bin/env bash

set -e
readonly src="$(cd $(dirname "$0") && pwd)"
cd "$src"

unset $(./configure --query-vars)

host=
for i in i{3..7}86-pc-msdosdjgpp; do
	if which $i-g++; then
		host=$i
		break
	fi
done
[[ -z "$host" ]] && exit 1
CXX=$host-g++

declare -A archs
archs[i386]='-march=i386'
archs[pentium-mmx]='-march=pentium-mmx'
archs[pentium3]='-march=pentium3 -mfpmath=both'
archs[athlon]='-march=athlon'

declare -A configs
configs[debug]='--debug=yes'
configs[optimize]='--debug=no'

declare -A dirs
declare -A dir_flags

rm -rf build/

for a in ${!archs[@]}; do
	for c in ${!configs[@]}; do
		dir="$src/build/$a-$c/"
		dirs[$a-$c]="$dir"
		dir_flags[$a-$c]="${archs[$a]}"
		mkdir -p "$dir"
		cd "$dir"
		echo Configuring target $a-$c
		"$src/configure" CXXFLAGS="${archs[$a]}" ${configs[$c]} --host=$host
		cd "$src"
	done
done

mkdir -p .vs/

echo Generating CMakeWorkspaceSettings.json
cat <<-EOF > .vs/CMakeWorkspaceSettings.json
{
  "enableCMake": false
}
EOF

echo Generating CppProperties.json
{
	cat <<-EOF
	{
	  "projectName": "jwdpmi_test",
	  "configurations": [
	EOF
	put_comma=
	for d in "${!dirs[@]}"; do
		[[ ! -z "$put_comma" ]] && echo ','
		put_comma=yes
		dir="${dirs[$d]}"
		mkdir -p "$dir/tools/"
		cxxflags="$(tr '\n' ' ' < "$dir/cxxflags")"
		cxxflags+=" ${dir_flags[$d]}"
		$host-g++ $cxxflags -x c++ -E -dM - < /dev/null > "$dir/tools/gcc_defines.h"
		cat <<-EOF
	    {
	      "name": "$d",
	      "includePath": [
		EOF
		for i in $(echo | $host-g++ $cxxflags -x c++ -E -Wp,-v - 2>&1 1> /dev/null | tr -d '\r' | grep '^ ' | sed 's/^ *//g'); do
			echo "\"$(cygpath -w "$i")\""
		done | tr '\\' '/' | sed 's/$/,/g'
		cat <<-EOF
	        "$(cygpath -w "$dir/tools" | tr '\\' '/')"
	      ],
	      "intelliSenseMode": "linux-gcc-x86",
	      "forcedInclude": [
	        "\${workspaceRoot}/tools/intellisense.h"
	      ],
	      "compilerSwitches": "-undef -std=gnu++2a"
	    }
		EOF
	done
	cat <<-EOF
	  ]
	}
	EOF
} > CppProperties.json

echo Generating tasks.vs.json
{
	cat <<- EOF
	{
	  "version": "0.2.1",
	  "tasks": [
	    {
	      "appliesTo": "vs-configure.sh",
	      "type": "launch",
	      "taskLabel": "generate-tasks",
	      "contextType": "build",
	      "workingDirectory": "\${workspaceRoot}",
	      "command": "\${workspaceRoot}/tools/run.cmd",
	      "env": { "build_config": "" },
	      "args": [
	        "./configure-vs.sh"
	      ]
	    },
	EOF

	for file in $(cd src/ && echo *.cpp); do
		f=${file%.*}
		cat <<- EOF
		    {
		      "appliesTo": "/src/$f.cpp",
		      "type": "launch",
		      "taskLabel": "build-$f",
		      "contextType": "build",
		      "workingDirectory": "\${workspaceRoot}",
		      "env": { "build_config": "\${cpp.activeConfiguration}", "FDD": "/a" },
		      "command": "\${workspaceRoot}/tools/make.cmd",
		      "args": [
		        "$f"
		      ]
		    },
		    {
		      "appliesTo": "/src/$f.cpp",
		      "type": "launch",
		      "taskLabel": "rebuild-$f",
		      "contextType": "rebuild",
		      "workingDirectory": "\${workspaceRoot}",
		      "env": { "build_config": "\${cpp.activeConfiguration}", "FDD": "/a" },
		      "command": "\${workspaceRoot}/tools/make.cmd",
		      "args": [
		        "-B $f"
		      ]
		    },
		EOF
	done

	cat <<- EOF
	    {
	      "appliesTo": "/configure",
	      "type": "launch",
	      "taskLabel": "build-all",
	      "contextType": "build",
	      "workingDirectory": "\${workspaceRoot}",
	      "env": { "build_config": "\${cpp.activeConfiguration}" },
	      "command": "\${workspaceRoot}/tools/make.cmd",
	      "args": [
	        "all"
	      ]
	    },
	    {
	      "appliesTo": "/configure",
	      "type": "launch",
	      "taskLabel": "rebuild-all",
	      "contextType": "rebuild",
	      "workingDirectory": "\${workspaceRoot}",
	      "env": { "build_config": "\${cpp.activeConfiguration}" },
	      "command": "\${workspaceRoot}/tools/make.cmd",
	      "args": [
	        "-B all"
	      ]
	    },
	    {
	      "appliesTo": "/configure",
	      "type": "launch",
	      "taskLabel": "clean-all",
	      "contextType": "clean",
	      "workingDirectory": "\${workspaceRoot}",
	      "env": { "build_config": "\${cpp.activeConfiguration}" },
	      "command": "\${workspaceRoot}/tools/make.cmd",
	      "args": [
	        "clean"
	      ]
	    },
	    {
	      "appliesTo": "/src/*.cpp",
	      "type": "launch",
	      "taskLabel": "clean-all-cpp",
	      "contextType": "clean",
	      "workingDirectory": "\${workspaceRoot}",
	      "env": { "build_config": "\${cpp.activeConfiguration}" },
	      "command": "\${workspaceRoot}/tools/make.cmd",
	      "args": [
	        "clean"
	      ]
	    }
	  ]
	}
	EOF
} > .vs/tasks.vs.json

echo Generating launch.vs.json
{
	cat <<- EOF
	{
	  "version": "0.2.1",
	  "defaults": {},
	  "configurations": [
	EOF

	for file in $(cd src/ && echo *.cpp); do
		f=${file%.*}
		cat <<- EOF
		    {
		      "type": "cppdbg",
		      "name": "$f (local VM)",
		      "project": "/src/$f.cpp",
		      "cwd": "\${workspaceRoot}/tools/",
		      "MIMode": "gdb",
		      "miDebuggerPath": "\${workspaceRoot}/tools/gdb.cmd",
		      "env": { "GDB": "$host-gdb" },
		      "program": "\${workspaceRoot}/build/\${cpp.activeConfiguration}/$f-debug.exe",
		      "setupCommands": [
		        { "text": "set auto-load local-gdbinit" }
		      ],
		      "request": "attach",
		      "miDebuggerServerAddress": ":3333",
		      "launchCompleteCommand": "exec-continue"
		    },
		    {
		      "type": "cppdbg",
		      "name": "$f (serial COM1)",
		      "project": "/src/$f.cpp",
		      "cwd": "\${workspaceRoot}/tools/",
		      "MIMode": "gdb",
		      "miDebuggerPath": "\${workspaceRoot}/tools/gdb.cmd",
		      "env": { "GDB": "$host-gdb" },
		      "program": "\${workspaceRoot}/build/\${cpp.activeConfiguration}/$f-debug.exe",
		      "setupCommands": [
		        { "text": "set auto-load local-gdbinit" },
		        { "text": "set serial baud 115200" }
		      ],
		      "request": "attach",
		      "miDebuggerServerAddress": "COM1",
		      "launchCompleteCommand": "exec-continue"
		    },
		EOF
	done

	cat <<- EOF
	    {
	      "name": "--> Regenerate VS project files",
	      "type": "default",
	      "project": "configure-vs.sh"
	    },
	    {
	      "name": "All",
	      "type": "default",
	      "project": "configure"
	    }
	  ]
	}
	EOF
} > .vs/launch.vs.json
