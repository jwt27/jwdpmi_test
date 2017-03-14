# jwdpmi_test
This program doesn't really do anything. It's just a test application I use to develop [libjwdpmi](https://github.com/jwt27/libjwdpmi).

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
* Install Visual Studio 2015 with the VSLinux extension.
* Build and install djgpp in msys2.
* Set up two global environment variables:
```
> setx MSYS2_ROOT C:\msys64
> setx MSYS2_DJGPP usr/local/djgpp
```
* Build and install [gcc2vs](https://github.com/jwt27/gcc2vs).
* Clone this repo in `~/projects/jwdpmi_test/`.
* Set up `sshd` in msys2 using [this script](https://gist.github.com/samhocevar/00eec26d9e9988d080ac).
* Set the remote build host in Visual Studio to `localhost:22`.
* Press Ctrl-Shift-B.

## To build with Visual Studio and test with Virtualbox:
* Follow the previous instructions.
* Create a VM in Virtualbox named `DOS`.
* Create a 1GB static virtual HDD image called `boot.vhd` in the `tools/` directory.
* Partition with FAT32 and set up a minimal DOS installation (just the kernel+shell should be enough).
* Install [ImDisk](http://www.ltr-data.se/opencode.html/#ImDisk).
* Press Ctrl-Shift-B in Visual Studio.
* Run the VM in Virtualbox.

## To build and debug with Visual Studio from within Virtualbox:
Yeah I haven't figured this part out yet.
