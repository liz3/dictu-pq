# Dictu pg
A simple postgresql client driver for [Dictu](https://dictu-lang.com)

# Building
Clone the repository:
```sh
git clone --recurse-submodules https://github.com/liz3/dictu-pg.git
```

## Prerequisites For Windows
* [ActiveState Perl distribution](https://platform.activestate.com/ActiveState-Projects/ActiveState-Perl-5.36.0)
* [bison.exe and flex.exe from Cygwin](https://www.cygwin.com/install.html)

## Building psql
### MacOS/Linux
```sh
cd third-party/pq
./configure with_icu=no --prefix=$(pwd)/build
make -j && make install
```
### Windows
**Note: If you Cygwin installation is NOT `c:\cygwin64` then update `third-party\pq\src\tools\msvc\buildenv.pl`**  
**Note: Make sure there are no cygwin or msys compilers in your path, so to use the MSVC Compiler**
```sh
cd <repository root>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cd <repository-root>\third-party\pq\src\tools\msvc
.\build.bat
cd third-party\pq
.\src\tools\msvc\install.bat .\build
```

## Building dictu-pg
### Linux and MacOS
```sh
cd <repository root>
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```
### Windows
```sh
cd <repository root>
cd build
cmake --build . --config Release
```
**Note: On windows after building you need to copy the DLL`s from `third-party\pg\build\lib` into `build\Release` for the mod to work.**

# Usage
`from "mod.du" import pg;`