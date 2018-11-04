# softfm-jj1bdx: Software decoder for FM broadcast radio with RTL-SDR (for MacOS and Linux)

* This repository is forked from [SoftFM](https://github.com/jorisvr/SoftFM)

## Modification of this fork from the original SoftFM

* Remove ALSA dependency to make this software run on macOS
* Fix other glitches on macOS
* Merge [ngsoftfm](https://github.com/f4exb/ngsoftfm) code
* Add quiet mode

### macOS usage example (also works on Linux)

    ./softfm -f 88100000 -g 8.7 -b 1.0 -q -R - | \
        play -t raw -esigned-integer -b16 -r 48000 -c 2 -q -

## Original README begins here

SoftFM is a software-defined radio receiver for FM broadcast radio.
It is written in C++ and uses RTL-SDR to interface with RTL2832-based
hardware.

This program is mostly an experiment rather than a useful tool.
The purposes of SoftFM are

* experimenting with digital signal processing and software radio;
* investigating the stability of the 19 kHz pilot;
* doing the above while listening to my favorite radio station.

Having said that, SoftFM actually produces pretty good stereo sound
when receiving a strong radio station.  Weak stations are noisy,
but SoftFM gets much better results than rtl\_fm (bundled with RTL-SDR)
and the few GNURadio-based FM receivers I have seen.

SoftFM provides:

* mono or stereo decoding of FM broadcasting stations
* real-time playback to soundcard or dumping to file
* command-line interface (no GUI, no visualization, nothing fancy)

SoftFM requires:

* Linux
* C++11
* RTL-SDR library (http://sdr.osmocom.org/trac/wiki/rtl-sdr)
* supported DVB-T receiver
* medium-fast computer (SoftFM takes 25% CPU time on my 1.6 GHz Core i3)
* medium-strong FM radio signal

For the latest version, see https://github.com/jorisvr/SoftFM

## Installing

The Osmocom RTL-SDR library must be installed before you can build SoftFM.
See http://sdr.osmocom.org/trac/wiki/rtl-sdr for more information.
SoftFM has been tested successfully with RTL-SDR 0.5.3.

To install SoftFM, download and unpack the source code and go to the
top level directory. Then do like this:

    $ mkdir build
    $ cd build
    $ cmake ..
    
    # CMake tries to find librtlsdr. If this fails, you need to specify
    # the location of the library in one the following ways:
    #
    #  $ cmake .. -DCMAKE_INSTALL_PREFIX=/path/rtlsdr
    #  $ cmake .. -DRTLSDR_INCLUDE_DIR=/path/rtlsdr/include -DRTLSDR_LIBRARY_PATH=/path/rtlsdr/lib/librtlsdr.a
    #  $ PKG_CONFIG_PATH=/path/rtlsdr/lib/pkgconfig cmake ..
    
    $ make
    
    $ ./softfm -f <radio-frequency-in-Hz>
    
    # ( enjoy music )

## Authors

* Joris van Rantwijk
* Edouard Griffiths, F4EXB
* Kenji Rikitake, JJ1BDX

## License

* As a whole package: GPLv3 (and later). See [LICENSE](LICENSE).
* Some source code files are stating GPL "v2 and later" license.
