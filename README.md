# softfm-jj1bdx: Software decoder for FM broadcast radio with RTL-SDR (for MacOS and Linux)

* This repository is forked from [SoftFM](https://github.com/jorisvr/SoftFM)
* SoftFM is an SDR of FM radio for RTL-SDR

## Modification of this fork from the original SoftFM

* Remove ALSA dependency to make this software run on macOS
* Fix other glitches on macOS
* Merge [ngsoftfm](https://github.com/f4exb/ngsoftfm) code
* Add quiet mode

### macOS usage example (also works on Linux)

    ./softfm -f 88100000 -g 8.7 -b 1.0 -q -R - | \
        play -t raw -esigned-integer -b16 -r 48000 -c 2 -q -

## Requirement

* Linux or macOS
* C++11 (gcc and clang will do)
* [RTL-SDR library](http://sdr.osmocom.org/trac/wiki/rtl-sdr)
* Supported DVB-T receiver
* Medium-fast computer (SoftFM takes 25% CPU time on my 1.6 GHz Core i3)
* Medium-strong FM radio signal

## Installation

The Osmocom RTL-SDR library must be installed before you can build SoftFM.
See <http://sdr.osmocom.org/trac/wiki/rtl-sdr> for more information.
SoftFM has been tested successfully with RTL-SDR 0.5.3.

To install SoftFM, download and unpack the source code and go to the
top level directory. Then do like this:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    
CMake tries to find librtlsdr. If this fails, you need to specify
the location of the library in one the following ways:

    $ cmake .. -DCMAKE_INSTALL_PREFIX=/path/rtlsdr
    $ cmake .. -DRTLSDR_INCLUDE_DIR=/path/rtlsdr/include -DRTLSDR_LIBRARY_PATH=/path/rtlsdr/lib/librtlsdr.a
    $ PKG_CONFIG_PATH=/path/rtlsdr/lib/pkgconfig cmake ..
    
## Authors

* Joris van Rantwijk
* Edouard Griffiths, F4EXB
* Kenji Rikitake, JJ1BDX

## License

* As a whole package: GPLv3 (and later). See [LICENSE](LICENSE).
* Some source code files are stating GPL "v2 and later" license.
