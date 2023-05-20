# dtachez

A fork of [dtach](https://dtach.sourceforge.net/) that uses pipes instead of Unix sockets.

At most 126 clients are supported for each server.

You will only need this if you removed the sockets support in your kernel configuration.

## Usage
The usage is exactly the same. See `README.old` for more info.

## Build
C++11 support and CMake are required.

## Caveats
There are probably some unhandled edge cases. Use with caution.

## License
GPLv3
