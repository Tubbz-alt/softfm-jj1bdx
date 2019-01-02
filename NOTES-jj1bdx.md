# Miscellaneous Notes

by @jj1bdx

## Signal comparison with other receivers

17-NOV-2018

Virtually no big difference between softfm-jj1bdx, SDR-Radio Console,
and a standalone receiver (ONKYO R-801A) connected to the same antenna system
(Nippon Antenna AF-1-SP dipole) at the balcony, ~20km direct view from the
FM transmission sites of Osaka region (Iimori Yama / Ikoma San).

Measured result of no-sound period and 880Hz time report tone of NHK-FM Osaka (88.1MHz):

* Noise level of the no-sound period: ~ -70dBFS +- 1dBFS
* THD+N level of the 880Hz tone: ~ 0.9% (multipath distortion possible) +- 0.1%

The antenna system seems to be the main factor.

Update: measurement on 2-JAN-2019 showed the similar results.

## Quadratic Multipath Monitor (QMM)

16-DEC-2018

Brian Beezley, K6STI, describes his idea of [Quadratic Multipath Monitor (QMM)](http://ham-radio.com/k6sti/qmm.htm) by *simply demodulating the subcarrier region with a quadrature oscillator of 90-degree shifted pilot signal*, called *QMM*.

The QMM output has the following characteristics: (quote from the Brian's page)

> For a perfectly transmitted and received signal, you'll get no output. For a real signal you may hear L+R harmonics, phase-rotated L−R intermodulation products, or crossmodulation between L+R, L−R, SCA, RDS, or HD Radio sidebands. Multipath propagation can cause any of these artifacts. You may also hear co-channel interference, adjacent-channel interference, HD Radio self-noise, or background noise.

I've implemented the QMM function as `-X` option, and with the variable `pilot_shift`. This option shifts the phase of regenerated subcarrier for decoding the L-R DSB signal from the original `sin(2*x)` (where x represents the 19kHz pilot frequency) to `cos(2*x)`.

## Phase error of 19kHz PLL

1-JAN-2019

Measurements of the local FM stations in Osaka suggest the maximum phase error of 19kHz PLL is +- 0.01~0.02 radian at maximum.

[More to go]
