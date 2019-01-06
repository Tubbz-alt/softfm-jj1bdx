# softfm-jj1bdx: Software decoder for FM broadcast radio with RTL-SDR (for MacOS and Linux)

* This repository is forked from [SoftFM](https://github.com/jorisvr/SoftFM)
* SoftFM is an SDR of FM radio for RTL-SDR

## Modification of this fork from the original SoftFM

* Ongoing: merge [ngsoftfm](https://github.com/f4exb/ngsoftfm) code
* Remove ALSA dependency to make this software run on macOS
* Fix other glitches on macOS
* Add quiet mode `-q`
* Add option `-X` for [Quadratic Multipath Monitor (QMM)](http://ham-radio.com/k6sti/qmm.htm)
* Add D/U ratio estimation based on I/F level: see <https://github.com/jj1bdx/rtl_power-fm-multipath> (this requires higher sampling frequency (>900kHz))
* IF bandwidth: 200kHz (+-100kHz)
* Set default sample rate to 960kHz (use `-L` to set to 240kHz), `-s` option removed
* Remove 19kHz pilot signal when the stereo PLL is locked
* Add option `-U` to set deemphasis timing to 75 microseconds for North America (default: 50 microseconds for Europe/Japan)
* Add experimental equalizer to compensate 0th-hold aperture effect of phase discriminator output (currently with fixed parameter for 240kHz/960kHz sampling rates)

### Usage example

* See `python-scripts/fmradio.py`

       softfm -f 88100000 -g 12.5 -b 0.5 -R - | \
          play -t raw -esigned-integer -b16 -r 48000 -c 2 -

## Tested hardware

### R820T2 SDR device

* All with RTL-SDR V3 dongle by rtl-sdr.com

### Computers

* Mac mini 2018 3.2GHz Intel Core i7 / macOS 10.14.2 (CPU time: ~9%)
* Intel NUC DN2820FYK Celeron N2830 / Ubuntu 18.04 (CPU time: ~45%)
* Intel NUC D54327RK i5-3427U / FreeBSD 11.2-STABLE
* Raspberry Pi 2 Model B Rev 1.2 / Raspbian (240k samples/sec only)

### Audio devices

* Mac mini: Roland UA-M10
* Other machines: C-Media Electronics (generic USB Audio device with headphone output and mic input)

## Software requirement

* Linux / macOS / FreeBSD
* C++11 (gcc or clang)
* [RTL-SDR library](http://sdr.osmocom.org/trac/wiki/rtl-sdr)
* [sox](http://sox.sourceforge.net/)
* Supported DVB-T receiver
* Medium-fast computer
* Medium-strength FM radio signal

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
