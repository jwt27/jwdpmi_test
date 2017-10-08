# jwdpmi_test
This is a test application I use to develop [libjwdpmi](https://github.com/jwt27/libjwdpmi). It also serves as a project template to start developing djgpp programs in Visual Studio.

## To build:
* Build and install djgpp.
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
* Build and install djgpp with mingw64.
* Set up two global environment variables:
```
> setx MSYS2_ROOT C:\msys64
> setx MSYS2_DJGPP usr/local/djgpp
```
* Build and install [gcc2vs](https://github.com/jwt27/gcc2vs).
* Open this project in Visual Studio by selecting "Open Folder".
* Select `makefile` as the "startup item".
* Press ctrl-shift-b to build.
