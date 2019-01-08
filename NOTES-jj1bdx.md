# Miscellaneous Notes

by @jj1bdx

## Signal comparison with other receivers

### 17-NOV-2018

Virtually no big difference between softfm-jj1bdx, SDR-Radio Console,
and a standalone receiver (ONKYO R-801A) connected to the same antenna system
(Nippon Antenna AF-1-SP dipole) at the balcony, ~20km direct view from the
FM transmission sites of Osaka region (Iimori Yama / Ikoma San).

Measured result of no-sound period and 880Hz time report tone of NHK-FM Osaka (88.1MHz):

* Noise level of the no-sound period: ~ -70dBFS +- 1dBFS
* THD+N level of the 880Hz tone: ~ 0.9% (multipath distortion possible) +- 0.1%

The antenna system seems to be the main factor.

### 3-JAN-2019

For softfm-jj1bdx, the following modification was made:

* LNA Gain: 8.7dB -> 12.5dB
* [Removal of RF spurious beat signal](https://github.com/jj1bdx/softfm-jj1bdx/issues/5)
* Increasing FIR downsampling filter stages by 25%

Revised measured result of no-sound period and 880Hz time report tone of NHK-FM Osaka (88.1MHz):

* Noise level of the no-sound period: ~ -74dBFS +- 1dBFS
* THD+N level of the 880Hz tone: ~ 0.82% (multipath distortion possible) +- 0.1%

### 7-JAN-2019

For 240kHz sampling frequency:

Measured result of no-sound period and 880Hz time report tone of NHK-FM Osaka (88.1MHz):

* Noise level of the no-sound period: ~ -57dBFS (Two beat signals at ~9kHz and ~10kHz, reason unknown)
* THD+N level of the 880Hz tone: ~ 1.07% (multipath distortion possible) +- 0.3%

## Quadratic Multipath Monitor (QMM)

### 16-DEC-2018

Brian Beezley, K6STI, describes his idea of [Quadratic Multipath Monitor (QMM)](http://ham-radio.com/k6sti/qmm.htm) by *simply demodulating the subcarrier region with a quadrature oscillator of 90-degree shifted pilot signal*, called *QMM*.

The QMM output has the following characteristics: (quote from the Brian's page)

> For a perfectly transmitted and received signal, you'll get no output. For a real signal you may hear L+R harmonics, phase-rotated L竏坦 intermodulation products, or crossmodulation between L+R, L竏坦, SCA, RDS, or HD Radio sidebands. Multipath propagation can cause any of these artifacts. You may also hear co-channel interference, adjacent-channel interference, HD Radio self-noise, or background noise.

I've implemented the QMM function as `-X` option, and with the variable `pilot_shift`. This option shifts the phase of regenerated subcarrier for decoding the L-R DSB signal from the original `sin(2*x)` (where x represents the 19kHz pilot frequency) to `cos(2*x)`.

## Phase error of 19kHz PLL

### 1-JAN-2019

Measurements of the local FM stations in Osaka suggest the maximum phase error of 19kHz PLL is +- 0.01~0.02 radian at maximum.

## Decimation by DownsampleFilter

### 2-JAN-2019

Over-optimizing assumption of integer ration on `m_resample_mono()` and `m_resample_stereo()` leads into an assertion error such as:

```
softfm: /home/kenji/src/softfm-jj1bdx/sfmbase/Filter.cpp:204: void DownsampleFilter::process(const SampleVector&, SampleVector&): Assertion `i == samples_out.size()' failed.
```

Dealing the assertion error with the compiler optimization option change is a wrong way.

## Aperture effect on Phase Discriminator output

### 3-JAN-2019

The output of phase discriminator is affected by [the aperture effect of zero-order hold signals](https://www.maximintegrated.com/en/app-notes/index.mvp/id/3853), and the higher-frequency portion of the signal should be compensated.

For 60kHz, the signal level decreases as:

* Nyquist frequency = 480kHz (ifrate = 960kHz): 0.9745, -0.224dB
* Nyquist frequency = 120kHz (ifrate = 240kHz): 0.6366, -3.922dB (!)

### 7-JAN-2019

Added experimental filter class DiscriminatorEqualizer for 960kHz sample rate. Parameters for 240kHz sample rate have also been computed.

The filter model:

Amplifier setting the maximum static gain + 1st-order moving average filter (an LPF) with a constant gain to *deduce* the signal, so that the result filter emphasizes the higher frequency with the maximum gain set.

The two gain parameters are computed by the least-square-maximum method so that the logarithmic ratio value of the compensation values and the filter outputs for each frequency for 51, 1050, 2050, ..., 57050Hz is minimized and closest to zero.

Note: static gain minus moving filter gain must not be less than 10^(-0.05) (-0.1dB).

Computed result by SciPy scipy.optimize.fmin:

* Sample rate 960kHz: static gain: 1.47112063, moving filter gain (fitfactor): 0.48567701
* Sample rate 240kHz: static gain: 1.3412962, moving filter gain (fitfactor): 0.34135089

## References

(Including Japanese books here with Japanese titles)

* Brian Beezley, K6STI, [88-108MHz](http://ham-radio.com/k6sti/)
* 林 輝彦, FPGA FMチューナ: 第2章 フィルタとFMステレオ復調のメカニズム, [トラ技エレキ工房 No.1](http://www.cqpub.co.jp/hanbai/books/46/46511.htm), CQ出版, 2013, ISBN-13: 9784789846516, pp. 89-119
* 三上 直樹, [はじめて学ぶディジタル・フィルタと高速フーリエ変換](http://www.cqpub.co.jp/hanbai/books/30/30881.htm), CQ出版, 2005, ISBN-13: 9784789830881

[More to go]
