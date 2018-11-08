# softfm-jj1bdx: Software decoder for FM broadcast radio with RTL-SDR (for MacOS and Linux)

* This repository is forked from [SoftFM](https://github.com/jorisvr/SoftFM)
* SoftFM is an SDR of FM radio for RTL-SDR

## Modification of this fork from the original SoftFM

* Remove ALSA dependency to make this software run on macOS
* Fix other glitches on macOS
* Merge [ngsoftfm](https://github.com/f4exb/ngsoftfm) code
* Add quiet mode

### Usage example

       # normal sample rate
       softfm -f 88100000 -g 8.7 -s 960000 -b 1.0 -R - | \
          "play -t raw -esigned-integer -b16 -r 48000 -c 2 -
       # lower sample rate for low performance hosts
       softfm -f 88100000 -g 8.7 -s 240000 -r 44100 -b 2.0 -R - | \
          "play -t raw -esigned-integer -b16 -r 44100 -c 2 -

## Tested hardware

* All with RTL-SDR V3 dongle by rtl-sdr.com

* Mac mini Late 2012 2.6GHz Intel Core i7 / macOS 10.14.1 (CPU time: ~12%)
* Intel NUC DN2820FYK Celeron N2830 / Ubuntu 18.04 (CPU time: ~45%)
* Raspberry Pi 2 Model B Rev 1.2 / Raspbian (240k samples/sec only) (CPU time: ~63%)
* Intel NUC D54327RK i5-3427U / FreeBSD 11.2-STABLE (CPU time: ~21%)

* Mac mini: Roland UA-M10
* Other machines: C-Media Electronics (generic USB Audio device with headphone output and mic input)

## Software equirement

* Linux / macOS / FreeBSD
* C++11 (gcc and clang will do)
* [RTL-SDR library](http://sdr.osmocom.org/trac/wiki/rtl-sdr)
* [sox](http://sox.sourceforge.net/)
* Supported DVB-T receiver
* Medium-fast computer
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
