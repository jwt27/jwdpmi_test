# jwdpmi_test
This repository contains several applications to test and demonstrate the use of [libjwdpmi](https://github.com/jwt27/libjwdpmi). It also serves as a project template to start developing and debugging DPMI programs in Visual Studio.

## Branches:
### `master`
A simple "Hello World" application.

###`game`
A basic video game where you control a character using the joystick or keyboard.
Demonstrates the use of timers, vectors, remapped DOS memory, keyboard and joystick, etc.

###`vbe`
Displays an animation with real-time alpha-blending, using both integer (MMX) and floating-point (SSE) math.
Demonstrates the VBE graphics interface and pixel layout structures.

## To build:
* Build and install gcc with [`--target=i586-pc-msdosdjgpp-g++`](https://github.com/jwt27/build-gcc).
* Clone this repo with its submodule:
```
$ git clone https://github.com/jwt27/jwdpmi_test.git
$ cd jwdpmi_test/
$ git submodule update --init
```
* Build:
```
$ make -j
```

## To build with Visual Studio:
* Install Visual Studio 2017.
* Build and install [i586-pc-msdosdjgpp-g++](https://github.com/jwt27/build-gcc) with mingw64.
* Set up two global environment variables:
```
> setx MSYS2_ROOT C:\msys64		# Where you installed MSYS2
> setx MSYS2_DJGPP /usr/local/cross	# Where you installed gcc (`--prefix=...`)
```
* Build and install [gcc2vs](https://github.com/jwt27/gcc2vs).
* Open this project in Visual Studio by selecting "Open Folder".
* Select `makefile` as the "startup item".
* Press ctrl-shift-b to build.

## To debug with Visual Studio:
* Make sure you have a cross-gdb installed in `%MSYS2_DJGPP%/bin/i586-pc-msdosdjgpp-gdb`.
* Connect your target machine with a serial null-modem cable on COM1.
* Select `Debug (remote COM1)` as startup item in VS.
* Build and copy `dpmitest.exe` to the target machine.
* Launch the target with `dpmitest.exe --debug`.
* Hit F5 in Visual Studio. Have fun.

![debugging example](https://i.imgur.com/HsREynj.png)

![debugging example](https://i.imgur.com/m5dQgs3.png)
