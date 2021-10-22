# jwdpmi_test
This repository contains several applications to test and demonstrate the use
of [jwdpmi](https://github.com/jwt27/libjwdpmi).  It also serves as a project
template to start developing and debugging DPMI programs in Visual Studio.

## Test programs:
### `hello`
A simple "Hello World" application.

### `game`
A basic video game where you control a character using the joystick or
keyboard.  Demonstrates the use of timers, vectors, remapped DOS memory,
keyboard and joystick, etc.

### `vbe`
Displays an animation with real-time alpha-blending, using both integer (MMX)
and floating-point (SSE) math.  Demonstrates the VBE graphics interface and
pixel layout structures.

### `keys`
Application to test keyboard functionality.

### `ring0`
Application to test ring 0 access.

## To build:
* Build and install gcc with [`--target=i386-pc-msdosdjgpp`](https://github.com/jwt27/build-gcc).
* Clone this repo with its submodules:
```sh
$ git clone https://github.com/jwt27/jwdpmi_test.git
$ cd jwdpmi_test/
$ git submodule update --init --recursive
```
* Configure:
```sh
$ mkdir build/
$ cd build/
$ ../configure --host=i386-pc-msdosdjgpp
```
* Build:
```sh
$ make -j all                       # build all subprograms
$ make -j hello                     # build only "Hello World"
$ make -j hello FDD=/media/floppy   # build "Hello World" and copy it to a floppy disk
```

## To build with Visual Studio:
* Install Visual Studio 2019 and MSYS2.
* Build and install the [toolchain](https://github.com/jwt27/build-gcc) with mingw-w64.
* Build and install [gcc2vs](https://github.com/jwt27/gcc2vs) with mingw-w64.
* Make sure your Windows `%PATH%` has access to `bash` and your Bash `$PATH` can access the toolchain.
* From a mingw-w64 shell, run `./vs-configure.sh` to generate project files.
* Open this project in Visual Studio by selecting "Open Folder".
* Select a build configuration from the dropdown menu on the toolbar.
* Right-click any `.cpp` file in the top-level `src/` directory and select Build.

## To debug with Visual Studio:
I can no longer get this to work with recent Visual Studio versions :(
