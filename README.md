[![Boosty](https://img.shields.io/badge/Boosty-donate-orange.svg)](https://boosty.to/qucs_s)
[![Telegram](https://img.shields.io/badge/Telegram-chat-blue.svg)](https://t.me/qucs_s)
[![Website](https://img.shields.io/badge/Website-ra3xdh.github.io-29d682.svg)](https://ra3xdh.github.io/)


# Description

QucsatorRF is a command line driven circuit simulator targeted for RF and microwave circuits.
It takes a network list in a certain format as input and outputs a Qucs XML dataset. This repository
also contians a QucsconvRF tool for data file formats conversion.

The Qucs-S project https://github.com/ra3xdh/qucs_s is a recommended GUI for both tools.

# Installation

QucsatorRF is a part of Qucs-S package and usually doesn't require installation as a separate package.
One may compile from source to test the newest version.

Some linux distributions may provide a binary package for QucsatorRF. Check here:
https://repology.org/project/qucsator-rf/versions

# Build instruction

## Dependencies

QucsatorRF uses CMake build system and has the following build dependencies:

* CMake
* Flex
* Bison
* Gperf
* Dos2unix
* ADMS is optional. It is disbaled by default and could be enabled using `-DWITH_ADMS=ON` flag

QucsatorRF has no runtime dependencies except the standard C++ library (libstdc++). ADMS is an optional.

Use the following command to install build dependencies on Debian/Ubuntu. Refer to your package manager
documentation for other platfroms.

~~~
sudo apt-get install build-essential cmake flex bison gperf dos2unix
~~~

## Linux

Perform the following step to build the project:

* Clone this git repository:
~~~
git clone https://github.com/ra3xdh/qucsator_rf
~~~
* Go to the source directory and create the build directory
~~~
cd qucsator_rf
mkdir build
~~~
* Configure project with Cmake; define the desired installation path using `CMAKE_INSTALL_PREFIX`.
~~~
cd build
cmake -DCMAKE_INSTALL_PREFIX=/path_to_install/ ..
~~~
* Build and install the `qucsator_rf` and `qucsconv_rf`
~~~
make
make install
~~~

## Windows

Use MSYS2 environment to build QucsatorRF. Install the GCC compiler and use CMake.
The build procedure and dependencies are the same as for Linux version.


